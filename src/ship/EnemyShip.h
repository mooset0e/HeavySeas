#pragma once
#include "core/Nation.h"
#include "ship/ShipType.h"

enum class Faction { Navy, Merchant, Pirate };

enum class AIState {
    Patrolling, Chasing, Firing, Boarding, Fleeing, ReturningHome
};

struct PatrolZone {
    float cx = 0, cy = 0;
    float radius = 8.0f;
};

struct EnemyShip {
    float x = 0, y = 0;
    float heading  = 0.0f;

    Nation  nation;
    Nation  displayNation;   // may be false colors for pirates
    Faction faction;
    ShipClass  shipClass;
    CannonTier cannonTier;

    int   hullMax   = 0, hullCur  = 0;
    int   crewMax   = 0, crewCur  = 0;
    int   cargoMax  = 0, cargoCur = 0;
    int   gold      = 0;
    int   cannonCount = 0;
    float morale    = 100.0f;

    AIState    state       = AIState::Patrolling;
    PatrolZone patrol;
    float      chaseTimer  = 0.0f;  // how long chasing without player sighting
    float      reloadTimer = 0.0f;  // 0 = ready to fire
    float      boardTimer  = 0.0f;  // countdown between boarding rounds

    float destX = 0, destY = 0;    // current movement destination
    int   portA = -1, portB = -1;  // merchant trade route endpoints
    bool  headingToA = true;        // which end the merchant is heading toward

    bool isVisible = false;   // if true, drawn on the sailing map (named/plot ships only)

    bool alive()    const { return hullCur > 0 && crewCur > 0; }
    bool defeated() const { return hullCur <= 0 || crewCur <= 0; }
};
