#include "world/World.h"
#include <random>
#include <algorithm>
#include <cmath>

static const std::vector<std::string> TOWN_NAMES = {
    "Port Royal", "Tortuga", "Nassau", "Havana", "Cartagena",
    "Maracaibo", "Santo Domingo", "Santiago", "Trinidad", "Bridgetown",
    "Barbados", "Vera Cruz", "Campeche", "Merida", "Panama",
    "Porto Bello", "Santa Marta", "Curacao", "St. Kitts", "Martinique"
};

void World::generate(uint32_t seed) {
    std::mt19937 rng(seed);
    std::bernoulli_distribution landChance(0.55);

    grid_.resize(WIDTH * HEIGHT);

    // Random fill
    for (auto& t : grid_)
        t = landChance(rng) ? Tile::Land : Tile::Ocean;

    // Force land border so the map is enclosed by coastline
    for (int x = 0; x < WIDTH; ++x) {
        grid_[0 * WIDTH + x]          = Tile::Land;
        grid_[(HEIGHT-1) * WIDTH + x] = Tile::Land;
    }
    for (int y = 0; y < HEIGHT; ++y) {
        grid_[y * WIDTH + 0]         = Tile::Land;
        grid_[y * WIDTH + WIDTH - 1] = Tile::Land;
    }

    // Cellular automata: 5 passes
    std::vector<Tile> next(grid_.size());
    for (int pass = 0; pass < 5; ++pass) {
        for (int y = 0; y < HEIGHT; ++y) {
            for (int x = 0; x < WIDTH; ++x) {
                // Keep border as land
                if (x == 0 || x == WIDTH-1 || y == 0 || y == HEIGHT-1) {
                    next[y * WIDTH + x] = Tile::Land;
                    continue;
                }
                int n = countLandNeighbors(x, y);
                next[y * WIDTH + x] = (n >= 5) ? Tile::Land : Tile::Ocean;
            }
        }
        grid_ = next;
    }

    placeTowns(seed ^ 0xDEADBEEF);
}

int World::countLandNeighbors(int x, int y) const {
    int count = 0;
    for (int dy = -1; dy <= 1; ++dy)
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            int nx = x + dx, ny = y + dy;
            if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT) {
                // Treat out-of-bounds as land to keep edges solid
                count++;
            } else if (grid_[ny * WIDTH + nx] == Tile::Land) {
                count++;
            }
        }
    return count;
}

int World::townAdjacentTo(int tileX, int tileY) const {
    for (int i = 0; i < (int)towns_.size(); ++i) {
        if (std::abs(towns_[i].x - tileX) <= 1 && std::abs(towns_[i].y - tileY) <= 1)
            return i;
    }
    return -1;
}

void World::placeTowns(uint32_t seed) {
    // Collect shoreline candidates: land tiles adjacent to at least one ocean tile
    std::vector<std::pair<int,int>> shore;
    for (int y = 1; y < HEIGHT-1; ++y) {
        for (int x = 1; x < WIDTH-1; ++x) {
            if (tile(x, y) != Tile::Land) continue;
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx) {
                    if (tile(x+dx, y+dy) == Tile::Ocean) {
                        shore.emplace_back(x, y);
                        goto next_tile;
                    }
                }
            next_tile:;
        }
    }

    std::mt19937 rng(seed);
    std::shuffle(shore.begin(), shore.end(), rng);

    int count = std::min(static_cast<int>(TOWN_NAMES.size()),
                         static_cast<int>(shore.size()));

    // Space towns out: reject candidates within 8 tiles of an existing town
    towns_.clear();
    for (auto& [tx, ty] : shore) {
        if (static_cast<int>(towns_.size()) >= count) break;
        bool tooClose = false;
        for (auto& existing : towns_) {
            int dist = std::abs(tx - existing.x) + std::abs(ty - existing.y);
            if (dist < 8) { tooClose = true; break; }
        }
        if (!tooClose)
            towns_.push_back({tx, ty, TOWN_NAMES[towns_.size()]});
    }
}
