#pragma once
#include <vector>
#include "ui/UIRenderer.h"
#include "ship/ShipStats.h"
#include "ship/EnemyShip.h"
#include "world/Wind.h"
#include "combat/CombatState.h"
#include "combat/CombatSystem.h"

class InputState;

class CombatScreen {
public:
    static constexpr int   ARENA_W = 40;
    static constexpr int   ARENA_H = 22;
    static constexpr int   TILE_PX = 32;
    static constexpr int   ARENA_Y = (720 - ARENA_H * TILE_PX) / 2;

    struct Fighter {
        float x = 0, y = 0;
        float heading     = 0.0f;
        float reloadTimer = 0.0f;
        float boardTimer  = 0.0f;
        float morale      = 100.0f;
        int   hullCur = 0, hullMax = 0;
        int   crewCur = 0, crewMax = 0;
        int   cargoCur = 0;
        int   gold    = 0;
        ShipClass  shipClass  = ShipClass::Sloop;
        CannonTier cannonTier = CannonTier::Light;
        Faction    faction    = Faction::Pirate;
    };

    struct Cannonball {
        float x, y;
        float vx, vy;
        float lifetime;
        float travelDist    = 1.0f;   // total distance to target (for arc progress)
        float distTraveled  = 0.0f;
        bool  fromPlayer;
        bool  willHit;
        int   damage;
        bool  active = true;
    };

    CombatScreen(UIRenderer& ui, const ShipStats& player,
                 const EnemyShip& enemy, const Wind& wind);

    void update(float dt, const InputState& input, Wind& wind);
    void render(const Wind& wind);

    bool          isOver()        const { return outcome_ != CombatOutcome::Ongoing; }
    CombatOutcome outcome()       const { return outcome_; }
    int           goldLooted()    const { return gold_; }
    bool          sackingFlagged() const { return sacked_; }  // main.cpp adds infamy if true

    const Fighter& playerState() const { return player_; }
    const Fighter& enemyState()  const { return enemy_; }

private:
    UIRenderer&          ui_;
    Fighter              player_;
    Fighter              enemy_;
    int                  playerSpeedLevel_ = 1;
    CombatOutcome        outcome_ = CombatOutcome::Ongoing;
    int                  gold_    = 0;
    bool                 sacked_  = false;
    std::vector<Cannonball> cannonballs_;

    // Surrender state
    bool surrenderOffered_  = false;   // flag offered at least once this combat
    bool surrenderPending_  = false;   // UI currently visible, combat paused
    int  surrenderMenuIdx_  = 0;

    void updateEnemyAI(float dt, const Wind& wind);
    void updateCannonballs(float dt);
    void fireVolley(bool fromPlayer, float firerX, float firerY,
                    float targetX, float targetY,
                    CannonTier tier, float morale,
                    float crewAccuracyMult, float broadsideMult);
    void checkSurrenderTrigger();
    void checkEscapeAndOutcomes();

    void renderBackground();
    void renderFighter(const Fighter& f, bool isPlayer);
    void renderCannonballs();
    void renderSurrenderUI();
    void renderHUD(const Wind& wind);
    void drawLine(int x1, int y1, int x2, int y2, SDL_Color c);

    int asx(float tx) const { return (int)(tx * TILE_PX); }
    int asy(float ty) const { return ARENA_Y + (int)(ty * TILE_PX); }

    static float distBetween(const Fighter& a, const Fighter& b);
    static float bearingTo(const Fighter& from, const Fighter& to);
    static bool  inBroadside(float shooterHeading, float bearingToTarget);
    static float broadsideQuality(float shooterHeading, float bearingToTarget);
    static void  steerToBearing(Fighter& f, float targetBearing, float dt);

    static constexpr float PI                = 3.14159265f;
    static constexpr float BALL_SPEED        = 18.0f;
    // All ship movement in the combat arena is scaled down so positioning matters.
    // At 0.25: a sloop at full sail with good wind does ~2.25 tiles/sec,
    // crossing the 40-tile arena in ~18 seconds.
    static constexpr float COMBAT_SPEED_SCALE = 0.25f;
};
