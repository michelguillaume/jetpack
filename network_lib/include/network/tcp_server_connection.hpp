#ifndef NETWORK_TCP_SERVER_CONNECTION_HPP_
#define NETWORK_TCP_SERVER_CONNECTION_HPP_

#include "protocol.hpp"
#include "tcp_socket.hpp"
#include <optional>
#include <vector>
#include <unistd.h>
#include <cstring>
#include <iostream>

namespace network {

template <typename PacketType>
class TcpServerConnection {
public:
    explicit TcpServerConnection(TcpSocket socket) noexcept
      : socket_(std::move(socket)) { }
    ~TcpServerConnection() = default;

    [[nodiscard]] int get_sockfd() const noexcept { return socket_.get(); }

    [[nodiscard]] bool hasPendingSendData() const noexcept {
        return !send_buffer_.empty();
    }

    std::optional<std::vector<Packet<PacketType>>> read_packets() {
        std::vector<Packet<PacketType>> packets;
        uint8_t read_data[1024];
        ssize_t bytes_read = ::read(socket_.get(), read_data, sizeof(read_data));
        if (bytes_read > 0) {
            recv_buffer_.insert(recv_buffer_.end(), read_data, read_data + bytes_read);
        }
        while (auto packetOpt = extract_packet()) {
            packets.push_back(std::move(*packetOpt));
        }
        if (packets.empty())
            return std::nullopt;
        return packets;
    }

    bool send_data() {
        if (send_buffer_.empty())
            return true;
        ssize_t bytes_sent = ::write(socket_.get(), send_buffer_.data(), send_buffer_.size());
        if (bytes_sent < 0) {
            std::cerr << "Write error: " << std::strerror(errno) << std::endl;
            return false;
        }
        send_buffer_.erase(send_buffer_.begin(), send_buffer_.begin() + bytes_sent);
        return true;
    }

    void queue_data(const std::vector<uint8_t>& data) {
        send_buffer_.insert(send_buffer_.end(), data.begin(), data.end());
    }

    void queue_data(const uint8_t* data, size_t size) {
        send_buffer_.insert(send_buffer_.end(), data, data + size);
    }

private:
    std::optional<Packet<PacketType>> extract_packet() {
        if (recv_buffer_.size() < sizeof(Header<PacketType>))
            return std::nullopt;
        Header<PacketType> header;
        std::memcpy(&header, recv_buffer_.data(), sizeof(Header<PacketType>));
        size_t totalSize = sizeof(Header<PacketType>) + header.size;
        if (recv_buffer_.size() < totalSize)
            return std::nullopt;
        Packet<PacketType> packet;
        std::memcpy(&packet.header, recv_buffer_.data(), sizeof(Header<PacketType>));
        if (header.size > 0) {
            packet.body.resize(header.size);
            std::memcpy(packet.body.data(), recv_buffer_.data() + sizeof(Header<PacketType>), header.size);
        }
        recv_buffer_.erase(recv_buffer_.begin(), recv_buffer_.begin() + static_cast<std::ptrdiff_t>(totalSize));
        return packet;
    }

    TcpSocket socket_;
    std::vector<uint8_t> recv_buffer_;
    std::vector<uint8_t> send_buffer_;
};

} // namespace network

#endif // NETWORK_TCP_SERVER_CONNECTION_HPP_
