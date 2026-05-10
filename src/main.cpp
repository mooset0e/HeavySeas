#include <SDL.h>
#include <SDL_ttf.h>
#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "core/Constants.h"
#include "core/GameState.h"
#include "core/InputState.h"
#include "core/Nation.h"
#include "ship/ShipStats.h"
#include "ship/ShipType.h"
#include "ship/EnemyShip.h"
#include "ship/EnemyManager.h"
#include "world/World.h"
#include "world/Port.h"
#include "world/Wind.h"
#include "combat/CombatSystem.h"
#include "combat/CombatState.h"
#include "captain/Captain.h"
#include "captain/GameTime.h"
#include "encounter/EncounterGenerator.h"
#include "ui/UIRenderer.h"
#include "ui/HUD.h"
#include "ui/PortScreen.h"
#include "ui/EncounterScreen.h"
#include "ui/CombatScreen.h"
#include "ui/CloudSystem.h"

static constexpr float PI = 3.14159265f;

// ---- utilities ----

static float shipDist(float ax, float ay, float bx, float by) {
    float dx = ax - bx, dy = ay - by;
    return std::sqrtf(dx * dx + dy * dy);
}

static int nearestPortIndex(float px, float py, const std::vector<Town>& towns) {
    int   best = 0;
    float bestD = 1e9f;
    for (int i = 0; i < (int)towns.size(); ++i) {
        float d = shipDist(px, py, (float)towns[i].x, (float)towns[i].y);
        if (d < bestD) { bestD = d; best = i; }
    }
    return best;
}

// Render world tiles, shipping routes, and ports
static void renderWorld(SDL_Renderer* renderer, const World& world, float camX, float camY) {
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

    // Shipping routes — A*-computed ocean-only paths (debug; will be hidden later)
    SDL_SetRenderDrawColor(renderer, 20, 70, 130, 255);
    for (const auto& route : world.routes()) {
        const auto& path = route.path;
        for (int i = 0; i + 1 < (int)path.size(); ++i) {
            int sx1 = (int)((path[i].first   + 0.5f) * TILE_SIZE - camX);
            int sy1 = (int)((path[i].second  + 0.5f) * TILE_SIZE - camY);
            int sx2 = (int)((path[i+1].first + 0.5f) * TILE_SIZE - camX);
            int sy2 = (int)((path[i+1].second+ 0.5f) * TILE_SIZE - camY);
            SDL_RenderDrawLine(renderer, sx1, sy1, sx2, sy2);
        }
    }

    // Ports
    SDL_SetRenderDrawColor(renderer, 220, 60, 60, 255);
    for (const auto& town : world.towns()) {
        int sx = (int)(town.x * TILE_SIZE - camX);
        int sy = (int)(town.y * TILE_SIZE - camY);
        SDL_Rect r{ sx + 6, sy + 6, TILE_SIZE - 12, TILE_SIZE - 12 };
        SDL_RenderFillRect(renderer, &r);
    }
}

// Render player ship triangle
static void renderPlayer(SDL_Renderer* renderer, float px, float py,
                          float heading, float camX, float camY) {
    int psx = (int)(px * TILE_SIZE - camX);
    int psy = (int)(py * TILE_SIZE - camY);
    float hRad = heading * PI / 180.0f;
    int   sz   = TILE_SIZE / 3;

    int fx  = psx + (int)(std::sinf(hRad) * sz);
    int fy  = psy - (int)(std::cosf(hRad) * sz);
    int blx = psx + (int)(std::sinf(hRad + 2.4f) * sz * 0.65f);
    int bly = psy - (int)(std::cosf(hRad + 2.4f) * sz * 0.65f);
    int brx = psx + (int)(std::sinf(hRad - 2.4f) * sz * 0.65f);
    int bry = psy - (int)(std::cosf(hRad - 2.4f) * sz * 0.65f);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int i = 0; i <= 20; ++i) {
        float t = i / 20.0f;
        SDL_RenderDrawLine(renderer, fx, fy,
            blx + (int)((brx - blx) * t),
            bly + (int)((bry - bly) * t));
    }
}

// ---- main ----

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init: " << SDL_GetError() << "\n";
        return 1;
    }
    if (TTF_Init() != 0) {
        std::cerr << "TTF_Init: " << TTF_GetError() << "\n";
        SDL_Quit(); return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Heavy Seas",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_W, SCREEN_H, SDL_WINDOW_SHOWN);
    if (!window) { std::cerr << SDL_GetError(); TTF_Quit(); SDL_Quit(); return 1; }

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) { std::cerr << SDL_GetError(); SDL_DestroyWindow(window); TTF_Quit(); SDL_Quit(); return 1; }

    TTF_Font* font = TTF_OpenFont("assets/fonts/main.ttf", 18);
    if (!font) {
        std::cerr << "TTF_OpenFont: " << TTF_GetError() << "\n";
        SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); TTF_Quit(); SDL_Quit(); return 1;
    }

    // ---- World ----
    World world;
    world.generate(12345);

    std::vector<Port> ports(world.towns().size());
    for (int i = 0; i < (int)ports.size(); ++i) {
        ports[i].townIndex          = i;
        ports[i].nation             = world.towns()[i].nation;
        ports[i].repairCostPerHP    = 5;
        ports[i].hireCostPerCrew    = 20;
        ports[i].sellsHeavyCannons  = (i % 4 == 0);
    }

    // ---- Game objects ----
    ShipStats  ship;
    Captain    captain;
    GameTime   gameTime;
    GameState  gameState;
    InputState input;
    Wind       wind;

    // EnemyManager holds only visible plot/mission ships (empty at game start)
    EnemyManager enemies;

    bool  navyHostile = false;
    bool  gameOver    = false;
    std::string gameOverMsg;

    float playerHeading   = 0.0f;
    float windSpeedFactor = 1.0f;
    int   speedLevel      = 1;
    static constexpr float SPEED_TIERS[] = { 0.0f, 0.35f, 0.65f, 1.0f };

    // JRPG encounter accumulation
    float encounterAccum    = 0.0f;
    float encounterCooldown = 0.0f;   // seconds before the next encounter can trigger

    UIRenderer  uiRenderer(renderer, font);
    HUD         hud(uiRenderer);
    CloudSystem clouds;

    std::unique_ptr<PortScreen>      portScreen;
    std::unique_ptr<EncounterScreen> encounterScreen;
    std::unique_ptr<CombatScreen>    combatScreen;

    // Player starts near first town, on an ocean tile
    const auto& startTown = world.towns().front();
    float px = startTown.x + 0.5f;
    float py = startTown.y + 0.5f;
    for (int dy = -2; dy <= 2; ++dy)
        for (int dx = -2; dx <= 2; ++dx) {
            int nx = startTown.x + dx, ny = startTown.y + dy;
            if (nx >= 0 && nx < World::WIDTH && ny >= 0 && ny < World::HEIGHT
                && world.tile(nx, ny) == Tile::Ocean) {
                px = nx + 0.5f; py = ny + 0.5f;
                goto foundStart;
            }
        }
    foundStart:;

    Uint32 lastTick = SDL_GetTicks();
    bool running = true;

    while (running) {
        Uint32 now = SDL_GetTicks();
        float  dt  = std::min((now - lastTick) / 1000.0f, 0.05f);
        lastTick   = now;

        // ---- Input ----
        input.beginFrame();
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            input.processEvent(e);
        }
        input.endFrame();

        // ---- Game-over screen ----
        if (gameOver) {
            if (input.justPressed(SDL_SCANCODE_RETURN) || input.justPressed(SDL_SCANCODE_ESCAPE))
                running = false;
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            uiRenderer.drawTextCentered("GAME OVER", { 0, SCREEN_H/2 - 40, SCREEN_W, 50 },
                                         { 220, 50, 50, 255 });
            uiRenderer.drawTextCentered(gameOverMsg, { 0, SCREEN_H/2 + 20, SCREEN_W, 30 },
                                         { 200, 180, 140, 255 });
            uiRenderer.drawTextCentered("Press Enter or Esc to exit",
                                         { 0, SCREEN_H/2 + 60, SCREEN_W, 30 }, { 90, 100, 100, 255 });
            SDL_RenderPresent(renderer);
            continue;
        }

        // Camera
        float camX = std::max(0.0f, std::min(px * TILE_SIZE - SCREEN_W / 2.0f,
                                              World::WIDTH  * TILE_SIZE - (float)SCREEN_W));
        float camY = std::max(0.0f, std::min(py * TILE_SIZE - SCREEN_H / 2.0f,
                                              World::HEIGHT * TILE_SIZE - (float)SCREEN_H));

        // ================================================================
        //  UPDATE
        // ================================================================

        // ===== SAILING =====
        if (gameState.mode == GameMode::Sailing) {
            if (input.justPressed(SDL_SCANCODE_ESCAPE)) running = false;

            wind.update(dt);
            windSpeedFactor = wind.speedFactor(playerHeading);
            clouds.update(dt, wind);

            if (input.justPressed(SDL_SCANCODE_W) || input.justPressed(SDL_SCANCODE_UP))
                speedLevel = std::min(3, speedLevel + 1);
            if (input.justPressed(SDL_SCANCODE_S) || input.justPressed(SDL_SCANCODE_DOWN))
                speedLevel = std::max(0, speedLevel - 1);
            if (input.justPressed(SDL_SCANCODE_A) || input.justPressed(SDL_SCANCODE_LEFT))
                playerHeading = std::fmodf(playerHeading - 45.0f + 360.0f, 360.0f);
            if (input.justPressed(SDL_SCANCODE_D) || input.justPressed(SDL_SCANCODE_RIGHT))
                playerHeading = std::fmodf(playerHeading + 45.0f, 360.0f);

            float speed   = PLAYER_SPEED * SPEED_TIERS[speedLevel] * windSpeedFactor;
            float headRad = playerHeading * PI / 180.0f;
            float moved   = speed * dt;

            float newPx = std::max(0.5f, std::min(px + std::sinf(headRad) * moved,
                                                   World::WIDTH  - 0.5f));
            float newPy = std::max(0.5f, std::min(py - std::cosf(headRad) * moved,
                                                   World::HEIGHT - 0.5f));

            auto tryEnterPort = [&](float npx, float npy) -> int {
                int tx = std::max(0, std::min((int)npx, World::WIDTH  - 1));
                int ty = std::max(0, std::min((int)npy, World::HEIGHT - 1));
                return world.townAt(tx, ty);
            };
            auto enterPort = [&](int idx) {
                gameState.mode = GameMode::Port;
                gameState.activePortIndex = idx;
                portScreen = std::make_unique<PortScreen>(
                    uiRenderer, ship, captain, world.towns()[idx], ports[idx]);
                gameTime.advanceDays(1);
            };

            int portX = tryEnterPort(newPx, py);
            if (portX >= 0) { enterPort(portX); }
            else if (world.tile(std::max(0,std::min((int)newPx,World::WIDTH-1)),
                                std::max(0,std::min((int)py,World::HEIGHT-1))) == Tile::Ocean)
                px = newPx;

            if (gameState.mode == GameMode::Sailing) {
                int portY = tryEnterPort(px, newPy);
                if (portY >= 0) { enterPort(portY); }
                else if (world.tile(std::max(0,std::min((int)px,World::WIDTH-1)),
                                    std::max(0,std::min((int)newPy,World::HEIGHT-1))) == Tile::Ocean)
                    py = newPy;
            }

            ship.reloadTimer = std::max(0.0f, ship.reloadTimer - dt);

            if (gameState.mode != GameMode::Sailing) goto endUpdate;

            // ---- Plot ship encounter trigger ----
            enemies.update(dt, px, py, wind);
            {
                int plotIdx = enemies.checkEncounterTrigger(px, py);
                if (plotIdx >= 0) {
                    gameState.activeEnemy = enemies.ships()[plotIdx];
                    // Prevent re-triggering while in encounter
                    enemies.ships()[plotIdx].state = AIState::Fleeing;
                    encounterScreen = std::make_unique<EncounterScreen>(
                        uiRenderer, gameState.activeEnemy, ship, captain);
                    encounterCooldown = 30.0f;
                    gameState.mode = GameMode::Encounter;
                    goto endUpdate;
                }
            }

            // ---- JRPG encounter tick ----
            encounterCooldown = std::max(0.0f, encounterCooldown - dt);
            if (speedLevel > 0 && encounterCooldown <= 0.0f) {
                float rProx = world.routeProximity(px, py);

                // Quadratic curve: rate is high on routes, drops steeply in open water.
                // On route (rProx=1):  ~0.067/tile → encounter every ~15 tiles
                // Route edge (rProx=0.5): ~0.019/tile → every ~53 tiles
                // Open ocean (rProx=0): ~0.003/tile → every ~333 tiles
                float baseRate   = 0.003f + rProx * rProx * 0.064f;
                float repMult    = 1.0f + (ship.rep.infamy / 50.0f);
                float bountyMult = (ship.rep.bounty > 300) ? 1.5f : 1.0f;

                encounterAccum += moved * baseRate * repMult * bountyMult;

                if (encounterAccum >= 1.0f) {
                    encounterAccum    = 0.0f;
                    encounterCooldown = 25.0f;

                    int   npIdx = nearestPortIndex(px, py, world.towns());
                    float pDist = shipDist(px, py,
                                          (float)world.towns()[npIdx].x,
                                          (float)world.towns()[npIdx].y);
                    EncounterContext ctx;
                    ctx.worldX         = px;
                    ctx.worldY         = py;
                    ctx.routeProximity = rProx;
                    ctx.portProximity  = std::max(0.0f, 1.0f - pDist / 8.0f);
                    ctx.nearestNation  = world.nearestPortNation(px, py);
                    ctx.infamy         = ship.rep.infamy;
                    ctx.bounty         = ship.rep.bounty;
                    ctx.navyHostile    = navyHostile;

                    gameState.activeEnemy = generateEncounter(ctx, (unsigned int)std::rand());
                    encounterScreen = std::make_unique<EncounterScreen>(
                        uiRenderer, gameState.activeEnemy, ship, captain);
                    gameState.mode = GameMode::Encounter;
                }
            }

        // ===== PORT =====
        } else if (gameState.mode == GameMode::Port && portScreen) {
            portScreen->handleInput(input);
            if (portScreen->wantsToLeave()) {
                const auto& t = world.towns()[gameState.activePortIndex];
                float awayX = px - (t.x + 0.5f);
                float awayY = py - (t.y + 0.5f);
                if (awayX != 0.0f || awayY != 0.0f)
                    playerHeading = std::fmodf(
                        std::atan2f(awayX, -awayY) * 180.0f / PI + 360.0f, 360.0f);
                playerHeading = std::fmodf(std::roundf(playerHeading / 45.0f) * 45.0f, 360.0f);
                speedLevel    = 1;
                encounterCooldown = 15.0f;   // brief grace period after leaving port
                gameState.mode = GameMode::Sailing;
                portScreen.reset();
            }

        // ===== ENCOUNTER =====
        } else if (gameState.mode == GameMode::Encounter && encounterScreen) {
            wind.update(dt);
            clouds.update(dt, wind);

            encounterScreen->handleInput(input);
            EncounterChoice choice = encounterScreen->choice();

            if (choice == EncounterChoice::Engage) {
                combatScreen = std::make_unique<CombatScreen>(
                    uiRenderer, ship, gameState.activeEnemy, wind);
                gameState.mode = GameMode::Combat;
                encounterScreen.reset();

            } else if (choice == EncounterChoice::Hail) {
                EnemyShip& ae = gameState.activeEnemy;
                if (ae.faction == Faction::Merchant) {
                    int gained = ae.cargoCur + ae.gold / 3;
                    ship.gold += gained;
                    encounterScreen->resultMsg =
                        "You pull alongside the merchant.\n"
                        "They exchange goods for safe passage.\n\n"
                        "Gold gained: " + std::to_string(gained);
                    // Screen waits for keypress, then choice_ becomes Flee → back to sailing
                } else if (ae.faction == Faction::Navy && !navyHostile) {
                    encounterScreen->resultMsg =
                        "The naval vessel inspects your papers.\n"
                        "All is well. They signal you to sail on.";
                } else {
                    // Navy hostile or pirate — they don't want to talk
                    combatScreen = std::make_unique<CombatScreen>(
                        uiRenderer, ship, ae, wind);
                    gameState.mode = GameMode::Combat;
                    encounterScreen.reset();
                }
                // If resultMsg was set, EncounterScreen waits for keypress then
                // returns choice Flee, which will be caught next frame to go back to sailing.

            } else if (choice == EncounterChoice::Rob) {
                EnemyShip& ae = gameState.activeEnemy;
                if (ae.faction == Faction::Merchant) {
                    // Intimidation succeeds based on infamy (20–80% chance)
                    float successChance = 0.20f + (ship.rep.infamy / 100.0f) * 0.60f;
                    float roll = (float)(std::rand() % 1000) / 1000.0f;
                    if (roll < successChance) {
                        int looted = ae.cargoCur * 3 + ae.gold;
                        ship.gold += looted;
                        ship.rep.infamy = std::min(100.0f, ship.rep.infamy + 8.0f);
                        ship.rep.bounty += (int)(ship.rep.infamy * 25.0f);
                        encounterScreen->resultMsg =
                            "They surrender without a fight!\n\n"
                            "Gold seized: " + std::to_string(looted) + "\n"
                            "Your infamy grows...";
                    } else {
                        encounterScreen->resultMsg =
                            "The merchant calls your bluff!\n"
                            "They fight back!";
                        // Will drop into combat next keypress via Flee→Sailing path — but we
                        // want combat, so start it immediately and let the result screen show first.
                        combatScreen = std::make_unique<CombatScreen>(
                            uiRenderer, ship, ae, wind);
                        gameState.mode = GameMode::Combat;
                        encounterScreen.reset();
                    }
                } else {
                    // Can't rob navy or pirates without a fight
                    combatScreen = std::make_unique<CombatScreen>(
                        uiRenderer, ship, ae, wind);
                    gameState.mode = GameMode::Combat;
                    encounterScreen.reset();
                }

            } else if (choice == EncounterChoice::Flee) {
                EnemyShip& ae = gameState.activeEnemy;
                float playerSpeed = PLAYER_SPEED * SPEED_TIERS[speedLevel] * windSpeedFactor;
                float enemySpeed  = getShipTypeDef(ae.shipClass).speed
                                    * wind.speedFactor(ae.heading);
                float chance = evasionChance(ship.shipClass, playerSpeed, enemySpeed,
                                              (float)ship.hullCur / ship.hullMax,
                                              windSpeedFactor, wind.speedFactor(ae.heading));
                float roll = (float)(std::rand() % 1000) / 1000.0f;
                if (roll >= chance) {
                    // Failed to flee — forced into combat
                    combatScreen = std::make_unique<CombatScreen>(
                        uiRenderer, ship, ae, wind);
                    gameState.mode = GameMode::Combat;
                } else {
                    gameState.mode = GameMode::Sailing;
                }
                encounterScreen.reset();
            }

        // ===== COMBAT =====
        } else if (gameState.mode == GameMode::Combat && combatScreen) {
            combatScreen->update(dt, input, wind);

            if (combatScreen->isOver()) {
                CombatOutcome outcome = combatScreen->outcome();
                EnemyShip&    ae     = gameState.activeEnemy;

                if (outcome == CombatOutcome::EnemySunk ||
                    outcome == CombatOutcome::EnemyCaptured) {
                    ship.hullCur  = combatScreen->playerState().hullCur;
                    ship.crewCur  = combatScreen->playerState().crewCur;
                    ship.gold    += combatScreen->goldLooted();
                    // Sacking a surrendered ship or sinking non-pirates gains infamy
                    bool gainedInfamy = combatScreen->sackingFlagged() ||
                                        (outcome == CombatOutcome::EnemySunk &&
                                         ae.faction != Faction::Pirate);
                    if (gainedInfamy) {
                        float gain = combatScreen->sackingFlagged() ? 15.0f : 5.0f;
                        ship.rep.infamy = std::min(100.0f, ship.rep.infamy + gain);
                        ship.rep.bounty += (int)(ship.rep.infamy * 20.0f);
                    }
                    if (!navyHostile && ae.faction == Faction::Navy)
                        navyHostile = true;
                    gameState.mode = GameMode::Sailing;

                } else if (outcome == CombatOutcome::PlayerSunk) {
                    ship.hullCur = combatScreen->playerState().hullCur;
                    ship.crewCur = combatScreen->playerState().crewCur;
                    if (captainSurvives(captain)) {
                        int lostHealth = 10 + (std::rand() % 11);
                        captain.health = std::max(0.0f, captain.health - lostHealth);
                        gameTime.advanceDays(4 + std::rand() % 4);
                        int np = nearestPortIndex(px, py, world.towns());
                        px = (float)world.towns()[np].x + 0.5f;
                        py = (float)world.towns()[np].y + 0.5f;
                        ship.hullCur = ship.hullMax / 4;
                        ship.crewCur = std::max(ship.crewMax / 5, ship.crewCur / 2);
                        gameState.mode = GameMode::Port;
                        gameState.activePortIndex = np;
                        portScreen = std::make_unique<PortScreen>(
                            uiRenderer, ship, captain, world.towns()[np], ports[np]);
                    } else {
                        gameOver    = true;
                        gameOverMsg = captain.name + " has drowned at sea.";
                    }

                } else if (outcome == CombatOutcome::PlayerCaptured) {
                    ship.gold    = 0;
                    ship.crewCur = ship.crewMax / 4;
                    ship.hullCur = std::max(1, ship.hullMax / 4);
                    int np = nearestPortIndex(px, py, world.towns());
                    px = (float)world.towns()[np].x + 0.5f;
                    py = (float)world.towns()[np].y + 0.5f;
                    gameState.mode = GameMode::Port;
                    gameState.activePortIndex = np;
                    portScreen = std::make_unique<PortScreen>(
                        uiRenderer, ship, captain, world.towns()[np], ports[np]);

                } else {
                    // Escaped — no stat changes
                    gameState.mode = GameMode::Sailing;
                }

                combatScreen.reset();
            }
        }

        endUpdate:;

        // ================================================================
        //  RENDER
        // ================================================================
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        if (gameState.mode == GameMode::Combat && combatScreen) {
            combatScreen->render(wind);
        } else if (gameState.mode == GameMode::Port && portScreen) {
            renderWorld(renderer, world, camX, camY);
            portScreen->render();
        } else {
            renderWorld(renderer, world, camX, camY);
            enemies.render(renderer, camX, camY);
            renderPlayer(renderer, px, py, playerHeading, camX, camY);
            clouds.render(renderer, camX, camY);
            hud.render(ship, wind, windSpeedFactor, speedLevel, gameTime,
                       (int)enemies.ships().size());
            if (gameState.mode == GameMode::Encounter && encounterScreen)
                encounterScreen->render();
        }

        SDL_RenderPresent(renderer);
    }

    portScreen.reset();
    encounterScreen.reset();
    combatScreen.reset();
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
