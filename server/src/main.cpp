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

#include "common/packet_type.hpp"
#include "game_server.hpp"

int main() {
  try {
    server::GameServer<PacketType> game_server(12345, "assets/basic.map");
    std::cout << "Server is running on port 12345." << std::endl;

    game_server.Run();
  } catch (const std::exception &ex) {
    std::cerr << "Server error: " << ex.what() << std::endl;
    return 1;
  }

  return 0;
}
