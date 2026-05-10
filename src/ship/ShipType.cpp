#include "ship/ShipType.h"
#include <array>

static const std::array<ShipTypeDef, 5> SHIP_TYPES = {{
    { ShipClass::Sloop,      "Sloop",      9.0f, 120.0f,  80,  30, 10,  80,  4, CannonTier::Light,   800 },
    { ShipClass::Brigantine, "Brigantine", 7.5f,  90.0f, 140,  60, 20, 150,  8, CannonTier::Light,  1800 },
    { ShipClass::Frigate,    "Frigate",    6.0f,  60.0f, 240, 120, 40, 200, 16, CannonTier::Medium, 4000 },
    { ShipClass::Galleon,    "Galleon",    4.5f,  40.0f, 360, 180, 60, 500, 12, CannonTier::Medium, 7000 },
    { ShipClass::ManOWar,    "Man-o'-War", 3.5f,  25.0f, 500, 300,100, 150, 24, CannonTier::Heavy, 15000 },
}};

static const std::array<CannonConfig, 3> CANNON_CONFIGS = {{
    {  8,  6.0f, 3.0f, 0.65f },  // Light
    { 14,  9.0f, 5.0f, 0.75f },  // Medium
    { 22, 13.0f, 8.0f, 0.85f },  // Heavy
}};

CannonConfig getCannonConfig(CannonTier tier) {
    return CANNON_CONFIGS[(int)tier];
}

const ShipTypeDef& getShipTypeDef(ShipClass cls) {
    return SHIP_TYPES[(int)cls];
}
