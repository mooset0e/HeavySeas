#pragma once
#include <string>
#include "ship/ShipType.h"
#include "ship/Reputation.h"

struct ShipStats {
    std::string name       = "The Intrepid";
    ShipClass   shipClass  = ShipClass::Sloop;
    CannonTier  cannonTier = CannonTier::Light;

    int   hullMax   = 80;
    int   hullCur   = 80;
    int   crewMax   = 30;
    int   crewCur   = 20;
    int   cargoMax  = 80;
    int   cargoCur  = 0;
    int   gold      = 500;
    int   cannons   = 4;
    float      morale      = 100.0f;
    float      reloadTimer = 0.0f;
    Reputation rep;
};
