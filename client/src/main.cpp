/*
** EPITECH PROJECT, 2025
** jetpack_client
** File description:
** main.cpp
*/

#include "network/tcp_client_connection.hpp"
#include "network/protocol.hpp"
#include "common/packet_type.hpp"

#include "window_manager.hpp"
#include "input_manager.hpp"

#include <SFML/Graphics.hpp>
#include <iostream>
#include <optional>
#include <vector>
#include <thread>
#include <poll.h>
#include <sstream>

int main() {
    network::TcpClientConnection<PacketType> client;
    if (!client.connect_to_server("127.0.0.1", 12345)) {
        std::cerr << "Failed to connect to server." << std::endl;
        return 1;
    }

    int sockfd = client.get_sockfd();

    pollfd clientPoll{};
    clientPoll.fd = sockfd;
    clientPoll.events = POLLIN;

    jetpack::WindowManager windowManager;
    sf::RenderWindow& window = windowManager.getWindow();

    sf::Clock pingClock;
    std::uint32_t lastPingTime = 0;
    std::uint32_t pingMs = 0;

    sf::Font font;
    if (!font.openFromFile("assets/fonts/Roboto Font/static/Roboto-Black.ttf")) {
        std::cerr << "Failed to load font." << std::endl;
        return 1;
    }

    sf::Text pingText(font, "", 24);
    pingText.setFillColor(sf::Color::White);
    pingText.setPosition(sf::Vector2f(10.f, 10.f));

    jetpack::InputManager inputManager(
        [&client](jetpack::InputManager::PlayerInput&& input) {
            auto inputPacket = network::PacketFactory<PacketType>::CreatePacket(PacketType::kPlayerInput, input);
            client.queue_data(inputPacket.Data());
            client.send_data();
        },
        windowManager
    );

    while (window.isOpen()) {
        while (const std::optional<sf::Event> event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            inputManager.HandleEvent(*event);
        }

        if (client.hasPendingSendData()) {
            clientPoll.events = POLLIN | POLLOUT;
        } else {
            clientPoll.events = POLLIN;
        }

        int ret = ::poll(&clientPoll, 1, 5);
        if (ret < 0) {
            std::cerr << "Poll error: " << std::strerror(errno) << std::endl;
        } else if (ret > 0) {
            if (clientPoll.revents & POLLHUP) {
                std::cerr << "Server disconnected." << std::endl;
                window.close();
            }
            if (clientPoll.revents & POLLOUT) {
                if (!client.send_data()) {
                    std::cerr << "Error sending data." << std::endl;
                }
            }
            if (clientPoll.revents & POLLIN) {
                auto maybePackets = client.read_packets();
                if (maybePackets.has_value()) {
                    for (const auto &packet : maybePackets.value()) {
                        if (packet.header.type == PacketType::kPong) {
                            if (packet.body.size() >= sizeof(std::uint32_t)) {
                                std::uint32_t sentTime;
                                std::memcpy(&sentTime, packet.body.data(), sizeof(std::uint32_t));
                                auto now = std::chrono::steady_clock::now();
                                auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
                                pingMs = static_cast<std::uint32_t>(nowMs) - sentTime;
                            }
                        }
                    }
                }
            }
        }

        if (pingClock.getElapsedTime().asSeconds() >= 1.0f) {
            auto now = std::chrono::steady_clock::now();
            auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            lastPingTime = static_cast<std::uint32_t>(nowMs);

            auto pingPacket = network::PacketFactory<PacketType>::CreatePacket(PacketType::kPing, lastPingTime);
            client.queue_data(pingPacket.Data());
            client.send_data();
            pingClock.restart();
        }

        std::ostringstream oss;
        oss << "Ping: " << pingMs << " ms";
        pingText.setString(oss.str());

        window.clear(sf::Color::Black);
        window.draw(pingText);

        sf::RectangleShape rect(sf::Vector2f(100.f, 50.f));
        rect.setFillColor(sf::Color::Green);
        rect.setPosition(sf::Vector2f(350.f, 275.f));
        window.draw(rect);

        window.display();
        sf::sleep(sf::milliseconds(16));
    }

    return 0;
}
