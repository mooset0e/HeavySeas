#pragma once
#include <string>

enum class ShipClass { Sloop, Brigantine, Frigate, Galleon, ManOWar };

enum class CannonTier { Light, Medium, Heavy };

struct CannonConfig {
    int   damage;
    float range;     // tiles
    float reload;    // seconds
    float accuracy;  // 0–1
};

struct ShipTypeDef {
    ShipClass   shipClass;
    std::string name;
    float       speed;       // base tiles/s
    float       turnRate;    // degrees/s
    int         hullMax;
    int         crewMax;
    int         crewMin;     // minimum crew to operate
    int         cargoMax;
    int         cannonCount;
    CannonTier  cannonTier;
    int         buyCost;     // gold
};

CannonConfig       getCannonConfig(CannonTier tier);
const ShipTypeDef& getShipTypeDef(ShipClass cls);
