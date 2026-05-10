#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include "core/Nation.h"
#include "world/ShippingRoute.h"

enum class Tile : uint8_t { Ocean, Land };

struct Town {
    int         x, y;
    std::string name;
    Nation      nation = Nation::Independent;
};

class World {
public:
    static constexpr int WIDTH  = 128;
    static constexpr int HEIGHT = 72;

    World() = default;
    void generate(uint32_t seed);

    Tile tile(int x, int y) const { return grid_[y * WIDTH + x]; }
    const std::vector<Town>&          towns()  const { return towns_;  }
    const std::vector<ShippingRoute>& routes() const { return routes_; }

    int townAdjacentTo(int tileX, int tileY) const;
    int townAt(int tileX, int tileY) const;

    // 0 = not near any route, 1 = squarely on a route
    float routeProximity(float worldX, float worldY) const;

    // Nation of the nearest port (used for encounter generation)
    Nation nearestPortNation(float worldX, float worldY) const;

private:
    std::vector<Tile>          grid_;
    std::vector<Town>          towns_;
    std::vector<ShippingRoute> routes_;
    std::vector<float>         routeProxGrid_; // precomputed per-tile proximity (0–1)

    int countLandNeighbors(int x, int y) const;
    void placeTowns(uint32_t seed);
    void buildRoutes();
    void buildProximityGrid();

    // A* on ocean tiles; returns empty vector if no path exists.
    std::vector<std::pair<int,int>> astarOcean(int sx, int sy, int ex, int ey) const;

    // Nearest ocean tile to (cx,cy), searching outward.
    std::pair<int,int> nearestOcean(int cx, int cy) const;
};
