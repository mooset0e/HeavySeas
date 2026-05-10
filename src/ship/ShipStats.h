#pragma once
#include <string>

struct ShipStats {
    std::string name     = "The Intrepid";
    int hullMax  = 100;
    int hullCur  = 100;
    int crewMax  = 50;
    int crewCur  = 25;
    int cargoMax = 200;
    int cargoCur = 0;
    int gold     = 500;
    int cannons  = 8;
};
