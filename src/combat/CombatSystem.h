#pragma once
#include "ship/ShipStats.h"
#include "ship/EnemyShip.h"
#include "captain/Captain.h"

struct BoardingRound {
    int playerCrewLost;
    int enemyCrewLost;
};

bool          rollHit(float accuracy, float dist, float range, float morale);
int           rollDamage(int baseDamage);
float         evasionChance(ShipClass playerClass, float playerSpeed, float enemySpeed,
                             float playerHullRatio, float playerWind, float enemyWind);
BoardingRound doBoardingRound(int playerCrew, float playerMorale,
                               int enemyCrew,  float enemyMorale);
bool          captainSurvives(const Captain& captain);
