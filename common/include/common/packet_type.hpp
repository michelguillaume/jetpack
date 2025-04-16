#ifndef PACKET_TYPE_HPP_
#define PACKET_TYPE_HPP_

#include <cstdint>

/**
 * @brief Defines the different packet types for the application.
 */
enum class PacketType : uint32_t {
  kPing = 0,
  kPong,
  kPlayerInput,
  kData,
  kMaxTypes
};

#endif  // PACKET_TYPE_HPP_
