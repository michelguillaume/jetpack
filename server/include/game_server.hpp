#ifndef GAME_SERVER_HPP_
#define GAME_SERVER_HPP_

#include <chrono>
#include <cstring>
#include <iostream>
#include <memory>
#include <poll.h>
#include <stdexcept>
#include <thread>
#include <vector>

#include "network/protocol.hpp"
#include "network/tcp_server.hpp"
#include "network/tcp_server_connection.hpp"
#include "network/tcp_socket.hpp"
#include "packet_dispatcher.hpp"

namespace server {

template <typename PacketType> class GameServer {
public:
  explicit GameServer(const uint16_t port) : port_(port), game_state_(0) {
    tcp_server_ = std::make_unique<TcpServer>(port_);
    pollfds_.push_back({tcp_server_->getSocket().get(), POLLIN, 0});
    std::cout << "Server listening on port " << port_ << std::endl;
  }

  ~GameServer() = default;

  void
  add_player(std::shared_ptr<network::TcpServerConnection<PacketType>> player) {
    int fd = player->get_sockfd();
    players_.push_back(player);
    pollfds_.push_back({fd, POLLIN, 0});
  }

  void remove_player(size_t index) {
    const int fd = players_[index]->get_sockfd();
    ::close(fd);
    size_t last_index = players_.size() - 1;
    if (index != last_index) {
      std::swap(players_[index], players_.back());
      std::swap(pollfds_[index + 1], pollfds_.back());
    }
    players_.pop_back();
    pollfds_.pop_back();
  }

  auto& getDispatcher() {
    return dispatcher_;
  }

  [[noreturn]] void Run() {
    using clock = std::chrono::steady_clock;
    constexpr auto kTickDuration =
        std::chrono::milliseconds(16); // ~16 ms per tick (~60 fps)
    uint64_t tick_counter = 0;
    auto previous_time = clock::now();
    auto next_tick_time = previous_time;

    while (true) {
      auto current_time = clock::now();
      float delta_time =
          std::chrono::duration<float>(current_time - previous_time).count();

      previous_time = current_time;

      update_pollfds();
      process_poll_events();

      // Update game logic.
      //      game_logic_.Update(delta_time, game_state_);

      // Optionally: handle connection checks, send updates, etc.
      // (Not implemented in this minimal example.)

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
  std::vector<std::shared_ptr<network::TcpServerConnection<PacketType>>>
      players_;
  network::PacketDispatcher<PacketType> dispatcher_;
  // game::GameLogic<PacketType> game_logic_;
  int game_state_;

  void update_pollfds() {
    for (size_t i = 0; i < players_.size(); ++i) {
      if (players_[i]->hasPendingSendData())
        pollfds_[i + 1].events = POLLIN | POLLOUT;
      else
        pollfds_[i + 1].events = POLLIN;
    }
  }

  void process_poll_events() {
    int ret = ::poll(pollfds_.data(), pollfds_.size(), 5);
    if (ret < 0) {
      std::cerr << "Poll error: " << std::strerror(errno) << std::endl;
      return;
    }
    if (ret > 0) {
      if (pollfds_[0].revents & POLLIN) {
        handle_new_connection();
        std::cout << "New connection accepted." << std::endl;
        ret--;
      }
      for (size_t i = pollfds_.size() - 1; i > 0 && ret > 0; --i) {
        if (pollfds_[i].revents == 0)
          continue;
        ret--;
        if (pollfds_[i].revents & (POLLERR | POLLHUP)) {
          std::cerr << "Error on client socket (fd: " << pollfds_[i].fd
                    << "). Removing connection." << std::endl;
          remove_player(i - 1);
          continue;
        }
        if (pollfds_[i].revents & POLLOUT) {
          auto client_conn = players_[i - 1];
          if (!client_conn->send_data()) {
            std::cerr << "Error sending data on socket (fd: " << pollfds_[i].fd
                      << "). Removing connection." << std::endl;
            remove_player(i - 1);
            continue;
          }
        }
        if (pollfds_[i].revents & POLLIN) {
          auto client_conn = players_[i - 1];
          auto maybePackets = client_conn->read_packets();
          if (!maybePackets.has_value())
            continue;
          for (auto &pkt : maybePackets.value()) {
            network::OwnedPacketTCP<PacketType> ownedPacket{client_conn,
                                                            std::move(pkt)};
            dispatcher_.Dispatch(std::move(ownedPacket));
          }
        }
      }
    }
  }

  void handle_new_connection() {
    try {
      TcpSocket client_socket = tcp_server_->acceptConnection();
      auto client_conn = std::make_shared<network::TcpServerConnection<PacketType>>(std::move(client_socket));
      add_player(client_conn);
    } catch (const std::exception &e) {
      std::cerr << "Error accepting new connection: " << e.what() << std::endl;
    }
  }
};

} // namespace server

#endif // GAME_SERVER_HPP_
