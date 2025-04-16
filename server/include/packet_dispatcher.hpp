#ifndef PACKET_DISPATCHER_HPP_
#define PACKET_DISPATCHER_HPP_

#include <array>
#include <cstddef>
#include <memory>
#include <iostream>

#include <network/protocol.hpp>
#include <network/tcp_server_connection.hpp>

namespace network {

/**
 * @brief PacketDispatcher routes incoming TCP packets to the appropriate handler based on packet type.
 *
 * @tparam PacketType Enum type representing the different packet types. Must define PacketType::kMaxTypes.
 */
template <typename PacketType>
class PacketDispatcher {
public:
  /**
   * @brief Function signature for handling TCP packets.
   *
   * Each handler receives a Packet object and the associated TCP connection.
   */
  using TCPHandler = void (*)(Packet<PacketType>&&, const std::shared_ptr<TcpServerConnection<PacketType>>&);

  PacketDispatcher() = default;
  ~PacketDispatcher() = default;

  /**
   * @brief Dispatches an owned TCP packet to its corresponding handler.
   *
   * If no handler is registered for the packet type, the default handler is invoked.
   *
   * @param owned_packet The TCP packet (with its associated connection) to dispatch.
   */
  void Dispatch(OwnedPacketTCP<PacketType>&& owned_packet) {
    HandlePacket(std::move(owned_packet));
  }

  /**
   * @brief Registers a handler for a specific packet type.
   *
   * @param type The packet type.
   * @param handler Pointer to the function that will handle packets of this type.
   */
  void RegisterHandler(PacketType type, TCPHandler handler) {
    const size_t index = static_cast<size_t>(type);
    if (index < handlers_.size()) {
      handlers_[index] = handler;
    }
  }

private:
  /**
   * @brief Calls the handler corresponding to the packet's type.
   *
   * If no handler is registered, DefaultTCPHandler is called.
   *
   * @param packet_variant An OwnedPacketTCP containing the packet and its connection.
   */
  void HandlePacket(OwnedPacketTCP<PacketType>&& packet_variant) const {
    auto& packet = packet_variant.packet;
    const auto& connection = packet_variant.connection;
    const size_t index = static_cast<size_t>(packet.header.type);
    if (index < handlers_.size() && handlers_[index] != nullptr) {
      handlers_[index](std::move(packet), connection);
    } else {
      DefaultTCPHandler(std::move(packet), connection);
    }
  }

  /**
   * @brief Default handler called when no specific handler is registered for a packet type.
   *
   * Logs a warning message indicating that no handler is registered for the given packet type.
   *
   * @param packet The unhandled TCP packet.
   * @param connection The associated TCP connection.
   */
  static void DefaultTCPHandler(Packet<PacketType>&& packet,
                                const std::shared_ptr<TcpServerConnection<PacketType>>& connection) {
    std::cerr << "Warning: No handler registered for packet type "
              << static_cast<size_t>(packet.header.type) << std::endl;
    (void)packet;
    (void)connection;
  }

  static void PingHandler(Packet<PacketType>&& packet,
                        const std::shared_ptr<TcpServerConnection<PacketType>>& connection) {
    if (packet.body.size() >= sizeof(std::uint32_t)) {
      std::uint32_t pingTimestamp;
      std::memcpy(&pingTimestamp, packet.body.data(), sizeof(std::uint32_t));

      // Use PacketFactory to create a PONG packet with the same timestamp.
      auto pongPacket = PacketFactory<PacketType>::CreatePacket(PacketType::kPong, pingTimestamp);

      // Queue and send the PONG packet.
      connection->queue_data(pongPacket.Data());
      connection->send_data();

      std::cout << "Ping received. Responded with Pong, timestamp: " << pingTimestamp << std::endl;
    } else {
      std::cerr << "Received an invalid PING packet." << std::endl;
    }
  }

  static void InputHandler(Packet<PacketType>&& packet,
                           const std::shared_ptr<TcpServerConnection<PacketType>>& connection) {
    struct PlayerInputData {
      uint16_t actions;
      float dir_x;
      float dir_y;
    };

    auto maybeInput = PacketFactory<PacketType>::template ExtractData<PlayerInputData>(packet);
    if (maybeInput.has_value()) {
      const auto& inputData = maybeInput.value();
      std::cout << "Received PlayerInput: actions=" << inputData.actions
                << ", dir_x=" << inputData.dir_x
                << ", dir_y=" << inputData.dir_y << std::endl;
    } else {
      std::cerr << "Received an invalid PlayerInput packet." << std::endl;
    }
    (void)connection;
  }

  inline static std::array<TCPHandler, static_cast<size_t>(PacketType::kMaxTypes)> handlers_ = {
    PingHandler,
    nullptr,
    InputHandler,
  };
};

}  // namespace network

#endif  // PACKET_DISPATCHER_HPP_
