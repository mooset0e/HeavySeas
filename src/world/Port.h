#pragma once
#include "core/Nation.h"

struct Port {
    int    townIndex        = -1;
    Nation nation           = Nation::Independent;
    int    repairCostPerHP  = 5;    // gold per missing hull point
    int    hireCostPerCrew  = 20;   // gold per crew member
    bool   sellsHeavyCannons = false;
    int    repWithPlayer    = 0;    // -100 to 100
};
