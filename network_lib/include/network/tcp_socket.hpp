#ifndef TCP_SOCKET_HPP_
#define TCP_SOCKET_HPP_

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdexcept>
#include <cstring>
#include <string>
#include <iostream>
#include <cerrno>

class SocketException final : public std::runtime_error {
public:
    explicit SocketException(const std::string &message)
        : std::runtime_error(message) {}
};

class TcpSocket {
public:
    TcpSocket() {
        sockfd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd_ < 0)
            throw SocketException("socket() failed: " + std::string(std::strerror(errno)));
    }

    explicit TcpSocket(const int fd) : sockfd_(fd) {
        if (sockfd_ < 0)
            throw SocketException("Invalid socket descriptor");
    }

    ~TcpSocket() noexcept {
        if (sockfd_ != -1) {
            ::shutdown(sockfd_, SHUT_RDWR);
            ::close(sockfd_);
        }
    }

    TcpSocket(const TcpSocket &) = delete;
    TcpSocket &operator=(const TcpSocket &) = delete;

    TcpSocket(TcpSocket &&other) noexcept : sockfd_(other.sockfd_) {
        other.sockfd_ = -1;
    }
    TcpSocket &operator=(TcpSocket &&other) noexcept {
        if (this != &other) {
            if (sockfd_ != -1)
                ::close(sockfd_);
            sockfd_ = other.sockfd_;
            other.sockfd_ = -1;
        }
        return *this;
    }

    [[nodiscard]] int get() const noexcept { return sockfd_; }

    void bindSocket(const std::string &ip, uint16_t port) const {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        if (::inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0)
            throw SocketException("inet_pton() failed: " + std::string(std::strerror(errno)));
        addr.sin_port = htons(port);
        if (::bind(sockfd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
            throw SocketException("bind() failed: " + std::string(std::strerror(errno)));
    }

    void listenSocket(const int backlog = 5) const {
        if (::listen(sockfd_, backlog) < 0)
            throw SocketException("listen() failed: " + std::string(std::strerror(errno)));
    }

    TcpSocket acceptConnection(sockaddr_in *client_addr = nullptr, socklen_t *addr_len = nullptr) const {
        int new_fd = ::accept(sockfd_, reinterpret_cast<sockaddr*>(client_addr), addr_len);
        if (new_fd < 0)
            throw SocketException("accept() failed: " + std::string(std::strerror(errno)));
        return TcpSocket(new_fd);
    }

    void closeSocket() {
        if (::shutdown(sockfd_, SHUT_RDWR) == -1)
            throw SocketException("shutdown() failed: " + std::string(std::strerror(errno)));
        if (::close(sockfd_) == -1)
            throw SocketException("close() failed: " + std::string(std::strerror(errno)));
        sockfd_ = -1;
    }

    void abortConnection() {
        if (::close(sockfd_) == -1)
            throw SocketException("abortConnection: close() failed: " + std::string(std::strerror(errno)));
        sockfd_ = -1;
    }

private:
    int sockfd_{-1};
};

#endif // TCP_SOCKET_HPP_
