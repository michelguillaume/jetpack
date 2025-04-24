#ifndef GAME_SERVER_HPP_
#define GAME_SERVER_HPP_

#include <chrono>
#include <cstring>
#include <iostream>
#include <memory>
#include <poll.h>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

#include "network/protocol.hpp"
#include "network/tcp_server.hpp"
#include "network/tcp_server_connection.hpp"
#include "network/tcp_socket.hpp"
#include "packet_dispatcher.hpp"

#include "game_logic.hpp"
#include "game_map.hpp"
#include "game_state.hpp"

namespace server {

template <typename PacketType> struct PlayerInfo {
  uint32_t id;
  std::shared_ptr<network::TcpServerConnection<PacketType>> conn;
};

template <typename PacketType> class GameServer {
public:
  explicit GameServer(const uint16_t port, std::string mapPath)
      : port_(port), tcp_server_(std::make_unique<TcpServer>(port_)),
        mapPath_(std::move(mapPath)), tileSize_(38.f) {
    if (!game_state_.map.loadFromFile(mapPath_, tileSize_)) {
      throw std::runtime_error("Unable to load map file");
    }

    pollfds_.push_back({tcp_server_->getSocket().get(), POLLIN, 0});
    std::cout << "Server listening on port " << port_ << std::endl;

    dispatcher_.RegisterHandler(
        PacketType::kPlayerInput,
        [this](network::Packet<PacketType> &&pkt, auto &conn) {
          if (auto in = network::PacketFactory<
                  PacketType>::template ExtractData<PlayerInputPacket>(pkt)) {
            game_state_.players[conn->get_player_id()].lastInput = *in;
          }
        });

    dispatcher_.RegisterHandler(
        PacketType::kPlayerReady,
        [this](network::Packet<PacketType> &&pkt, auto &conn) {
          if (auto pr = network::PacketFactory<
                  PacketType>::template ExtractData<PlayerReadyPacket>(pkt)) {
            const uint32_t pid = conn->get_player_id();
            game_state_.players[pid].ready = pr->ready;
            broadcastReadyCount();
            tryStartGame();
          }
        });
  }

  ~GameServer() = default;

  auto &getDispatcher() { return dispatcher_; }

  [[noreturn]] void Run() {
    constexpr uint32_t kUpdateFrequencyTicks{1};

    using clock = std::chrono::steady_clock;
    constexpr auto kTickDuration =
        std::chrono::milliseconds(16); // ~16 ms per tick (~60 fps)
    uint64_t tick_counter = 0;
    auto previous_time = clock::now();
    auto next_tick_time = previous_time;

    while (true) {
      auto current_time = clock::now();
      const float delta_time =
          std::chrono::duration<float>(current_time - previous_time).count();

      previous_time = current_time;

      update_pollfds();
      process_poll_events();

      auto [coins, zaps, deaths, expiries, wins, loses] =
          game_logic_.Update(delta_time, game_state_);

      for (auto &evt : coins) {
        auto pkt = network::PacketFactory<PacketType>::CreatePacket(
            PacketType::kCoinCollected, evt);
        for (auto &pl : players_)
          pl.conn->queue_data(pkt.Data());
      }

      for (auto &evt : zaps) {
        auto pkt = network::PacketFactory<PacketType>::CreatePacket(
            PacketType::kZapperCollision, evt);
        for (auto &pl : players_)
          pl.conn->queue_data(pkt.Data());
      }

      for (auto &evt : deaths) {
        auto pktDeath = network::PacketFactory<PacketType>::CreatePacket(
            PacketType::kPlayerDeath, evt);
        for (auto &pl : players_)
          pl.conn->queue_data(pktDeath.Data());

        uint32_t pid = evt.player_id;
        const uint32_t coinsCount = game_logic_.GetCollectedCount(pid);
        PlayerScorePacket scorePkt{pid, coinsCount};
        auto pktScore = network::PacketFactory<PacketType>::CreatePacket(
            PacketType::kPlayerScore, scorePkt);

        for (auto &pl : players_) {
          if (pl.id == pid) {
            pl.conn->queue_data(pktScore.Data());
            break;
          }
        }
      }

      for (auto &evt : expiries) {
        auto pkt = network::PacketFactory<PacketType>::CreatePacket(
            PacketType::kCoinExpired, evt);
        for (auto &pl : players_)
          pl.conn->queue_data(pkt.Data());
      }

      for (auto &[playerId, coinsCollected] : wins) {
        const uint32_t pid = playerId;
        const uint32_t score = game_logic_.GetCollectedCount(pid);
        PlayerWinPacket wp{pid, score};

        auto pktWin = network::PacketFactory<PacketType>::CreatePacket(
            PacketType::kPlayerWin, wp);
        for (auto &pl : players_)
          pl.conn->queue_data(pktWin.Data());
      }

      for (auto &[playerId, coinsCollected] : loses) {
        const uint32_t pid = playerId;
        const uint32_t score = coinsCollected;
        PlayerLosePacket lp{pid, score};

        auto pktLose = network::PacketFactory<PacketType>::CreatePacket(
            PacketType::kPlayerLose, lp);
        for (auto &pl : players_)
          pl.conn->queue_data(pktLose.Data());
      }

      if (tick_counter % kUpdateFrequencyTicks == 0) {
        SendUpdatesToClients();
      }

      ++tick_counter;
      next_tick_time += kTickDuration;

      if (auto sleep_time = next_tick_time - clock::now();
          sleep_time > std::chrono::milliseconds(0)) {
        std::this_thread::sleep_for(sleep_time);
      } else {
        std::cerr << "[GameServer] Tick overrun by " << -sleep_time.count()
                  << " ms" << std::endl;
        next_tick_time = clock::now();
      }
    }
  }

private:
  uint16_t port_;
  std::unique_ptr<TcpServer> tcp_server_;
  std::vector<pollfd> pollfds_;
  std::vector<PlayerInfo<PacketType>> players_;
  network::PacketDispatcher<PacketType> dispatcher_;
  GameLogic game_logic_;
  GameState game_state_;
  uint32_t next_player_id_{0};
  std::string mapPath_;
  float tileSize_;
  Vec2 spawnPosition_{};

  std::unordered_set<uint32_t> readyPlayers_;

  void broadcastMapData() {
    const auto &coins = game_state_.map.getCoins();
    constexpr size_t MaxCoins = 512;
    std::array<MapCoin, MaxCoins> coinArr{};
    const size_t coinCount = std::min(coins.size(), coinArr.size());
    for (size_t i = 0; i < coinCount; ++i)
      coinArr[i] = {coins[i].id, coins[i].pos};

    auto pktCoins = network::PacketFactory<PacketType>::CreatePacket(
        PacketType::kMapCoins, std::span(coinArr.data(), coinCount));
    for (auto &pl : players_)
      pl.conn->queue_data(pktCoins.Data());

    const auto &segs = game_state_.map.getZapperSegments();
    constexpr size_t MaxSegs = 512;
    std::array<MapZapperSegment, MaxSegs> segArr{};
    const size_t segCount = std::min(segs.size(), segArr.size());
    for (size_t i = 0; i < segCount; ++i)
      segArr[i] = {segs[i].id, segs[i].a, segs[i].b};

    auto pktZaps = network::PacketFactory<PacketType>::CreatePacket(
        PacketType::kMapZappers, std::span(segArr.data(), segCount));
    for (auto &pl : players_)
      pl.conn->queue_data(pktZaps.Data());
  }

  void resetGame() {
    game_state_.map.loadFromFile(mapPath_, tileSize_);
    game_logic_.Reset();

    for (auto &[pid, pd] : game_state_.players) {
      pd = PlayerData{};
      pd.position = spawnPosition_;
    }

    game_state_.started = false;

    {
      const auto &coins = game_state_.map.getCoins();
      constexpr size_t MaxCoins = 512;
      std::array<MapCoin, MaxCoins> arr{};
      size_t count = std::min(coins.size(), arr.size());
      for (size_t i = 0; i < count; ++i)
        arr[i] = {coins[i].id, coins[i].pos};

      auto pkt = network::PacketFactory<PacketType>::CreatePacket(
          PacketType::kMapCoins, std::span(arr.data(), count));
      for (auto &pl : players_)
        pl.conn->queue_data(pkt.Data());
    }

    {
      const auto &segs = game_state_.map.getZapperSegments();
      constexpr size_t MaxSegs = 512;
      std::array<MapZapperSegment, MaxSegs> arr{};
      size_t count = std::min(segs.size(), arr.size());
      for (size_t i = 0; i < count; ++i)
        arr[i] = {segs[i].id, segs[i].a, segs[i].b};

      auto pkt = network::PacketFactory<PacketType>::CreatePacket(
          PacketType::kMapZappers, std::span(arr.data(), count));
      for (auto &pl : players_)
        pl.conn->queue_data(pkt.Data());
    }
  }

  void add_player(uint32_t id,
                  std::shared_ptr<network::TcpServerConnection<PacketType>> conn) {
    conn->set_player_id(id);
    players_.push_back({id, conn});
    game_state_.players[id] = PlayerData{};
    pollfds_.push_back({conn->get_sockfd(), POLLIN, 0});
  }

  void remove_player(size_t index) {
    const int fd = players_[index].conn->get_sockfd();
    ::close(fd);

    size_t last = players_.size() - 1;
    if (index != last) {
      std::swap(players_[index], players_.back());
      std::swap(pollfds_[index + 1], pollfds_.back());
    }
    game_state_.players.erase(players_.back().id);
    players_.pop_back();
    pollfds_.pop_back();

    broadcastReadyCount();
  }

  void update_pollfds() {
    for (size_t i = 0; i < players_.size(); ++i) {
      auto &pfd = pollfds_[i + 1];
      if (auto &info = players_[i]; info.conn->hasPendingSendData())
        pfd.events = POLLIN | POLLOUT;
      else
        pfd.events = POLLIN;
    }
  }

  void process_poll_events() {
    int ret = ::poll(pollfds_.data(), pollfds_.size(), 5);

    if (ret < 0) {
      std::cerr << "Poll error: " << std::strerror(errno) << std::endl;
      return;
    }
    if (ret == 0)
      return;

    if (pollfds_[0].revents & POLLIN) {
      handle_new_connection();
      std::cout << "New connection accepted." << std::endl;
      ret--;
    }

    for (size_t i = pollfds_.size() - 1; i > 0 && ret > 0; --i) {
      const auto &pfd = pollfds_[i];
      if (pfd.revents == 0)
        continue;
      ret--;

      size_t idx = i - 1;
      auto &info = players_[idx];

      if (pfd.revents & (POLLERR | POLLHUP)) {
        std::cerr << "Client fd=" << pfd.fd << " error/hangup. Removing.\n";
        remove_player(idx);
        continue;
      }

      if (pfd.revents & POLLOUT) {
        if (!info.conn->send_data()) {
          std::cerr << "Send error on fd=" << pfd.fd << ". Removing.\n";
          remove_player(idx);
          continue;
        }
      }

      if (pfd.revents & POLLIN) {
        auto maybePackets = info.conn->read_packets();
        if (!maybePackets.has_value())
          continue;
        for (auto &pkt : maybePackets.value()) {
          network::OwnedPacketTCP<PacketType> owned{info.conn, std::move(pkt)};
          dispatcher_.Dispatch(std::move(owned));
        }
      }
    }
  }

  void handle_new_connection() {
    try {
      auto sock = tcp_server_->acceptConnection();
      auto conn = std::make_shared<network::TcpServerConnection<PacketType>>(
          std::move(sock));
      uint32_t id = next_player_id_++;
      add_player(id, conn);

      {
        auto pkt = network::PacketFactory<PacketType>::CreatePacket(
            PacketType::kAssignPlayerId, id);
        conn->queue_data(pkt.Data());
      }

      broadcastReadyCount();

      std::cout << "Player " << id << " connected\n";
    } catch (const std::exception &e) {
      std::cerr << "Accept error: " << e.what() << std::endl;
    }
  }

  void broadcastReadyCount() {
    PlayerReadyCountPacket pr{};
    pr.ready_count = 0;
    for (auto const &[pid, pd] : game_state_.players)
      if (pd.ready)
        ++pr.ready_count;
    pr.total_count = game_state_.players.size();
    auto pkt = network::PacketFactory<PacketType>::CreatePacket(
        PacketType::kPlayerReadyCount, pr);
    for (auto &pl : players_)
      pl.conn->queue_data(pkt.Data());
  }

  void tryStartGame() {
    const auto total = game_state_.players.size();
    if (total < 2)
      return;

    size_t ready = 0;
    for (auto const &[pid, pd] : game_state_.players)
      if (pd.ready)
        ++ready;

    if (ready == total && !game_state_.started) {
      resetGame();
      game_state_.started = true;
      std::cout << "All players ready — starting game\n";

      GameStartPacket gsp{};
      auto pktStart = network::PacketFactory<PacketType>::CreatePacket(
          PacketType::kGameStart, gsp);
      for (auto &pl : players_)
        pl.conn->queue_data(pktStart.Data());

      broadcastMapData();

      for (auto & [pid, pd] : game_state_.players) {
        pd.ready = false;
      }
      broadcastReadyCount();
    }
  }

  void SendUpdatesToClients() { SendPlayerUpdates(); }

  [[nodiscard]] std::pair<float, float> getPlayerPosition(uint32_t id) const {
    const auto &pd = game_state_.players.at(id);
    return {pd.position.x, pd.position.y};
  }

  void SendPlayerUpdates() {
    std::array<UpdatePlayer, 1024> updates{};
    size_t cnt = std::min(players_.size(), updates.size());
    for (size_t i = 0; i < cnt; ++i) {
      const auto &info = players_[i];
      auto [x, y] = getPlayerPosition(info.id);
      updates[i] = {info.id, x, y};
    }
    SendUpdatePacket(updates, cnt, PacketType::kUpdatePlayers);
  }

  template <typename T, std::size_t MaxUpdates>
  void SendUpdatePacket(const std::array<T, MaxUpdates> &updates, size_t count,
                        PacketType type) {
    auto update_packet = network::PacketFactory<PacketType>::CreatePacket(
        type, std::span(updates.data(), count));

    for (auto &info : players_) {
      info.conn->queue_data(update_packet.Data());
    }
  }
};

} // namespace server

#endif // GAME_SERVER_HPP_
