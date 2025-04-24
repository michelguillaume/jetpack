#pragma once

#include "common/vec2.hpp"
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>

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
    std::vector occ(rows, std::vector(cols, false));
    coins_.clear();
    segments_.clear();
    hasFinish_ = false;

    uint32_t nextId = 0;
    for (uint32_t r = 0; r < rows; ++r) {
      for (uint32_t c = 0; c < cols; ++c) {
        char ch = lines_[r][c];
        Vec2 worldPos{
          static_cast<float>(c) * tileSize + tileSize / 2.f,
          static_cast<float>(r) * tileSize + tileSize / 2.f
        };

        if (ch == 'c') {
          coins_.push_back({ nextId++, worldPos });
        }
        else if (ch == 'e') {
          occ[r][c] = true;
        }
        else if (ch == 'F') {
          finishPos_ = worldPos;
          hasFinish_ = true;
        }
      }
    }

    for (uint32_t r = 0; r < rows; ++r) {
      uint32_t c = 0;
      while (c < cols) {
        if (!occ[r][c]) { ++c; continue; }
        uint32_t start = c;
        while (c < cols && occ[r][c]) ++c;
        uint32_t end = c - 1;
        if (end > start) {
          Vec2 A{ static_cast<float>(start) * tileSize, static_cast<float>(r) * tileSize };
          Vec2 B{ static_cast<float>(end + 1) * tileSize, static_cast<float>(r) * tileSize };
          segments_.push_back({ nextId++, A, B });
        }
      }
    }

    for (uint32_t c = 0; c < cols; ++c) {
      uint32_t r = 0;
      while (r < rows) {
        if (!occ[r][c]) { ++r; continue; }
        uint32_t start = r;
        while (r < rows && occ[r][c]) ++r;
        uint32_t end = r - 1;
        if (end > start) {
          Vec2 A{ static_cast<float>(c) * tileSize, static_cast<float>(start) * tileSize };
          Vec2 B{ static_cast<float>(c) * tileSize, static_cast<float>(end + 1) * tileSize };
          segments_.push_back({ nextId++, A, B });
        }
      }
    }

    return true;
  }

  [[nodiscard]] const std::vector<Coin> &getCoins() const { return coins_; }
  [[nodiscard]] const std::vector<ZapperSegment> &getZapperSegments() const { return segments_; }

  [[nodiscard]] bool isFinishReached(const Vec2 &playerPos) const {
    if (!hasFinish_) return false;
    return playerPos.x >= finishPos_.x;
  }

private:
  std::vector<std::string> lines_;
  std::vector<Coin> coins_;
  std::vector<ZapperSegment> segments_;

  Vec2 finishPos_{ 0.f, 0.f };
  bool hasFinish_ = false;
};

} // namespace server
