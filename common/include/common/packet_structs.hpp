#ifndef PACKET_STRUCTS_HPP_
#define PACKET_STRUCTS_HPP_

#include "vec2.hpp"

#include <cstdint>

#pragma pack(push, 1)

struct PingPacket {
    uint32_t timestamp_ms;
};

struct PongPacket {
    uint32_t timestamp_ms;
};

struct PlayerInputPacket {
    uint16_t actions;
    float dir_x;
    float dir_y;
};

struct AssignPlayerIdPacket {
    uint32_t player_id;
};

struct DataPacket {
    uint32_t game_state;
    float    player_x;
    float    player_y;
    uint32_t score;
};

struct UpdatePlayer {
    uint32_t player_id;
    float x;
    float y;
};

struct CoinCollectedPacket {
    uint32_t player_id;
    uint32_t coin_id;
};

struct MapCoin {
    uint32_t id;
    Vec2     pos;
};

struct MapZapperSegment {
    uint32_t id;
    Vec2     a;
    Vec2     b;
};

struct ZapperCollisionPacket {
    uint32_t player_id;
    uint32_t zapper_id;
};

struct PlayerDeathPacket {
    uint32_t player_id;
};

struct CoinExpiredPacket {
    uint32_t coin_id;
};

struct PlayerReadyCountPacket {
    uint32_t ready_count;
    uint32_t total_count;
};

struct PlayerReadyPacket {
    bool ready;
};

struct GameStartPacket {};

struct PlayerScorePacket {
    uint32_t player_id;
    uint32_t coins_collected;
};

struct PlayerWinPacket {
    uint32_t playerId;
    uint32_t coinsCollected;
};

#pragma pack(pop)

#endif // PACKET_STRUCTS_HPP_
