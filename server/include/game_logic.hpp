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
    std::vector<PlayerWinPacket>
  > Update(float dt, GameState &state) {
    if (!state.started) {
      return {{}, {}, {}, {}, {}};
    }

    std::vector<CoinCollectedPacket> coins;
    std::vector<ZapperCollisionPacket> zaps;
    std::vector<PlayerDeathPacket> deaths;
    std::vector<CoinExpiredPacket> expired;
    std::vector<PlayerWinPacket> wins;

    for (auto & [pid, pd] : state.players) {
      if (!pd.alive) continue;
      bool jet = (pd.lastInput.actions & static_cast<uint16_t>(PlayerAction::ActivateJetpack)) != 0;
      bool fall = pd.velocity.y > 0;
      pd.position.x += horizontalSpeed_ * dt;
      if (jet) {
        float thrust = accelThrust_ * (fall ? fallingThrustMultiplier_ : 1.f);
        pd.velocity.y -= thrust * dt;
      }
      float g = gravity_;
      if (fall) g *= fallMultiplier_;
      else if (!jet && pd.velocity.y < 0) g *= lowJumpMultiplier_;
      pd.velocity.y += g * dt;
      pd.velocity.y = std::clamp(pd.velocity.y, -maxUpSpeed_, maxFallSpeed_);
      pd.position.y += pd.velocity.y * dt;
      if (pd.position.y >= groundY_) {
        pd.position.y = groundY_;
        pd.velocity.y = 0;
        pd.onGround = true;
      } else {
        pd.onGround = false;
      }
      if (pd.position.y <= ceilingY_) {
        pd.position.y = ceilingY_;
        pd.velocity.y = 0;
      }
    }

    const float rsum = (10.f + 10.f) * (10.f + 10.f);
    for (auto & [pid, pd] : state.players) {
      if (!pd.alive) continue;
      auto &col = collectedByPlayer_[pid];
      for (auto &c : state.map.getCoins()) {
        if (col.count(c.id)) continue;
        float dx = pd.position.x - c.pos.x;
        float dy = pd.position.y - c.pos.y;
        if (dx*dx + dy*dy <= rsum) {
          coins.push_back({pid, c.id});
          col.insert(c.id);
        }
      }
    }

    for (auto &c : state.map.getCoins()) {
      bool all = true;
      for (auto const & [pid, pd] : state.players) {
        if (!collectedByPlayer_[pid].count(c.id)) {
          all = false;
          break;
        }
      }
      if (all) expired.push_back({c.id});
    }

    for (auto & [pid, pd] : state.players) {
      if (!pd.alive) continue;
      for (auto &zs : state.map.getZapperSegments()) {
        float minx = std::min(zs.a.x, zs.b.x) - 10;
        float maxx = std::max(zs.a.x, zs.b.x) + 10;
        float miny = std::min(zs.a.y, zs.b.y) - 10;
        float maxy = std::max(zs.a.y, zs.b.y) + 10;
        if (pd.position.x >= minx && pd.position.x <= maxx &&
            pd.position.y >= miny && pd.position.y <= maxy) {
          zaps.push_back({pid, zs.id});
          deaths.push_back({pid});
          state.players[pid].alive = false;
          break;
        }
      }
    }

    for (auto & [pid, pd] : state.players) {
      if (!pd.alive) continue;
      if (state.map.isFinishReached(pd.position)) {
        uint32_t count = static_cast<uint32_t>(collectedByPlayer_[pid].size());
        wins.push_back({pid, count});
        state.players[pid].alive = false;
      }
    }

    {
      std::vector<uint32_t> surv;
      for (auto const & [pid, pd] : state.players)
        if (pd.alive) surv.push_back(pid);
      if (surv.size() == 1) {
        uint32_t last = surv[0];
        uint32_t count = static_cast<uint32_t>(collectedByPlayer_[last].size());
        wins.push_back({last, count});
        state.players[last].alive = false;
      }
    }

    bool any = false;
    for (auto const & [_, pd] : state.players) {
      if (pd.alive) { any = true; break; }
    }
    if (!any) state.started = false;

    return {coins, zaps, deaths, expired, wins};
  }

  [[nodiscard]] uint32_t GetCollectedCount(uint32_t pid) const {
    auto it = collectedByPlayer_.find(pid);
    return it == collectedByPlayer_.end() ? 0u : static_cast<uint32_t>(it->second.size());
  }

  void Reset() {
    collectedByPlayer_.clear();
  }

private:
  float gravity_, accelThrust_, fallingThrustMultiplier_;
  float fallMultiplier_, lowJumpMultiplier_;
  float horizontalSpeed_, maxFallSpeed_, maxUpSpeed_;
  float groundY_, ceilingY_;

  std::unordered_map<uint32_t, std::unordered_set<uint32_t>> collectedByPlayer_;
};

} // namespace server

#endif // GAME_LOGIC_HPP_
