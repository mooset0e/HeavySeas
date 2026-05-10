#pragma once
#include "ship/EnemyShip.h"

enum class GameMode { Sailing, Port, Encounter, Combat };

struct GameState {
    GameMode mode            = GameMode::Sailing;
    int      activePortIndex = -1;
    // The enemy currently in an encounter or combat — generated on demand.
    // Valid only when mode == Encounter or mode == Combat.
    EnemyShip activeEnemy;
};
