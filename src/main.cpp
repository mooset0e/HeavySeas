#include <SDL.h>
#include <SDL_ttf.h>
#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>

#include "core/Constants.h"
#include "core/GameState.h"
#include "core/InputState.h"
#include "ship/ShipStats.h"
#include "world/World.h"
#include "world/Port.h"
#include "world/Wind.h"
#include "ui/UIRenderer.h"
#include "ui/HUD.h"
#include "ui/PortScreen.h"
#include "ui/CloudSystem.h"

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
    }
    if (TTF_Init() != 0) {
        std::cerr << "TTF_Init failed: " << TTF_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Heavy Seas",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_W, SCREEN_H,
        SDL_WINDOW_SHOWN);
    if (!window) { std::cerr << SDL_GetError() << "\n"; TTF_Quit(); SDL_Quit(); return 1; }

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) { std::cerr << SDL_GetError() << "\n"; SDL_DestroyWindow(window); TTF_Quit(); SDL_Quit(); return 1; }

    TTF_Font* font = TTF_OpenFont("assets/fonts/main.ttf", 18);
    if (!font) {
        std::cerr << "TTF_OpenFont failed: " << TTF_GetError() << "\n";
        SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); TTF_Quit(); SDL_Quit();
        return 1;
    }

    // --- World & game objects ---
    World world;
    world.generate(12345);

    std::vector<Port> ports(world.towns().size());
    for (int i = 0; i < (int)ports.size(); ++i)
        ports[i].townIndex = i;

    ShipStats  ship;
    GameState  gameState;
    InputState input;
    Wind       wind;

    float playerHeading   = 0.0f;  // degrees, 0=N clockwise
    float windSpeedFactor = 1.0f;
    int   speedLevel      = 1;     // 0=anchored, 1=slow, 2=medium, 3=full
    static constexpr float SPEED_TIERS[] = { 0.0f, 0.35f, 0.65f, 1.0f };

    UIRenderer  uiRenderer(renderer, font);
    HUD         hud(uiRenderer);
    CloudSystem clouds;
    std::unique_ptr<PortScreen> portScreen;

    // Player starts on the ocean tile adjacent to the first town
    const auto& startTown = world.towns().front();
    float px = startTown.x + 0.5f;
    float py = startTown.y + 0.5f;
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            int nx = startTown.x + dx, ny = startTown.y + dy;
            if (nx >= 0 && nx < World::WIDTH && ny >= 0 && ny < World::HEIGHT
                && world.tile(nx, ny) == Tile::Ocean) {
                px = nx + 0.5f; py = ny + 0.5f;
                goto foundStart;
            }
        }
    }
    foundStart:;

    Uint32 lastTick = SDL_GetTicks();
    bool running = true;

    while (running) {
        Uint32 now = SDL_GetTicks();
        float  dt  = (now - lastTick) / 1000.0f;
        lastTick   = now;

        // --- Input ---
        input.beginFrame();
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            input.processEvent(e);
        }
        input.endFrame();

        if (input.justPressed(SDL_SCANCODE_ESCAPE)) {
            if (gameState.mode == GameMode::Port) {
                gameState.mode = GameMode::Sailing;
                portScreen.reset();
            } else {
                running = false;
            }
        }

        // --- Sailing mode ---
        if (gameState.mode == GameMode::Sailing) {
            wind.update(dt);

            // Speed control — W/Up cycles up, S/Down cycles down
            if (input.justPressed(SDL_SCANCODE_W) || input.justPressed(SDL_SCANCODE_UP))
                speedLevel = std::min(3, speedLevel + 1);
            if (input.justPressed(SDL_SCANCODE_S) || input.justPressed(SDL_SCANCODE_DOWN))
                speedLevel = std::max(0, speedLevel - 1);

            // Steering — A/Left snaps -45°, D/Right snaps +45°
            if (input.justPressed(SDL_SCANCODE_A) || input.justPressed(SDL_SCANCODE_LEFT))
                playerHeading = std::fmod(playerHeading - 45.0f + 360.0f, 360.0f);
            if (input.justPressed(SDL_SCANCODE_D) || input.justPressed(SDL_SCANCODE_RIGHT))
                playerHeading = std::fmod(playerHeading + 45.0f, 360.0f);

            windSpeedFactor = wind.speedFactor(playerHeading);
            clouds.update(dt, wind);

            auto enterPort = [&](int idx) {
                gameState.mode            = GameMode::Port;
                gameState.activePortIndex = idx;
                portScreen = std::make_unique<PortScreen>(
                    uiRenderer, ship, world.towns()[idx], ports[idx]);
            };

            // Ship moves at chosen speed tier, scaled by wind
            float speed   = PLAYER_SPEED * SPEED_TIERS[speedLevel] * windSpeedFactor;
            float headRad = playerHeading * 3.14159265f / 180.0f;
            float newPx   = std::max(0.5f, std::min(px + std::sinf(headRad) * speed * dt, World::WIDTH  - 0.5f));
            float newPy   = std::max(0.5f, std::min(py - std::cosf(headRad) * speed * dt, World::HEIGHT - 0.5f));

            // X axis
            int txX = std::max(0, std::min((int)newPx, World::WIDTH  - 1));
            int tyX = std::max(0, std::min((int)py,    World::HEIGHT - 1));
            int portX = world.townAt(txX, tyX);
            if      (portX >= 0)                          enterPort(portX);
            else if (world.tile(txX, tyX) == Tile::Ocean) px = newPx;

            // Y axis
            int txY = std::max(0, std::min((int)px,    World::WIDTH  - 1));
            int tyY = std::max(0, std::min((int)newPy, World::HEIGHT - 1));
            int portY = world.townAt(txY, tyY);
            if      (portY >= 0)                          enterPort(portY);
            else if (world.tile(txY, tyY) == Tile::Ocean) py = newPy;

            // Camera
            float camX = std::max(0.0f, std::min(px * TILE_SIZE - SCREEN_W / 2.0f,
                                                  World::WIDTH  * TILE_SIZE - (float)SCREEN_W));
            float camY = std::max(0.0f, std::min(py * TILE_SIZE - SCREEN_H / 2.0f,
                                                  World::HEIGHT * TILE_SIZE - (float)SCREEN_H));

            // --- Render world ---
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);

            int tx0 = (int)(camX / TILE_SIZE);
            int ty0 = (int)(camY / TILE_SIZE);
            int tx1 = tx0 + SCREEN_W / TILE_SIZE + 2;
            int ty1 = ty0 + SCREEN_H / TILE_SIZE + 2;

            for (int ty = ty0; ty <= ty1; ++ty) {
                for (int tx = tx0; tx <= tx1; ++tx) {
                    if (tx < 0 || tx >= World::WIDTH || ty < 0 || ty >= World::HEIGHT) continue;
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

            // Towns
            SDL_SetRenderDrawColor(renderer, 220, 60, 60, 255);
            for (const auto& town : world.towns()) {
                int sx = (int)(town.x * TILE_SIZE - camX);
                int sy = (int)(town.y * TILE_SIZE - camY);
                SDL_Rect r{ sx + 6, sy + 6, TILE_SIZE - 12, TILE_SIZE - 12 };
                SDL_RenderFillRect(renderer, &r);
            }

            // Player
            // Draw player as a filled triangle pointing in heading direction
            int   psx  = (int)(px * TILE_SIZE - camX);
            int   psy  = (int)(py * TILE_SIZE - camY);
            float hRad = playerHeading * 3.14159265f / 180.0f;
            int   sz   = TILE_SIZE / 3;

            // Triangle: front tip, back-left, back-right
            int  fx  = psx + (int)(std::sinf(hRad) * sz);
            int  fy  = psy - (int)(std::cosf(hRad) * sz);
            int  blx = psx + (int)(std::sinf(hRad + 2.4f) * sz * 0.65f);
            int  bly = psy - (int)(std::cosf(hRad + 2.4f) * sz * 0.65f);
            int  brx = psx + (int)(std::sinf(hRad - 2.4f) * sz * 0.65f);
            int  bry = psy - (int)(std::cosf(hRad - 2.4f) * sz * 0.65f);

            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            for (int i = 0; i <= 20; ++i) {
                float t = i / 20.0f;
                SDL_RenderDrawLine(renderer, fx, fy,
                    blx + (int)((brx - blx) * t),
                    bly + (int)((bry - bly) * t));
            }

            clouds.render(renderer, camX, camY);
            hud.render(ship, wind, windSpeedFactor, speedLevel);

        // --- Port mode ---
        } else if (gameState.mode == GameMode::Port && portScreen) {
            portScreen->handleInput(input);

            if (portScreen->wantsToLeave()) {
                // Point ship away from the port so it doesn't immediately re-enter
                const auto& t = world.towns()[gameState.activePortIndex];
                float awayX = px - (t.x + 0.5f);
                float awayY = py - (t.y + 0.5f);
                if (awayX != 0.0f || awayY != 0.0f)
                    playerHeading = std::fmod(
                        std::atan2f(awayX, -awayY) * 180.0f / 3.14159265f + 360.0f, 360.0f);
                // Snap to nearest 45° so it aligns with the steering system
                playerHeading = std::fmod(std::round(playerHeading / 45.0f) * 45.0f, 360.0f);
                speedLevel = 1; // start at slow so player has control

                gameState.mode = GameMode::Sailing;
                portScreen.reset();
            } else {
                // Render world behind port overlay
                float camX = std::max(0.0f, std::min(px * TILE_SIZE - SCREEN_W / 2.0f,
                                                      World::WIDTH  * TILE_SIZE - (float)SCREEN_W));
                float camY = std::max(0.0f, std::min(py * TILE_SIZE - SCREEN_H / 2.0f,
                                                      World::HEIGHT * TILE_SIZE - (float)SCREEN_H));
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderClear(renderer);

                int tx0 = (int)(camX / TILE_SIZE), ty0 = (int)(camY / TILE_SIZE);
                int tx1 = tx0 + SCREEN_W / TILE_SIZE + 2;
                int ty1 = ty0 + SCREEN_H / TILE_SIZE + 2;
                for (int ty = ty0; ty <= ty1; ++ty) {
                    for (int tx = tx0; tx <= tx1; ++tx) {
                        if (tx < 0 || tx >= World::WIDTH || ty < 0 || ty >= World::HEIGHT) continue;
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

                portScreen->render();
            }
        }

        SDL_RenderPresent(renderer);
    }

    portScreen.reset();
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
