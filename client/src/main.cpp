/*
** EPITECH PROJECT, 2025
** jetpack_client
** File description:
** main.cpp
*/

#include "common/packet_structs.hpp"
#include "common/packet_type.hpp"
#include "network/protocol.hpp"
#include "network/tcp_client_connection.hpp"

#include "input_manager.hpp"
#include "window_manager.hpp"

#include <SFML/Graphics.hpp>
#include <cstring>
#include <iostream>
#include <optional>
#include <poll.h>
#include <vector>
#include <unordered_map>

#include "common/vec2.hpp"

int main() {
    network::TcpClientConnection<PacketType> client;
    if (!client.connect_to_server("127.0.0.1", 12345)) {
        std::cerr << "Failed to connect to server." << std::endl;
        return 1;
    }
    int sockfd = client.get_sockfd();

    std::optional<uint32_t> localPlayerId;

    jetpack::WindowManager windowManager;
    sf::RenderWindow &window = windowManager.getWindow();

    const float worldWidth  = 1726.f;
    const float worldHeight = 341.f;

    auto winSize = window.getSize();
    float aspect = float(winSize.x) / float(winSize.y);
    sf::View gameView;
    gameView.setSize({worldHeight * aspect, worldHeight});
    gameView.setCenter({gameView.getSize().x/2.f, worldHeight/2.f});
    window.setView(gameView);

    sf::Texture backgroundTex;
    if (!backgroundTex.loadFromFile("assets/background.png")) {
        std::cerr << "Erreur : impossible de charger background.png" << std::endl;
        return 1;
    }
    sf::Sprite backgroundSprite(backgroundTex);

    sf::Texture coinSheet;
    if (!coinSheet.loadFromFile("assets/coins_sprite_sheet.png")) {
        std::cerr << "Erreur : impossible de charger coins_sprite_sheet.png" << std::endl;
        return 1;
    }
    auto sheetSize   = coinSheet.getSize();
    int frameW       = sheetSize.x / 6;
    int frameH       = sheetSize.y;
    sf::IntRect coinFrameRect({0,0}, {frameW,frameH});
    std::unordered_map<uint32_t, sf::Sprite> coinSprites;

    std::vector<sf::RectangleShape> zapperShapes;

    sf::Font font;
    if (!font.openFromFile("assets/fonts/Roboto Font/static/Roboto-Black.ttf")) {
        std::cerr << "Failed to load font." << std::endl;
        return 1;
    }
    sf::Text pingText(font, "", 24);
    pingText.setFillColor(sf::Color::White);
    pingText.setPosition({10.f,10.f});

    bool isReady = false;
    sf::RectangleShape readyButton({120.f,40.f});
    readyButton.setPosition({10.f,50.f});
    readyButton.setFillColor({100,100,100});
    sf::Text readyText(font,"Ready",20);
    readyText.setFillColor(sf::Color::White);
    readyText.setPosition( readyButton.getPosition() + sf::Vector2f{10.f,5.f} );

    sf::Text readyCountText(font,"Ready: 0 / 0",20);
    readyCountText.setFillColor(sf::Color::Yellow);
    readyCountText.setPosition({10.f,100.f});

    jetpack::InputManager inputManager(
        [&client](jetpack::InputManager::PlayerInput &&input){
            auto pkt = network::PacketFactory<PacketType>
                           ::CreatePacket(PacketType::kPlayerInput, input);
            client.queue_data(pkt.Data());
        },
        windowManager
    );

    sf::Texture playerTex;
    if (!playerTex.loadFromFile("assets/player_sprite_sheet.png")) {
        std::cerr << "Failed to load player sprite sheet" << std::endl;
        return 1;
    }
    sf::IntRect playerFrameRect({0,0},{134,130});
    std::unordered_map<uint32_t, sf::Sprite> playerSprites;

    sf::Clock pingClock;
    uint32_t lastPingTime=0, pingMs=0;

    pollfd pfd{ .fd = sockfd, .events = POLLIN };

    bool gameStarted = false;
    bool isDead      = false;
    uint32_t finalScore = 0;

    sf::Text scoreText(font, "", 32);
    scoreText.setFillColor(sf::Color::White);
    scoreText.setPosition({400.f, 150.f});

    sf::RectangleShape quitButton({200.f, 50.f});
    quitButton.setFillColor({150, 50, 50});
    quitButton.setPosition({400.f, 250.f});
    sf::Text quitText(font, "Quit", 24);
    quitText.setFillColor(sf::Color::White);
    quitText.setPosition(quitButton.getPosition() + sf::Vector2f(60.f, 10.f));

    while(window.isOpen()) {
        while(auto evtOpt = window.pollEvent()) {
            auto &evt = *evtOpt;
            if(evt.is<sf::Event::Closed>()) {
                window.close();
            }
            if(evt.is<sf::Event::Resized>()) {
                auto *r = evt.getIf<sf::Event::Resized>();
                gameView.setSize({ static_cast<float>(r->size.x), static_cast<float>(r->size.y) });
                window.setView(gameView);
            }
            if (isDead && evt.is<sf::Event::MouseButtonPressed>()) {
                auto mb = evt.getIf<sf::Event::MouseButtonPressed>();
                if (mb->button == sf::Mouse::Button::Left) {
                    sf::Vector2f mpos = window.mapPixelToCoords({mb->position.x, mb->position.y});
                    if (quitButton.getGlobalBounds().contains(mpos)) {
                        window.close();
                    }
                }
            }
            if(!gameStarted && evt.is<sf::Event::MouseButtonPressed>()) {
                auto mb = evt.getIf<sf::Event::MouseButtonPressed>();
                if(mb->button == sf::Mouse::Button::Left) {
                    sf::Vector2f mpos = window.mapPixelToCoords({mb->position.x, mb->position.y});
                    if(readyButton.getGlobalBounds().contains(mpos)) {
                        isReady = !isReady;
                        readyText.setString(isReady ? "Cancel" : "Ready");
                        PlayerReadyPacket pr{ isReady };
                        auto rpkt = network::PacketFactory<PacketType>
                                        ::CreatePacket(PacketType::kPlayerReady, pr);
                        client.queue_data(rpkt.Data());
                    }
                }
            }
            inputManager.HandleEvent(evt);
        }

        pfd.events = client.hasPendingSendData() ? POLLIN|POLLOUT : POLLIN;
        if(::poll(&pfd,1,5)>0) {
            if(pfd.revents & POLLOUT) {
                client.send_data();
            }
            if(pfd.revents & POLLIN) {
                if(auto pkts = client.read_packets()) {
                    for(auto &packet : *pkts) {
                        switch(packet.header.type) {
                        case PacketType::kAssignPlayerId: {
                            uint32_t id;
                            std::memcpy(&id, packet.body.data(), sizeof(id));
                            localPlayerId = id;
                            std::cout<<"Assigned local ID: "<<id<<"\n";
                            break;
                        }
                        case PacketType::kPlayerReadyCount: {
                            auto prc = network::PacketFactory<PacketType>
                                           ::ExtractData<PlayerReadyCountPacket>(packet);
                            if(prc) {
                                readyCountText.setString(
                                    "Ready: "
                                    + std::to_string(prc->ready_count)
                                    + " / "
                                    + std::to_string(prc->total_count)
                                );
                            }
                            break;
                        }
                        case PacketType::kPong: {
                            uint32_t sent; std::memcpy(&sent, packet.body.data(),sizeof(sent));
                            auto now = std::chrono::steady_clock::now();
                            auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                             now.time_since_epoch()).count();
                            pingMs = uint32_t(nowMs) - sent;
                            break;
                        }
                        case PacketType::kMapCoins: {
                            if(auto arr = network::PacketFactory<PacketType>
                                           ::ExtractDataArray<MapCoin>(packet))
                            {
                                for(auto &mc : *arr) {
                                    sf::Sprite spr(coinSheet, coinFrameRect);
                                    spr.setOrigin({frameW/2.f, frameH/2.f});
                                    spr.setScale({0.1f, 0.1f});
                                    spr.setPosition({mc.pos.x, mc.pos.y});
                                    coinSprites.emplace(mc.id, std::move(spr));
                                }
                            }
                            break;
                        }
                        case PacketType::kMapZappers: {
                            if(auto arr = network::PacketFactory<PacketType>
                                           ::ExtractDataArray<MapZapperSegment>(packet))
                            {
                                zapperShapes.clear();
                                for(auto &zs : *arr) {
                                    sf::Vector2f a{zs.a.x,zs.a.y}, b{zs.b.x,zs.b.y};
                                    sf::RectangleShape rect;
                                    if(a.y==b.y) {
                                        rect.setSize({ std::abs(b.x-a.x), 5.f });
                                        rect.setPosition(a);
                                    } else {
                                        rect.setSize({ 5.f, std::abs(b.y-a.y) });
                                        rect.setPosition(a);
                                    }
                                    rect.setFillColor(sf::Color::Red);
                                    zapperShapes.push_back(rect);
                                }
                            }
                            break;
                        }
                        case PacketType::kCoinCollected: {
                            if(auto ccp = network::PacketFactory<PacketType>
                                            ::ExtractData<CoinCollectedPacket>(packet))
                            {
                                auto it = coinSprites.find(ccp->coin_id);
                                if(it!=coinSprites.end()){
                                    auto col = it->second.getColor();
                                    col.a = 128;
                                    it->second.setColor(col);
                                }
                            }
                            break;
                        }
                        case PacketType::kCoinExpired: {
                            if(auto cep = network::PacketFactory<PacketType>
                                            ::ExtractData<CoinExpiredPacket>(packet))
                            {
                                coinSprites.erase(cep->coin_id);
                            }
                            break;
                        }
                        case PacketType::kZapperCollision: {
                            if(auto zcp = network::PacketFactory<PacketType>
                                            ::ExtractData<ZapperCollisionPacket>(packet))
                            {
                                std::cout<<"Player "<<zcp->player_id
                                         <<" hit zapper "<<zcp->zapper_id<<"\n";
                            }
                            break;
                        }
                        case PacketType::kPlayerDeath: {
                            if(auto pdp = network::PacketFactory<PacketType>
                                            ::ExtractData<PlayerDeathPacket>(packet))
                            {
                                playerSprites.erase(pdp->player_id);
                                std::cout<<"Player "<<pdp->player_id<<" died\n";
                            }
                            break;
                        }
                        case PacketType::kUpdatePlayers: {
                            if(auto arr = network::PacketFactory<PacketType>
                                           ::ExtractDataArray<UpdatePlayer>(packet))
                            {
                                for(auto &up : *arr) {
                                    auto it = playerSprites.find(up.player_id);
                                    if(it==playerSprites.end()){
                                        sf::Sprite spr(playerTex, playerFrameRect);
                                        spr.setOrigin(
                                          {playerFrameRect.size.x/2.f,
                                          playerFrameRect.size.y/2.f}
                                        );
                                        spr.setScale({0.2f,0.2f});
                                        spr.setPosition({up.x, up.y});
                                        // si ce n'est pas nous, on l'estompe
                                        if(localPlayerId && *localPlayerId!=up.player_id){
                                            auto c = spr.getColor();
                                            c.a = 128;
                                            spr.setColor(c);
                                        }
                                        playerSprites.emplace(up.player_id, std::move(spr));
                                    } else {
                                        // update position
                                        it->second.setPosition({up.x, up.y});
                                    }
                                }
                            }
                            break;
                        }
                        case PacketType::kPlayerScore: {
                            if (auto psp = network::PacketFactory<PacketType>
                                             ::ExtractData<PlayerScorePacket>(packet)) {
                                if (localPlayerId && psp->player_id == *localPlayerId) {
                                    isDead     = true;
                                    finalScore = psp->coins_collected;
                                    scoreText.setString("Your score: " + std::to_string(finalScore));
                                }
                                             }
                            break;
                        }
                        case PacketType::kGameStart: {
                            std::cout<<"Game is starting now!\n";
                            gameStarted = true;
                            break;
                        }
                        default: break;
                        }
                    }
                }
            }

            if(pingClock.getElapsedTime().asSeconds()>=1.f){
                auto now = std::chrono::steady_clock::now();
                uint32_t nowMs = uint32_t(
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        now.time_since_epoch()
                    ).count()
                );
                lastPingTime = nowMs;
                auto pingPkt = network::PacketFactory<PacketType>
                                   ::CreatePacket(PacketType::kPing, lastPingTime);
                client.queue_data(pingPkt.Data());
                client.send_data();
                pingClock.restart();
            }
            pingText.setString("Ping: "+std::to_string(pingMs)+" ms");

            if(localPlayerId){
                auto it = playerSprites.find(*localPlayerId);
                if(it!=playerSprites.end()){
                    sf::Vector2f pos = it->second.getPosition();
                    sf::Vector2f half = gameView.getSize()/2.f;
                    float cx = std::clamp(pos.x, half.x, worldWidth-half.x);
                    float cy = std::clamp(pos.y, half.y, worldHeight-half.y);
                    gameView.setCenter({cx,cy});
                }
            }

            window.clear(sf::Color::Black);
            window.setView(gameView);
            window.draw(backgroundSprite);
            for(auto &kv : coinSprites)   window.draw(kv.second);
            for(auto &r: zapperShapes)     window.draw(r);
            for(auto &kv : playerSprites)  window.draw(kv.second);

            window.setView(window.getDefaultView());
            window.draw(pingText);

            if (!gameStarted && !isDead) {
                window.draw(readyButton);
                window.draw(readyText);
                window.draw(readyCountText);
            }

            if (isDead) {
                window.draw(scoreText);
                window.draw(quitButton);
                window.draw(quitText);
            }

            window.display();
            sf::sleep(sf::milliseconds(16));
        }
    }

    return 0;
}
