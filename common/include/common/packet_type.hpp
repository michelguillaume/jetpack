#ifndef PACKET_TYPE_HPP_
#define PACKET_TYPE_HPP_

#include <cstdint>

enum class PacketType : uint32_t {
  kPing = 0,
  kPong,
  kPlayerInput,
  kAssignPlayerId,
  kUpdatePlayers,
  kMapCoins,
  kCoinCollected,
  kMapZappers,
  kZapperCollision,
  kPlayerDeath,
  kCoinExpired,
  kPlayerReady,
  kPlayerReadyCount,
  kGameStart,
  kPlayerScore,
  kData,
  kMaxTypes
};


#endif  // PACKET_TYPE_HPP_
