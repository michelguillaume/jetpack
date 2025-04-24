#ifndef GAME_LOGIC_HPP_
#define GAME_LOGIC_HPP_

#include "common/packet_structs.hpp"
#include "common/player_actions.hpp"
#include "game_state.hpp"
#include <algorithm>
#include <cstdint>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace server {

class GameLogic {
public:
  GameLogic()
    : gravity_(980.f),
      accelThrust_(3500.f),
      fallingThrustMultiplier_(1.6f),
      fallMultiplier_(2.5f),
      lowJumpMultiplier_(2.0f),
      horizontalSpeed_(100.f),
      maxFallSpeed_(800.f),
      maxUpSpeed_(600.f),
      groundY_(255.f),
      ceilingY_(85.f)
  {}

  std::tuple<
    std::vector<CoinCollectedPacket>,
    std::vector<ZapperCollisionPacket>,
    std::vector<PlayerDeathPacket>,
    std::vector<CoinExpiredPacket>,
    std::vector<PlayerWinPacket>,
    std::vector<PlayerLosePacket>
  > Update(const float dt, GameState &state) {
    if (!state.started) {
      return {{}, {}, {}, {}, {}, {}};
    }

    std::vector<CoinCollectedPacket>   coins;
    std::vector<ZapperCollisionPacket> zaps;
    std::vector<PlayerDeathPacket>     deaths;
    std::vector<CoinExpiredPacket>     expired;
    std::vector<PlayerWinPacket>       wins;
    std::vector<PlayerLosePacket>      loses;

    for (auto &[pid, pd] : state.players) {
      if (!pd.alive)
        continue;

      const bool jet = (pd.lastInput.actions &
                  static_cast<uint16_t>(PlayerAction::ActivateJetpack)) != 0;
      const bool fall = pd.velocity.y > 0;

      pd.position.x += horizontalSpeed_ * dt;

      if (jet) {
        const float thrust = accelThrust_ * (fall ? fallingThrustMultiplier_ : 1.f);
        pd.velocity.y -= thrust * dt;
      }

      float g = gravity_;
      if (fall)
        g *= fallMultiplier_;
      else if (!jet && pd.velocity.y < 0.f)
        g *= lowJumpMultiplier_;

      pd.velocity.y += g * dt;
      pd.velocity.y = std::clamp(pd.velocity.y, -maxUpSpeed_, maxFallSpeed_);
      pd.position.y += pd.velocity.y * dt;

      if (pd.position.y >= groundY_) {
        pd.position.y = groundY_;
        pd.velocity.y = 0.f;
        pd.onGround = true;
      } else {
        pd.onGround = false;
      }
      if (pd.position.y <= ceilingY_) {
        pd.position.y = ceilingY_;
        pd.velocity.y = 0.f;
      }
    }

    constexpr float rsum = (10.f + 10.f) * (10.f + 10.f);
    for (auto & [pid, pd] : state.players) {
      if (!pd.alive) continue;
      auto &col = collectedByPlayer_[pid];
      for (const auto &[id, pos] : state.map.getCoins()) {
        if (col.contains(id))
          continue;
        const float dx = pd.position.x - pos.x;
        const float dy = pd.position.y - pos.y;
        if (dx*dx + dy*dy <= rsum) {
          coins.push_back({ pid, id });
          col.insert(id);
        }
      }
    }

    for (const auto &[id, pos] : state.map.getCoins()) {
      bool all = true;
      for (auto const & [pid, pd] : state.players) {
        if (!collectedByPlayer_[pid].contains(id)) {
          all = false;
          break;
        }
      }
      if (all) expired.push_back({ id });
    }

    for (auto & [pid, pd] : state.players) {
      if (!pd.alive) continue;
      for (const auto &[id, a, b] : state.map.getZapperSegments()) {
        const float minx = std::min(a.x, b.x) - 10.f;
        const float maxx = std::max(a.x, b.x) + 10.f;
        const float miny = std::min(a.y, b.y) - 10.f;
        const float maxy = std::max(a.y, b.y) + 10.f;
        if (pd.position.x >= minx && pd.position.x <= maxx &&
            pd.position.y >= miny && pd.position.y <= maxy) {
          zaps.push_back({ pid, id });
          deaths.push_back({ pid });
          state.players[pid].alive = false;
          break;
        }
      }
    }

    std::vector<uint32_t> finishers;
    for (auto & [pid, pd] : state.players) {
      if (!pd.alive) continue;
      if (state.map.isFinishReached(pd.position)) {
        finishers.push_back(pid);
      }
    }
    if (!finishers.empty()) {
      uint32_t best      = finishers[0];
      auto bestCount = static_cast<uint32_t>(collectedByPlayer_[best].size());
      for (auto pid : finishers) {
        if (const auto cnt = static_cast<uint32_t>(collectedByPlayer_[pid].size());
            cnt > bestCount) {
          best      = pid;
          bestCount = cnt;
        }
      }
      wins.push_back({ best, bestCount });
      state.players[best].alive = false;

      for (auto pid : finishers) {
        if (pid == best)
          continue;
        const auto cnt = static_cast<uint32_t>(collectedByPlayer_[pid].size());
        loses.push_back({ pid, cnt });
        state.players[pid].alive = false;
      }
    }

    {
      std::vector<uint32_t> surv;
      for (auto const & [pid, pd] : state.players)
        if (pd.alive) surv.push_back(pid);

      if (surv.size() == 1) {
        const uint32_t last = surv[0];
        const auto count = static_cast<uint32_t>(collectedByPlayer_[last].size());
        wins.push_back({ last, count });
        state.players[last].alive = false;
      }
    }

    bool anyAlive = false;
    for (auto const & [_, pd] : state.players) {
      if (pd.alive) { anyAlive = true; break; }
    }
    if (!anyAlive) {
      state.started = false;
    }

    return { coins, zaps, deaths, expired, wins, loses };
  }

  [[nodiscard]] uint32_t GetCollectedCount(const uint32_t pid) const {
    const auto it = collectedByPlayer_.find(pid);
    return it == collectedByPlayer_.end()
         ? 0u
         : static_cast<uint32_t>(it->second.size());
  }

  void Reset() {
    collectedByPlayer_.clear();
  }

private:
  float gravity_;
  float accelThrust_;
  float fallingThrustMultiplier_;
  float fallMultiplier_;
  float lowJumpMultiplier_;
  float horizontalSpeed_;
  float maxFallSpeed_;
  float maxUpSpeed_;
  float groundY_;
  float ceilingY_;

  std::unordered_map<uint32_t, std::unordered_set<uint32_t>> collectedByPlayer_;
};

} // namespace server

#endif // GAME_LOGIC_HPP_
