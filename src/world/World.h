#pragma once
#include <vector>
#include <string>
#include <cstdint>

enum class Tile : uint8_t { Ocean, Land };

struct Town {
    int x, y;
    std::string name;
};

class World {
public:
    static constexpr int WIDTH  = 128;
    static constexpr int HEIGHT = 72;

    World() = default;
    void generate(uint32_t seed);

    Tile tile(int x, int y) const { return grid_[y * WIDTH + x]; }
    const std::vector<Town>& towns() const { return towns_; }
    int townAdjacentTo(int tileX, int tileY) const;

private:
    std::vector<Tile> grid_;
    std::vector<Town> towns_;

    int countLandNeighbors(int x, int y) const;
    void placeTowns(uint32_t seed);
};
