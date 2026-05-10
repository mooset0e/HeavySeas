#include "world/World.h"
#include <random>
#include <algorithm>
#include <cmath>
#include <queue>
#include <tuple>

static Nation assignNation(int x, int y) {
    bool north = y < World::HEIGHT / 3;
    bool south = y > 2 * World::HEIGHT / 3;
    bool west  = x < World::WIDTH  / 4;
    bool east  = x > 3 * World::WIDTH / 4;
    if (north)         return Nation::England;
    if (south && west) return Nation::Spain;
    if (south)         return Nation::Spain;
    if (east)          return Nation::Portugal;
    if (west)          return Nation::France;
    return Nation::Netherlands;
}

static const std::vector<std::string> TOWN_NAMES = {
    "Port Royal", "Tortuga", "Nassau", "Havana", "Cartagena",
    "Maracaibo", "Santo Domingo", "Santiago", "Trinidad", "Bridgetown",
    "Barbados", "Vera Cruz", "Campeche", "Merida", "Panama",
    "Porto Bello", "Santa Marta", "Curacao", "St. Kitts", "Martinique"
};

// ---- generation ----

void World::generate(uint32_t seed) {
    std::mt19937 rng(seed);
    std::bernoulli_distribution landChance(0.55);

    grid_.resize(WIDTH * HEIGHT);
    for (auto& t : grid_)
        t = landChance(rng) ? Tile::Land : Tile::Ocean;

    for (int x = 0; x < WIDTH; ++x) {
        grid_[0 * WIDTH + x]          = Tile::Land;
        grid_[(HEIGHT-1) * WIDTH + x] = Tile::Land;
    }
    for (int y = 0; y < HEIGHT; ++y) {
        grid_[y * WIDTH + 0]         = Tile::Land;
        grid_[y * WIDTH + WIDTH - 1] = Tile::Land;
    }

    std::vector<Tile> next(grid_.size());
    for (int pass = 0; pass < 5; ++pass) {
        for (int y = 0; y < HEIGHT; ++y) {
            for (int x = 0; x < WIDTH; ++x) {
                if (x == 0 || x == WIDTH-1 || y == 0 || y == HEIGHT-1) {
                    next[y * WIDTH + x] = Tile::Land; continue;
                }
                next[y * WIDTH + x] = (countLandNeighbors(x, y) >= 5) ? Tile::Land : Tile::Ocean;
            }
        }
        grid_ = next;
    }

    placeTowns(seed ^ 0xDEADBEEF);
    buildRoutes();
    buildProximityGrid();
}

int World::countLandNeighbors(int x, int y) const {
    int count = 0;
    for (int dy = -1; dy <= 1; ++dy)
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            int nx = x + dx, ny = y + dy;
            if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT) count++;
            else if (grid_[ny * WIDTH + nx] == Tile::Land) count++;
        }
    return count;
}

int World::townAt(int tileX, int tileY) const {
    for (int i = 0; i < (int)towns_.size(); ++i)
        if (towns_[i].x == tileX && towns_[i].y == tileY) return i;
    return -1;
}

int World::townAdjacentTo(int tileX, int tileY) const {
    for (int i = 0; i < (int)towns_.size(); ++i)
        if (std::abs(towns_[i].x - tileX) <= 1 && std::abs(towns_[i].y - tileY) <= 1)
            return i;
    return -1;
}

void World::placeTowns(uint32_t seed) {
    std::vector<std::pair<int,int>> shore;
    for (int y = 1; y < HEIGHT-1; ++y) {
        for (int x = 1; x < WIDTH-1; ++x) {
            if (tile(x, y) != Tile::Land) continue;
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx)
                    if (tile(x+dx, y+dy) == Tile::Ocean) { shore.emplace_back(x,y); goto nxt; }
            nxt:;
        }
    }

    std::mt19937 rng(seed);
    std::shuffle(shore.begin(), shore.end(), rng);

    int count = std::min((int)TOWN_NAMES.size(), (int)shore.size());
    towns_.clear();
    for (auto& [tx, ty] : shore) {
        if ((int)towns_.size() >= count) break;
        bool tooClose = false;
        for (auto& t : towns_)
            if (std::abs(tx-t.x) + std::abs(ty-t.y) < 8) { tooClose = true; break; }
        if (!tooClose)
            towns_.push_back({tx, ty, TOWN_NAMES[towns_.size()], assignNation(tx, ty)});
    }
}

// ---- A* on ocean tiles ----

std::pair<int,int> World::nearestOcean(int cx, int cy) const {
    for (int r = 1; r <= 8; ++r) {
        for (int dy = -r; dy <= r; ++dy) {
            for (int dx = -r; dx <= r; ++dx) {
                if (std::abs(dx) != r && std::abs(dy) != r) continue;
                int nx = cx+dx, ny = cy+dy;
                if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT) continue;
                if (grid_[ny * WIDTH + nx] == Tile::Ocean) return {nx, ny};
            }
        }
    }
    return {-1, -1};
}

std::vector<std::pair<int,int>>
World::astarOcean(int sx, int sy, int ex, int ey) const {
    if (sx == ex && sy == ey) return {{sx, sy}};

    struct Cell { float g = 1e30f; int px = -1, py = -1; bool closed = false; };
    std::vector<Cell> cells(WIDTH * HEIGHT);

    auto idx = [&](int x, int y) { return y * WIDTH + x; };
    auto heur = [&](int x, int y) {
        float dx = (float)(x - ex), dy = (float)(y - ey);
        return std::sqrtf(dx*dx + dy*dy);
    };

    // open = (f, x, y)
    using Entry = std::tuple<float, int, int>;
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> open;

    cells[idx(sx,sy)].g = 0.0f;
    open.push({heur(sx,sy), sx, sy});

    // 8-directional neighbours; diagonal costs √2
    static const int DX[8] = {-1,0,1,-1,1,-1,0,1};
    static const int DY[8] = {-1,-1,-1,0,0,1,1,1};
    static const float DC[8] = {1.4142f,1,1.4142f,1,1,1.4142f,1,1.4142f};

    while (!open.empty()) {
        auto [cf, cx, cy] = open.top(); open.pop();
        int ci = idx(cx, cy);
        if (cells[ci].closed) continue;
        cells[ci].closed = true;

        if (cx == ex && cy == ey) {
            // Reconstruct
            std::vector<std::pair<int,int>> path;
            int x = ex, y = ey;
            while (!(x == sx && y == sy)) {
                path.push_back({x, y});
                int nx = cells[idx(x,y)].px;
                int ny = cells[idx(x,y)].py;
                x = nx; y = ny;
            }
            path.push_back({sx, sy});
            std::reverse(path.begin(), path.end());
            return path;
        }

        for (int d = 0; d < 8; ++d) {
            int nx = cx + DX[d], ny = cy + DY[d];
            if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT) continue;
            if (grid_[ny * WIDTH + nx] != Tile::Ocean) continue;
            int ni = idx(nx, ny);
            if (cells[ni].closed) continue;
            float ng = cells[ci].g + DC[d];
            if (ng < cells[ni].g) {
                cells[ni].g  = ng;
                cells[ni].px = cx;
                cells[ni].py = cy;
                open.push({ng + heur(nx, ny), nx, ny});
            }
        }
    }
    return {}; // no ocean path
}

// ---- route building ----

void World::buildRoutes() {
    routes_.clear();
    int n = (int)towns_.size();
    if (n < 2) return;

    // Connect each port to its 2 nearest neighbours (de-duplicated)
    for (int i = 0; i < n; ++i) {
        std::vector<std::pair<float,int>> dists;
        for (int j = 0; j < n; ++j) {
            if (i == j) continue;
            float dx = (float)(towns_[i].x - towns_[j].x);
            float dy = (float)(towns_[i].y - towns_[j].y);
            dists.push_back({std::sqrtf(dx*dx + dy*dy), j});
        }
        std::sort(dists.begin(), dists.end());

        int connections = std::min(2, (int)dists.size());
        for (int k = 0; k < connections; ++k) {
            int j = dists[k].second;
            bool exists = false;
            for (auto& r : routes_)
                if ((r.portA==i && r.portB==j) || (r.portA==j && r.portB==i))
                    { exists = true; break; }
            if (exists) continue;

            // Find nearest ocean tile to each port (ports sit on land)
            auto [sx, sy] = nearestOcean(towns_[i].x, towns_[i].y);
            auto [ex, ey] = nearestOcean(towns_[j].x, towns_[j].y);
            if (sx < 0 || ex < 0) continue;

            auto path = astarOcean(sx, sy, ex, ey);
            if (path.empty()) continue;

            routes_.push_back({i, j, std::move(path)});
        }
    }
}

// ---- precomputed proximity grid ----

void World::buildProximityGrid() {
    routeProxGrid_.assign(WIDTH * HEIGHT, 0.0f);

    static constexpr float FULL_DIST = 2.0f;  // tiles from route centre = full 1.0
    static constexpr float FADE_DIST = 7.0f;  // tiles from route centre = 0.0

    for (const auto& route : routes_) {
        for (auto& [px, py] : route.path) {
            int r = (int)FADE_DIST + 1;
            for (int dy = -r; dy <= r; ++dy) {
                for (int dx = -r; dx <= r; ++dx) {
                    int nx = px + dx, ny = py + dy;
                    if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT) continue;
                    float d = std::sqrtf((float)(dx*dx + dy*dy));
                    float prox;
                    if (d <= FULL_DIST)  prox = 1.0f;
                    else if (d >= FADE_DIST) prox = 0.0f;
                    else prox = 1.0f - (d - FULL_DIST) / (FADE_DIST - FULL_DIST);
                    float& cell = routeProxGrid_[ny * WIDTH + nx];
                    if (prox > cell) cell = prox;
                }
            }
        }
    }
}

// ---- public queries ----

float World::routeProximity(float wx, float wy) const {
    int tx = std::max(0, std::min((int)wx, WIDTH  - 1));
    int ty = std::max(0, std::min((int)wy, HEIGHT - 1));
    return routeProxGrid_[ty * WIDTH + tx];
}

Nation World::nearestPortNation(float wx, float wy) const {
    float best = 1e9f;
    Nation result = Nation::Independent;
    for (const auto& t : towns_) {
        float dx = wx - t.x, dy = wy - t.y;
        float d = dx*dx + dy*dy;
        if (d < best) { best = d; result = t.nation; }
    }
    return result;
}
