#ifndef GAME_LOGIC_HPP_
#define GAME_LOGIC_HPP_

#include "common/packet_structs.hpp"
#include "common/player_actions.hpp"
#include "game_state.hpp"
#include <algorithm>
#include <cstdint>
#include <unordered_set>
#include <vector>

namespace server {

class GameLogic {
public:
  GameLogic()
      : gravity_(980.f), accelThrust_(3500.f), fallingThrustMultiplier_(1.6f),
        fallMultiplier_(2.5f), lowJumpMultiplier_(2.0f),
        horizontalSpeed_(100.f), maxFallSpeed_(800.f), maxUpSpeed_(600.f),
        groundY_(255.f), ceilingY_(85.f) {}

  std::tuple<std::vector<CoinCollectedPacket>,
             std::vector<ZapperCollisionPacket>, std::vector<PlayerDeathPacket>,
             std::vector<CoinExpiredPacket>>
  Update(float dt, GameState &state) {
    if (!state.started)
      return {{}, {}, {}, {}};

    std::vector<CoinCollectedPacket> coins;
    std::vector<ZapperCollisionPacket> zaps;
    std::vector<PlayerDeathPacket> deaths;
    std::vector<CoinExpiredPacket> expired;

    for (auto &[id, data] : state.players) {
      if (!data.alive)
        continue;
      bool jetpackOn =
          (data.lastInput.actions &
           static_cast<uint16_t>(PlayerAction::ActivateJetpack)) != 0;
      bool isFalling = data.velocity.y > 0.f;
      data.position.x += horizontalSpeed_ * dt;
      if (jetpackOn) {
        float thrust =
            accelThrust_ * (isFalling ? fallingThrustMultiplier_ : 1.f);
        data.velocity.y -= thrust * dt;
      }
      float g = gravity_;
      if (isFalling)
        g *= fallMultiplier_;
      else if (!jetpackOn && data.velocity.y < 0.f)
        g *= lowJumpMultiplier_;
      data.velocity.y += g * dt;
      data.velocity.y =
          std::clamp(data.velocity.y, -maxUpSpeed_, maxFallSpeed_);
      data.position.y += data.velocity.y * dt;
      if (data.position.y >= groundY_) {
        data.position.y = groundY_;
        data.velocity.y = 0.f;
        data.onGround = true;
      } else
        data.onGround = false;
      if (data.position.y <= ceilingY_) {
        data.position.y = ceilingY_;
        data.velocity.y = 0.f;
      }
    }

    const float coinR = 10.f, playerR = 10.f;
    const float sumR2 = (coinR + playerR) * (coinR + playerR);
    for (auto &[pid, pd] : state.players) {
      if (!pd.alive)
        continue;
      auto &colSet = collectedByPlayer_[pid];
      for (auto &coin : state.map.getCoins()) {
        if (colSet.count(coin.id))
          continue;
        float dx = pd.position.x - coin.pos.x;
        float dy = pd.position.y - coin.pos.y;
        if (dx * dx + dy * dy <= sumR2) {
          coins.push_back({pid, coin.id});
          colSet.insert(coin.id);
        }
      }
    }

    for (auto &coin : state.map.getCoins()) {
      bool allGot = true;
      for (auto &[pid, _] : state.players) {
        if (!collectedByPlayer_[pid].count(coin.id)) {
          allGot = false;
          break;
        }
      }
      if (allGot)
        expired.push_back({coin.id});
    }

    for (auto &[pid, pd] : state.players) {
      if (!pd.alive)
        continue;
      for (auto &seg : state.map.getZapperSegments()) {
        if (pd.position.x >= std::min(seg.a.x, seg.b.x) - playerR &&
            pd.position.x <= std::max(seg.a.x, seg.b.x) + playerR &&
            pd.position.y >= std::min(seg.a.y, seg.b.y) - playerR &&
            pd.position.y <= std::max(seg.a.y, seg.b.y) + playerR) {
          zaps.push_back({pid, seg.id});
          deaths.push_back({pid});
          state.players[pid].alive = false;
          break;
        }
      }
    }

    bool anyAlive = false;
    for (auto const &[pid, pd] : state.players) {
      if (pd.alive) { anyAlive = true; break; }
    }
    if (!anyAlive) {
      state.started = false;
    }

    return {coins, zaps, deaths, expired};
  }

  uint32_t GetCollectedCount(const uint32_t pid) const {
    const auto it = collectedByPlayer_.find(pid);
    return it == collectedByPlayer_.end()
               ? 0u
               : static_cast<uint32_t>(it->second.size());
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
