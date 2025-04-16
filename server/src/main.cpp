/*
** EPITECH PROJECT, 2025
** jetpack_server
** File description:
** main.cpp
**
**⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣄⣀⣀⠀⠀⣶⣤⣀⠀⢀⣄⠀⠀⠀⠀⠀⠀
**⠀⠀⠀⠀⠀⠀⢀⣠⣤⣤⣌⡻⣿⣿⣿⣶⣼⣿⣿⣷⣼⣿⣷⡄⢿⣷⡀⠀
**⠀⠀⠀⠀⠀⠐⠿⠿⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣾⣿⣿⡄
**⠀⠀⠀⢀⣠⣶⣶⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣇
**⠀⠀⢠⣿⣿⣿⣿⣿⣿⣿⣿⠿⠛⠛⠿⠿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡏
**⠀⠀⠀⣠⣾⣿⣿⣿⣿⠋⠀⣀⣀⣀⠀⠀⠀⠀⠉⠛⠛⠿⢿⣿⣿⣿⣿⡇
**⠀⠀⠀⠈⢻⣿⣿⣿⣷⠀⠀⢉⣭⣿⣿⣷⣦⣄⠀⠀⠀⠀⠀⠀⠀⠀⢿⡇
**⠀⢀⣀⣀⢸⣿⣿⣿⡟⠀⠀⣿⡏⢀⣴⢿⣿⢿⣷⡀⠀⢀⣀⣠⣤⣄⣸⡇
**⣸⡟⠋⠛⠿⣿⣿⣿⠁⠀⠀⢿⣇⢸⣇⣸⣿⣼⡟⠀⠀⢸⡟⢻⣿⢻⣿⠃
**⢸⣧⠠⡿⣷⡜⣿⣿⡄⠀⠀⠈⠻⠿⣿⡿⠿⠛⠀⠘⢿⣾⣷⣾⢋⣿⠃⠀
**⠀⠹⣷⡀⠘⠃⢹⣿⣷⣄⠀⠀⣠⣄⣀⣀⡀⠈⠻⠷⠾⠃⠉⠛⢿⡏⠀⠀
**⠀⠀⠈⠻⣶⣶⡾⣿⣿⣿⣶⣾⣿⣏⠉⠙⠛⠻⠷⠶⠶⠶⢶⣶⣾⠃⠀⠀
**⠀⠀⠀⠀⠀⠀⠀⢹⣿⣿⣿⣿⣿⣿⡆⣀⣀⣤⣤⣄⣀⣀⣿⣿⠏⠀⠀⠀
**⠀⠀⠀⠀⠀⠀⠀⠀⠙⠻⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡏⠀⠀⠀⠀
**⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠉⠛⠻⠿⠿⠿⠿⠿⠟⠛⠋⠀⠀⠀⠀⠀
*/

#include <iostream>

#include "game_server.hpp"
#include "common/packet_type.hpp"

int main() {
    try {
        server::GameServer<PacketType> gameServer(12345);
        std::cout << "Server is running on port 12345." << std::endl;

        gameServer.Run();
    }
    catch (const std::exception &ex) {
        std::cerr << "Server error: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
