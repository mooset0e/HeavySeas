#include "combat/CombatSystem.h"
#include <cmath>
#include <random>
#include <algorithm>

static std::mt19937& rng() {
    static std::mt19937 g(std::random_device{}());
    return g;
}

static float frand(float lo, float hi) {
    return std::uniform_real_distribution<float>(lo, hi)(rng());
}

bool rollHit(float accuracy, float dist, float range, float morale) {
    float p = accuracy * (1.0f - dist / range) * (morale / 100.0f);
    p = std::max(0.0f, p);
    return frand(0.0f, 1.0f) < p;
}

int rollDamage(int baseDamage) {
    return baseDamage + (int)frand(-2.0f, 3.0f);
}

float evasionChance(ShipClass playerClass, float playerSpeed, float enemySpeed,
                    float playerHullRatio, float playerWind, float enemyWind)
{
    static const float CLASS_MOD[] = { 1.3f, 1.1f, 0.9f, 0.7f, 0.5f };
    float speedAdv = std::min(playerSpeed / std::max(enemySpeed, 0.01f), 1.5f);
    float classMod = CLASS_MOD[(int)playerClass];
    float windMod  = playerWind  / std::max(enemyWind, 0.01f);
    float p = speedAdv * classMod * playerHullRatio * windMod;
    return std::min(p, 0.95f);
}

BoardingRound doBoardingRound(int playerCrew, float playerMorale,
                               int enemyCrew,  float enemyMorale)
{
    float pAtk = playerCrew * (playerMorale / 100.0f) * frand(0.8f, 1.2f);
    float eAtk = enemyCrew  * (enemyMorale  / 100.0f) * frand(0.8f, 1.2f);
    BoardingRound r;
    r.playerCrewLost = std::max(0, (int)(eAtk * 0.15f));
    r.enemyCrewLost  = std::max(0, (int)(pAtk * 0.15f));
    return r;
}

bool captainSurvives(const Captain& captain) {
    float ageMod;
    if      (captain.age < 35) ageMod = 1.0f;
    else if (captain.age < 55) ageMod = 0.8f;
    else if (captain.age < 70) ageMod = 0.5f;
    else                       ageMod = 0.25f;
    float skillBonus = captain.navigationSkill * 0.015f;
    float p = (captain.health / 100.0f) * ageMod + skillBonus;
    return frand(0.0f, 1.0f) < p;
}
