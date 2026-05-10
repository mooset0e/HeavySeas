#pragma once
#include "ship/EnemyShip.h"
#include "ship/Reputation.h"
#include "core/Nation.h"

struct EncounterContext {
    float  worldX, worldY;
    float  routeProximity;   // 0–1
    float  portProximity;    // 0–1  (1 = right next to a port)
    Nation nearestNation;
    float  infamy;
    int    bounty;
    bool   navyHostile;
};

// Generates a random enemy ship appropriate for the current context.
// All generated ships have isVisible = false (they live only during the encounter).
EnemyShip generateEncounter(const EncounterContext& ctx, unsigned int seed);
