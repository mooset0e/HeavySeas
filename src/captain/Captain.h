#pragma once
#include <string>

struct Captain {
    std::string name = "Jack Holt";
    int         age  = 25;
    float       health = 100.0f;   // 0–100; permanently reduced on near-death

    int navigationSkill = 0;       // 0–10
    int combatSkill     = 0;
    int tradeSkill      = 0;

    bool hasLookout = false;
    bool hasSpotter = false;
};
