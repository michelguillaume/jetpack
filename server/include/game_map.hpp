#pragma once

#include "common/vec2.hpp"
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace server {

struct Coin {
  uint32_t id;
  Vec2 pos;
};

struct ZapperSegment {
  uint32_t id;
  Vec2 a;
  Vec2 b;
};

class GameMap {
public:
  bool loadFromFile(const std::string &path, float tileSize) {
    std::ifstream in(path);
    if (!in)
      return false;

    lines_.clear();
    for (std::string line; std::getline(in, line);)
      lines_.push_back(line);

    const uint32_t rows = lines_.size();
    const uint32_t cols = rows ? lines_[0].size() : 0;
    std::vector<std::vector<bool>> occ(rows, std::vector<bool>(cols, false));
    coins_.clear();
    segments_.clear();

    uint32_t nextId = 0;
    for (uint32_t r = 0; r < rows; ++r) {
      for (uint32_t c = 0; c < cols; ++c) {
        char ch = lines_[r][c];
        Vec2 worldPos{float(c) * tileSize + tileSize / 2.f,
                      float(r) * tileSize + tileSize / 2.f};
        if (ch == 'c') {
          coins_.push_back({nextId++, worldPos});
        } else if (ch == 'e') {
          occ[r][c] = true;
        }
      }
    }

    for (uint32_t r = 0; r < rows; ++r) {
      uint32_t c = 0;
      while (c < cols) {
        if (!occ[r][c]) {
          ++c;
          continue;
        }
        uint32_t start = c;
        while (c < cols && occ[r][c])
          ++c;
        uint32_t end = c - 1;
        if (end > start) {
          Vec2 A{float(start) * tileSize, float(r) * tileSize};
          Vec2 B{float(end + 1) * tileSize, float(r) * tileSize};
          segments_.push_back({nextId++, A, B});
        }
      }
    }

    for (uint32_t c = 0; c < cols; ++c) {
      uint32_t r = 0;
      while (r < rows) {
        if (!occ[r][c]) {
          ++r;
          continue;
        }
        uint32_t start = r;
        while (r < rows && occ[r][c])
          ++r;
        uint32_t end = r - 1;
        if (end > start) {
          Vec2 A{float(c) * tileSize, float(start) * tileSize};
          Vec2 B{float(c) * tileSize, float(end + 1) * tileSize};
          segments_.push_back({nextId++, A, B});
        }
      }
    }
    return true;
  }

  const std::vector<Coin> &getCoins() const { return coins_; }
  const std::vector<ZapperSegment> &getZapperSegments() const {
    return segments_;
  }

private:
  std::vector<std::string> lines_;
  std::vector<Coin> coins_;
  std::vector<ZapperSegment> segments_;
};

} // namespace server
