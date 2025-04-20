#ifndef PACKET_DISPATCHER_HPP_
#define PACKET_DISPATCHER_HPP_

#include <array>
#include <cstddef>
#include <functional>
#include <iostream>
#include <memory>

#include <common/packet_structs.hpp>
#include <network/protocol.hpp>
#include <network/tcp_server_connection.hpp>

namespace network {

template <typename PacketType> class PacketDispatcher {
public:
  using TCPHandler = std::function<void(
      Packet<PacketType> &&,
      const std::shared_ptr<TcpServerConnection<PacketType>> &)>;

  PacketDispatcher() {
    handlers_.fill(nullptr);

    handlers_[static_cast<size_t>(PacketType::kPing)] = PingHandler;
    handlers_[static_cast<size_t>(PacketType::kPong)] = nullptr;
  }
  ~PacketDispatcher() = default;

  void Dispatch(OwnedPacketTCP<PacketType> &&owned_packet) {
    HandlePacket(std::move(owned_packet));
  }

  void RegisterHandler(PacketType t, TCPHandler h) {
    handlers_[static_cast<size_t>(t)] = std::move(h);
  }

private:
  void HandlePacket(OwnedPacketTCP<PacketType> &&packet_variant) const {
    auto &packet = packet_variant.packet;
    const auto &connection = packet_variant.connection;
    const size_t index = static_cast<size_t>(packet.header.type);
    if (index < handlers_.size() && handlers_[index] != nullptr) {
      handlers_[index](std::move(packet), connection);
    } else {
      DefaultTCPHandler(std::move(packet), connection);
    }
  }

  static void DefaultTCPHandler(
      Packet<PacketType> &&packet,
      const std::shared_ptr<TcpServerConnection<PacketType>> &connection) {
    std::cerr << "Warning: No handler registered for packet type "
              << static_cast<size_t>(packet.header.type) << std::endl;
    (void)packet;
    (void)connection;
  }

  static void PingHandler(
      Packet<PacketType> &&packet,
      const std::shared_ptr<TcpServerConnection<PacketType>> &connection) {
    if (packet.body.size() >= sizeof(std::uint32_t)) {
      std::uint32_t pingTimestamp;
      std::memcpy(&pingTimestamp, packet.body.data(), sizeof(std::uint32_t));

      auto pongPacket = PacketFactory<PacketType>::CreatePacket(
          PacketType::kPong, pingTimestamp);

      connection->queue_data(pongPacket.Data());
      connection->send_data();

      //std::cout << "Ping received. Responded with Pong, timestamp: "
      //          << pingTimestamp << std::endl;
    } else {
      std::cerr << "Received an invalid PING packet." << std::endl;
    }
  }

  inline static std::array<TCPHandler,
                           static_cast<size_t>(PacketType::kMaxTypes)>
      handlers_;
};

} // namespace network

#endif // PACKET_DISPATCHER_HPP_


/*
template<typename PacketType>
inline std::array<
  typename PacketDispatcher<PacketType>::TCPHandler,
  static_cast<size_t>(PacketType::kMaxTypes)
> PacketDispatcher<PacketType>::handlers_ = {
  PacketDispatcher<PacketType>::PingHandler,
  nullptr,   // Pong
  PacketDispatcher<PacketType>::InputHandler,
  // …
};

*/