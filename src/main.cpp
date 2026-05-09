#include <SDL.h>
#include <iostream>
#include "world/World.h"

static constexpr int TILE_SIZE   = 64;
static constexpr int SCREEN_W    = 1280;
static constexpr int SCREEN_H    = 720;
static constexpr float PLAYER_SPEED = 6.0f; // tiles per second

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Heavy Seas",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_W, SCREEN_H,
        SDL_WINDOW_SHOWN
    );
    if (!window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    World world;
    world.generate(12345);

    // Player starts in the ocean tile adjacent to the first town
    const auto& startTown = world.towns().front();
    float px = startTown.x + 0.5f;
    float py = startTown.y + 0.5f;
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            int nx = startTown.x + dx, ny = startTown.y + dy;
            if (nx >= 0 && nx < World::WIDTH && ny >= 0 && ny < World::HEIGHT
                && world.tile(nx, ny) == Tile::Ocean) {
                px = nx + 0.5f;
                py = ny + 0.5f;
                goto foundStart;
            }
        }
    }
    foundStart:;

    Uint32 lastTick = SDL_GetTicks();
    bool running = true;
    SDL_Event event;

    while (running) {
        Uint32 now = SDL_GetTicks();
        float dt = (now - lastTick) / 1000.0f;
        lastTick = now;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
                running = false;
        }

        // Smooth movement via keyboard state
        const Uint8* keys = SDL_GetKeyboardState(nullptr);
        float dx = 0, dy = 0;
        if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP])    dy -= 1;
        if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN])  dy += 1;
        if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT])  dx -= 1;
        if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) dx += 1;

        // Normalize diagonal movement
        if (dx != 0 && dy != 0) { dx *= 0.7071f; dy *= 0.7071f; }

        // Resolve X and Y independently so player slides along coastlines
        auto canOccupy = [&](float nx, float ny) {
            int tx = (int)nx, ty = (int)ny;
            tx = std::max(0, std::min(tx, World::WIDTH  - 1));
            ty = std::max(0, std::min(ty, World::HEIGHT - 1));
            return world.tile(tx, ty) == Tile::Ocean;
        };

        float newPx = px + dx * PLAYER_SPEED * dt;
        float newPy = py + dy * PLAYER_SPEED * dt;
        newPx = std::max(0.5f, std::min(newPx, World::WIDTH  - 0.5f));
        newPy = std::max(0.5f, std::min(newPy, World::HEIGHT - 0.5f));

        if (canOccupy(newPx, py)) px = newPx;
        if (canOccupy(px, newPy)) py = newPy;

        // Camera: player centered, clamped so the world edge never scrolls into view
        float camX = px * TILE_SIZE - SCREEN_W / 2.0f;
        float camY = py * TILE_SIZE - SCREEN_H / 2.0f;
        camX = std::max(0.0f, std::min(camX, World::WIDTH  * TILE_SIZE - (float)SCREEN_W));
        camY = std::max(0.0f, std::min(camY, World::HEIGHT * TILE_SIZE - (float)SCREEN_H));

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Draw tiles — only those that overlap the viewport
        int tileX0 = (int)(camX / TILE_SIZE);
        int tileY0 = (int)(camY / TILE_SIZE);
        int tileX1 = tileX0 + SCREEN_W / TILE_SIZE + 2;
        int tileY1 = tileY0 + SCREEN_H / TILE_SIZE + 2;

        for (int ty = tileY0; ty <= tileY1; ++ty) {
            for (int tx = tileX0; tx <= tileX1; ++tx) {
                if (tx < 0 || tx >= World::WIDTH || ty < 0 || ty >= World::HEIGHT)
                    continue;

                int sx = (int)(tx * TILE_SIZE - camX);
                int sy = (int)(ty * TILE_SIZE - camY);
                SDL_Rect r{ sx, sy, TILE_SIZE, TILE_SIZE };

                if (world.tile(tx, ty) == Tile::Land)
                    SDL_SetRenderDrawColor(renderer, 60, 120, 40, 255);
                else
                    SDL_SetRenderDrawColor(renderer, 10, 40, 80, 255);
                SDL_RenderFillRect(renderer, &r);
            }
        }

        // Draw towns
        SDL_SetRenderDrawColor(renderer, 220, 60, 60, 255);
        for (const auto& town : world.towns()) {
            int sx = (int)(town.x * TILE_SIZE - camX);
            int sy = (int)(town.y * TILE_SIZE - camY);
            SDL_Rect r{ sx + 6, sy + 6, TILE_SIZE - 12, TILE_SIZE - 12 };
            SDL_RenderFillRect(renderer, &r);
        }

        // Draw player at its actual world position relative to the clamped camera
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        int playerScreenX = (int)(px * TILE_SIZE - camX);
        int playerScreenY = (int)(py * TILE_SIZE - camY);
        SDL_Rect playerRect{
            playerScreenX - TILE_SIZE / 4,
            playerScreenY - TILE_SIZE / 4,
            TILE_SIZE / 2,
            TILE_SIZE / 2
        };
        SDL_RenderFillRect(renderer, &playerRect);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
