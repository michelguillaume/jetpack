#ifndef NETWORK_TCP_SERVER_HPP_
#define NETWORK_TCP_SERVER_HPP_

#include "tcp_socket.hpp"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <stdexcept>

class TcpServer {
public:
  explicit TcpServer(uint16_t port) {
    server_socket_ = TcpSocket();
    server_socket_.bindSocket("0.0.0.0", port);
    server_socket_.listenSocket(5);
    std::cout << "Server is listening on port " << port << std::endl;
  }

  TcpSocket acceptConnection() {
    sockaddr_in client_addr{};
    socklen_t addr_len = sizeof(client_addr);
    TcpSocket clientSocket = server_socket_.acceptConnection(&client_addr, &addr_len);
    char client_ip[INET_ADDRSTRLEN] = {0};
    if (inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip))) {
      std::cout << "Accepted connection from " << client_ip
                << ":" << ntohs(client_addr.sin_port) << std::endl;
    }
    return clientSocket;
  }

  TcpSocket &getSocket() { return server_socket_; }

private:
  TcpSocket server_socket_;
};

#endif // NETWORK_TCP_SERVER_HPP_
