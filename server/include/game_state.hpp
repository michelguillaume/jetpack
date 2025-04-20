#ifndef GAME_STATE_HPP_
#define GAME_STATE_HPP_

#include <cstdint>
#include <unordered_map>
#include "common/vec2.hpp"
#include "common/packet_structs.hpp"
#include "game_map.hpp"

namespace server {

struct PlayerData {
  Vec2 position;
  Vec2 velocity;
  bool onGround {false};
  bool alive    {true};
  bool ready    {false};

  PlayerInputPacket lastInput{};
};

struct GameState {
  std::unordered_map<uint32_t, PlayerData> players;
  GameMap map;
  bool started {false};
};

} // namespace server

#endif // GAME_STATE_HPP_
