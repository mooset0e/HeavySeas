#pragma once
#include <vector>
#include <SDL.h>
#include "ship/EnemyShip.h"
#include "world/Wind.h"

// Manages only visible, plot-significant ships (quest targets, famous pirates, etc.).
// Random encounters are handled separately by the JRPG-style encounter tick in main.cpp.
class EnemyManager {
public:
    void addPlotShip(EnemyShip ship);
    void update(float dt, float px, float py, const Wind& wind);
    void render(SDL_Renderer* renderer, float camX, float camY) const;

    std::vector<EnemyShip>&       ships()       { return ships_; }
    const std::vector<EnemyShip>& ships() const { return ships_; }

    // Returns index of the first plot ship that has entered encounter range, -1 otherwise.
    int  checkEncounterTrigger(float px, float py);
    void clearTrigger() { trigger_ = -1; }

private:
    std::vector<EnemyShip> ships_;
    int                    trigger_ = -1;

    static constexpr float ENCOUNTER_RANGE = 2.5f;  // tiles
    static constexpr float PI              = 3.14159265f;

    static float dist(float ax, float ay, float bx, float by);
    static void  steerToward(EnemyShip& e, float tx, float ty, float dt);
    static void  moveShip(EnemyShip& e, float dt, const Wind& wind);
};
