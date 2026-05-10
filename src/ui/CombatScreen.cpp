#include "ui/CombatScreen.h"
#include "core/InputState.h"
#include "ship/ShipType.h"
#include <cmath>
#include <algorithm>
#include <string>

static constexpr float SPEED_TIERS[4] = { 0.0f, 0.35f, 0.65f, 1.0f };

// ---- static helpers ----

float CombatScreen::distBetween(const Fighter& a, const Fighter& b) {
    float dx = a.x - b.x, dy = a.y - b.y;
    return std::sqrtf(dx * dx + dy * dy);
}

float CombatScreen::bearingTo(const Fighter& from, const Fighter& to) {
    float a = std::atan2f(to.x - from.x, -(to.y - from.y)) * 180.0f / PI;
    if (a < 0.0f) a += 360.0f;
    return a;
}

bool CombatScreen::inBroadside(float shooterHeading, float bearingToTarget) {
    float diff = bearingToTarget - shooterHeading;
    if (diff >  180.0f) diff -= 360.0f;
    if (diff < -180.0f) diff += 360.0f;
    float abs = std::fabsf(diff);
    return abs > 30.0f && abs < 150.0f;
}

void CombatScreen::steerToBearing(Fighter& f, float targetBearing, float dt) {
    float diff = targetBearing - f.heading;
    if (diff >  180.0f) diff -= 360.0f;
    if (diff < -180.0f) diff += 360.0f;
    float maxTurn = getShipTypeDef(f.shipClass).turnRate * dt;
    f.heading += std::max(-maxTurn, std::min(diff, maxTurn));
    f.heading = std::fmodf(f.heading + 360.0f, 360.0f);
}

void CombatScreen::drawLine(int x1, int y1, int x2, int y2, SDL_Color c) {
    SDL_SetRenderDrawColor(ui_.renderer(), c.r, c.g, c.b, c.a);
    SDL_RenderDrawLine(ui_.renderer(), x1, y1, x2, y2);
}

// ---- constructor ----

CombatScreen::CombatScreen(UIRenderer& ui, const ShipStats& player,
                             const EnemyShip& enemy, const Wind& /*wind*/)
    : ui_(ui)
{
    player_.x          = 4.0f;
    player_.y          = ARENA_H / 2.0f;
    player_.heading    = 90.0f;
    player_.hullCur    = player.hullCur;
    player_.hullMax    = player.hullMax;
    player_.crewCur    = player.crewCur;
    player_.crewMax    = player.crewMax;
    player_.gold       = player.gold;
    player_.morale     = player.morale;
    player_.shipClass  = player.shipClass;
    player_.cannonTier = player.cannonTier;

    enemy_.x          = ARENA_W - 4.0f;
    enemy_.y          = ARENA_H / 2.0f;
    enemy_.heading    = 270.0f;
    enemy_.hullCur    = enemy.hullCur;
    enemy_.hullMax    = enemy.hullMax;
    enemy_.crewCur    = enemy.crewCur;
    enemy_.crewMax    = enemy.crewMax;
    enemy_.gold       = enemy.gold;
    enemy_.cargoCur   = enemy.cargoCur;
    enemy_.morale     = enemy.morale;
    enemy_.shipClass  = enemy.shipClass;
    enemy_.cannonTier = enemy.cannonTier;
    enemy_.faction    = enemy.faction;
}

// ---- cannonball helpers ----

void CombatScreen::fireVolley(bool fromPlayer,
                               float firerX, float firerY,
                               float targetX, float targetY,
                               CannonTier tier, float morale)
{
    float dx   = targetX - firerX;
    float dy   = targetY - firerY;
    float dist = std::sqrtf(dx * dx + dy * dy);
    if (dist < 0.01f) return;

    auto  cannon = getCannonConfig(tier);

    Cannonball cb;
    cb.x          = firerX;
    cb.y          = firerY;
    cb.vx         = dx / dist * BALL_SPEED;
    cb.vy         = dy / dist * BALL_SPEED;
    cb.lifetime   = dist / BALL_SPEED + 0.15f;
    cb.fromPlayer = fromPlayer;
    cb.willHit    = rollHit(cannon.accuracy, dist, cannon.range, morale);
    cb.damage     = cb.willHit ? rollDamage(cannon.damage) : 0;
    cb.active     = true;
    cannonballs_.push_back(cb);
}

void CombatScreen::updateCannonballs(float dt) {
    for (auto& cb : cannonballs_) {
        if (!cb.active) continue;
        cb.x        += cb.vx * dt;
        cb.y        += cb.vy * dt;
        cb.lifetime -= dt;

        Fighter& target = cb.fromPlayer ? enemy_ : player_;
        float dx = cb.x - target.x;
        float dy = cb.y - target.y;
        float d  = std::sqrtf(dx * dx + dy * dy);

        // Detonate when close to target or time runs out
        bool arrived = d < 0.5f;
        bool expired = cb.lifetime <= 0.0f;
        if (arrived || expired) {
            if (arrived && cb.willHit)
                target.hullCur = std::max(0, target.hullCur - cb.damage);
            cb.active = false;
        }
    }
    cannonballs_.erase(
        std::remove_if(cannonballs_.begin(), cannonballs_.end(),
                       [](const Cannonball& c){ return !c.active; }),
        cannonballs_.end());
}

// ---- update ----

void CombatScreen::checkSurrenderTrigger() {
    if (surrenderOffered_ || surrenderPending_) return;
    float hullRatio = (float)enemy_.hullCur / (float)std::max(1, enemy_.hullMax);
    if (hullRatio < 0.25f) {
        surrenderOffered_ = true;
        surrenderPending_ = true;
        surrenderMenuIdx_ = 0;
    }
}

void CombatScreen::update(float dt, const InputState& input, Wind& wind) {
    if (outcome_ != CombatOutcome::Ongoing) return;

    // ---- Surrender UI — combat paused while flag is flying ----
    if (surrenderPending_) {
        if (input.justPressed(SDL_SCANCODE_UP))
            surrenderMenuIdx_ = std::max(0, surrenderMenuIdx_ - 1);
        if (input.justPressed(SDL_SCANCODE_DOWN))
            surrenderMenuIdx_ = std::min(2, surrenderMenuIdx_ + 1);
        if (input.justPressed(SDL_SCANCODE_RETURN)) {
            if (surrenderMenuIdx_ == 0) {
                // Accept surrender — take gold and cargo, spare the crew
                gold_    = enemy_.gold + enemy_.cargoCur * 3 + enemy_.crewCur * 5;
                outcome_ = CombatOutcome::EnemyCaptured;
            } else if (surrenderMenuIdx_ == 1) {
                // Sack & plunder — take everything, sink the ship
                gold_   = enemy_.gold + enemy_.cargoCur * 5 + enemy_.hullMax * 2;
                sacked_ = true;
                outcome_ = CombatOutcome::EnemySunk;
            } else {
                // Refuse — lower the white flag and keep fighting
                surrenderPending_ = false;
            }
        }
        return;
    }

    wind.update(dt);

    // Player turning (held = continuous)
    float maxTurn = getShipTypeDef(player_.shipClass).turnRate * dt;
    if (input.held(SDL_SCANCODE_A) || input.held(SDL_SCANCODE_LEFT))
        player_.heading = std::fmodf(player_.heading - maxTurn + 360.0f, 360.0f);
    if (input.held(SDL_SCANCODE_D) || input.held(SDL_SCANCODE_RIGHT))
        player_.heading = std::fmodf(player_.heading + maxTurn, 360.0f);

    if (input.justPressed(SDL_SCANCODE_W) || input.justPressed(SDL_SCANCODE_UP))
        playerSpeedLevel_ = std::min(3, playerSpeedLevel_ + 1);
    if (input.justPressed(SDL_SCANCODE_S) || input.justPressed(SDL_SCANCODE_DOWN))
        playerSpeedLevel_ = std::max(0, playerSpeedLevel_ - 1);

    // Player movement — ship class speed, sail level, wind, flooding, combat scale.
    {
        float hullRatio = (float)player_.hullCur / (float)player_.hullMax;
        float floodMult = (hullRatio < 0.2f)
            ? 0.05f + (hullRatio / 0.2f) * 0.15f
            : 1.0f;
        float wf  = wind.speedFactor(player_.heading);
        float spd = getShipTypeDef(player_.shipClass).speed
                    * SPEED_TIERS[playerSpeedLevel_] * wf * floodMult * COMBAT_SPEED_SCALE;
        float rad = player_.heading * PI / 180.0f;
        player_.x += std::sinf(rad) * spd * dt;
        player_.y -= std::cosf(rad) * spd * dt;
    }

    // Player fires (Space)
    player_.reloadTimer = std::max(0.0f, player_.reloadTimer - dt);
    if (input.justPressed(SDL_SCANCODE_SPACE) && player_.reloadTimer <= 0.0f) {
        auto  cannon = getCannonConfig(player_.cannonTier);
        float d      = distBetween(player_, enemy_);
        float bear   = bearingTo(player_, enemy_);
        if (inBroadside(player_.heading, bear) && d <= cannon.range) {
            fireVolley(true, player_.x, player_.y, enemy_.x, enemy_.y,
                       player_.cannonTier, player_.morale);
            player_.reloadTimer = cannon.reload;
        }
    }

    updateEnemyAI(dt, wind);
    updateCannonballs(dt);
    checkSurrenderTrigger();

    // Boarding (close range + low hull)
    {
        float d = distBetween(player_, enemy_);
        bool boardZone = d < 0.6f &&
            ((float)player_.hullCur / player_.hullMax < 0.3f ||
             (float)enemy_.hullCur  / enemy_.hullMax  < 0.3f);
        if (boardZone) {
            player_.boardTimer = std::max(0.0f, player_.boardTimer - dt);
            if (player_.boardTimer <= 0.0f) {
                auto round = doBoardingRound(player_.crewCur, player_.morale,
                                              enemy_.crewCur,  enemy_.morale);
                player_.crewCur = std::max(0, player_.crewCur - round.playerCrewLost);
                enemy_.crewCur  = std::max(0, enemy_.crewCur  - round.enemyCrewLost);
                player_.boardTimer = 2.0f;
            }
        }
    }

    checkEscapeAndOutcomes();
}

void CombatScreen::updateEnemyAI(float dt, const Wind& wind) {
    float d    = distBetween(enemy_, player_);
    float bear = bearingTo(enemy_, player_);
    auto  cannon = getCannonConfig(enemy_.cannonTier);

    if ((float)enemy_.hullCur / enemy_.hullMax < 0.20f) {
        // Critical hull — flee to nearest arena edge
        float awayBear = std::fmodf(bear + 180.0f, 360.0f);
        steerToBearing(enemy_, awayBear, dt);
    } else if (d > cannon.range + 1.0f) {
        steerToBearing(enemy_, bear, dt);
    } else if (d < cannon.range - 1.5f) {
        steerToBearing(enemy_, std::fmodf(bear + 180.0f, 360.0f), dt);
    } else {
        // Orbit at range: bring broadside to bear
        steerToBearing(enemy_, std::fmodf(bear + 90.0f, 360.0f), dt);
    }

    // Enemy movement — same wind physics and flooding penalty as player.
    {
        float hullRatio = (float)enemy_.hullCur / (float)enemy_.hullMax;
        float floodMult = (hullRatio < 0.2f)
            ? 0.05f + (hullRatio / 0.2f) * 0.15f
            : 1.0f;
        float wf  = wind.speedFactor(enemy_.heading);
        float spd = getShipTypeDef(enemy_.shipClass).speed * wf * floodMult * COMBAT_SPEED_SCALE;
        float rad = enemy_.heading * PI / 180.0f;
        enemy_.x += std::sinf(rad) * spd * dt;
        enemy_.y -= std::cosf(rad) * spd * dt;
        // Only a critically damaged ship (< 20% hull) can leave the arena.
        // All other maneuvers — orbiting, backing off, chasing — are clamped in.
        if (hullRatio >= 0.2f) {
            enemy_.x = std::max(0.5f, std::min(enemy_.x, (float)ARENA_W - 0.5f));
            enemy_.y = std::max(0.5f, std::min(enemy_.y, (float)ARENA_H - 0.5f));
        }
    }

    // Enemy fires
    enemy_.reloadTimer = std::max(0.0f, enemy_.reloadTimer - dt);
    if (enemy_.reloadTimer <= 0.0f && inBroadside(enemy_.heading, bear) && d <= cannon.range) {
        fireVolley(false, enemy_.x, enemy_.y, player_.x, player_.y,
                   enemy_.cannonTier, enemy_.morale);
        enemy_.reloadTimer = cannon.reload;
    }
}

void CombatScreen::checkEscapeAndOutcomes() {
    bool pEscaped = player_.x < 0.0f || player_.x > ARENA_W ||
                    player_.y < 0.0f || player_.y > ARENA_H;
    // Enemy escape is intentional — only possible when hull < 20% and crawling.
    bool eEscaped = enemy_.x  < 0.0f || enemy_.x  > ARENA_W ||
                    enemy_.y  < 0.0f || enemy_.y  > ARENA_H;
    if (pEscaped || eEscaped) { outcome_ = CombatOutcome::Escaped; return; }

    if (player_.hullCur <= 0) { outcome_ = CombatOutcome::PlayerSunk;     return; }
    if (player_.crewCur <= 0) { outcome_ = CombatOutcome::PlayerCaptured; return; }
    if (enemy_.hullCur  <= 0) {
        outcome_ = CombatOutcome::EnemySunk;
        gold_    = enemy_.gold + enemy_.hullMax * 2;
        return;
    }
    if (enemy_.crewCur  <= 0) {
        outcome_ = CombatOutcome::EnemyCaptured;
        gold_    = enemy_.gold + enemy_.hullMax * 3;
        return;
    }
}

// ---- render ----

void CombatScreen::renderBackground() {
    SDL_Rect arena{ 0, ARENA_Y, ARENA_W * TILE_PX, ARENA_H * TILE_PX };
    SDL_SetRenderDrawColor(ui_.renderer(), 8, 35, 72, 255);
    SDL_RenderFillRect(ui_.renderer(), &arena);

    // Escape zone edge strips
    int margin = 2 * TILE_PX;
    SDL_SetRenderDrawBlendMode(ui_.renderer(), SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ui_.renderer(), 30, 80, 140, 80);
    SDL_Rect strips[4] = {
        { 0,                          ARENA_Y,                              margin, ARENA_H * TILE_PX },
        { ARENA_W * TILE_PX - margin, ARENA_Y,                              margin, ARENA_H * TILE_PX },
        { 0,                          ARENA_Y,                              ARENA_W * TILE_PX, margin },
        { 0,                          ARENA_Y + ARENA_H * TILE_PX - margin, ARENA_W * TILE_PX, margin },
    };
    for (auto& s : strips) SDL_RenderFillRect(ui_.renderer(), &s);
    SDL_SetRenderDrawBlendMode(ui_.renderer(), SDL_BLENDMODE_NONE);

    SDL_Color dimBlue = { 60, 100, 150, 255 };
    ui_.drawTextCentered("OPEN SEA", { 0,                        ARENA_Y + ARENA_H * TILE_PX / 2 - 10, margin * 2, 20 }, dimBlue);
    ui_.drawTextCentered("OPEN SEA", { ARENA_W * TILE_PX - margin*2, ARENA_Y + ARENA_H * TILE_PX / 2 - 10, margin * 2, 20 }, dimBlue);

    SDL_SetRenderDrawColor(ui_.renderer(), 30, 70, 120, 255);
    SDL_RenderDrawRect(ui_.renderer(), &arena);
}

void CombatScreen::renderFighter(const Fighter& f, bool isPlayer) {
    int   sx   = asx(f.x);
    int   sy   = asy(f.y);
    float hRad = f.heading * PI / 180.0f;
    int   sz   = TILE_PX / 2 + 2;

    int fx  = sx + (int)(std::sinf(hRad) * sz);
    int fy  = sy - (int)(std::cosf(hRad) * sz);
    int blx = sx + (int)(std::sinf(hRad + 2.4f) * sz * 0.65f);
    int bly = sy - (int)(std::cosf(hRad + 2.4f) * sz * 0.65f);
    int brx = sx + (int)(std::sinf(hRad - 2.4f) * sz * 0.65f);
    int bry = sy - (int)(std::cosf(hRad - 2.4f) * sz * 0.65f);

    SDL_Color col = isPlayer ? SDL_Color{ 255, 255, 255, 255 }
                             : SDL_Color{ 230,  60,  60, 255 };
    SDL_SetRenderDrawColor(ui_.renderer(), col.r, col.g, col.b, col.a);
    for (int i = 0; i <= 20; ++i) {
        float t = i / 20.0f;
        SDL_RenderDrawLine(ui_.renderer(), fx, fy,
            blx + (int)((brx - blx) * t),
            bly + (int)((bry - bly) * t));
    }

    // Hull bar above ship
    int barW = TILE_PX * 2;
    int barX = sx - barW / 2;
    int barY = sy - sz - 10;
    SDL_Rect bg{ barX, barY, barW, 5 };
    SDL_SetRenderDrawColor(ui_.renderer(), 20, 20, 20, 255);
    SDL_RenderFillRect(ui_.renderer(), &bg);
    int   filled = (int)(barW * std::max(0, f.hullCur) / (float)std::max(1, f.hullMax));
    SDL_Rect fill{ barX, barY, filled, 5 };
    SDL_Color hc = isPlayer ? SDL_Color{ 60, 200, 60, 255 } : SDL_Color{ 200, 80, 50, 255 };
    SDL_SetRenderDrawColor(ui_.renderer(), hc.r, hc.g, hc.b, hc.a);
    SDL_RenderFillRect(ui_.renderer(), &fill);
}

void CombatScreen::renderCannonballs() {
    for (const auto& cb : cannonballs_) {
        if (!cb.active) continue;
        int sx = asx(cb.x);
        int sy = asy(cb.y);

        // Cannonball: bright yellow-white dot with a small trail hint
        SDL_Color col = cb.fromPlayer ? SDL_Color{ 255, 230, 80, 255 }
                                      : SDL_Color{ 255, 120, 60, 255 };
        SDL_SetRenderDrawColor(ui_.renderer(), col.r, col.g, col.b, col.a);
        SDL_Rect dot{ sx - 3, sy - 3, 6, 6 };
        SDL_RenderFillRect(ui_.renderer(), &dot);

        // 1-px core
        SDL_SetRenderDrawColor(ui_.renderer(), 255, 255, 255, 255);
        SDL_Rect core{ sx - 1, sy - 1, 2, 2 };
        SDL_RenderFillRect(ui_.renderer(), &core);

        // Simple trail: draw a dimmer dot one step back
        float trailX = cb.x - cb.vx * 0.04f;
        float trailY = cb.y - cb.vy * 0.04f;
        int tsx = asx(trailX), tsy = asy(trailY);
        SDL_SetRenderDrawColor(ui_.renderer(), col.r / 2, col.g / 2, col.b / 2, 180);
        SDL_Rect trail{ tsx - 2, tsy - 2, 4, 4 };
        SDL_RenderFillRect(ui_.renderer(), &trail);
    }
}

void CombatScreen::renderSurrenderUI() {
    static const SDL_Color BG     = {  8, 16, 38, 245 };
    static const SDL_Color BORDER = { 220,200, 80, 255 };  // gold border — white flag moment
    static const SDL_Color TITLE  = { 255, 230, 80, 255 };
    static const SDL_Color TEXT   = { 200, 180,140, 255 };
    static const SDL_Color DIM    = {  90, 100,100, 255 };

    // Full-screen dim
    SDL_SetRenderDrawBlendMode(ui_.renderer(), SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ui_.renderer(), 0, 0, 0, 160);
    SDL_Rect fs{ 0, 0, 1280, 720 };
    SDL_RenderFillRect(ui_.renderer(), &fs);
    SDL_SetRenderDrawBlendMode(ui_.renderer(), SDL_BLENDMODE_NONE);

    static constexpr int PX = 290, PY = 150, PW = 700, PH = 420;
    SDL_Rect panel{ PX, PY, PW, PH };
    ui_.drawPanel(panel, BG, BORDER, 3);

    // Title bar
    SDL_Rect titleR{ PX, PY, PW, 56 };
    ui_.drawPanel(titleR, { 40, 35, 5, 255 }, BORDER, 2);
    ui_.drawTextCentered("~ THEY HAVE STRUCK THEIR COLOURS ~", titleR, TITLE);

    // Flavour text
    int iy = PY + 72;
    ui_.drawTextCentered("The enemy has run up the white flag.", { PX, iy, PW, 26 }, TEXT); iy += 30;
    ui_.drawTextCentered("Your crew awaits your orders, Captain.", { PX, iy, PW, 26 }, DIM);  iy += 48;

    // Menu items
    struct Option { const char* label; const char* desc; };
    static const Option opts[3] = {
        { "Accept Surrender",   "Take their gold and cargo. Spare the crew." },
        { "Sack & Plunder",     "Strip the ship bare. Leave nothing. Gain infamy." },
        { "Refuse — Fight On",  "Lower their flag with cannon fire instead." },
    };

    for (int i = 0; i < 3; ++i) {
        SDL_Rect itemR{ PX + 30, iy, PW - 60, 52 };
        ui_.drawMenuItem(opts[i].label, itemR, i == surrenderMenuIdx_);
        SDL_Rect descR{ PX + 30, iy + 30, PW - 60, 22 };
        ui_.drawText(opts[i].desc, PX + 50, iy + 30, (i == surrenderMenuIdx_) ? TEXT : DIM);
        iy += 60;
    }

    ui_.drawText("[Up/Down] Navigate    [Enter] Confirm", PX + 30, PY + PH - 28, DIM);
}

void CombatScreen::renderHUD(const Wind& wind) {
    static const SDL_Color TXT  = { 200, 180, 140, 255 };
    static const SDL_Color DIM  = {  90, 100, 100, 255 };
    static const SDL_Color GOLD = { 255, 220,  80, 255 };
    static const SDL_Color RDY  = { 255, 220,  50, 255 };

    // Player hull — top-left
    SDL_Rect pp{ 8, 8, 230, 56 };
    ui_.drawPanel(pp, { 10, 20, 45, 220 }, { 70, 120, 155, 255 }, 2);
    ui_.drawText("Your Ship", pp.x + 8, pp.y + 6, GOLD);
    ui_.drawText("Hull:", pp.x + 8, pp.y + 30, TXT);
    SDL_Rect ph{ pp.x + 58, pp.y + 32, pp.w - 68, 14 };
    SDL_Color phc = player_.hullCur > player_.hullMax / 2
        ? SDL_Color{ 60, 200, 60, 255 } : SDL_Color{ 220, 80, 60, 255 };
    ui_.drawHealthBar(player_.hullCur, player_.hullMax, ph, phc);

    // Enemy hull — top-right
    static const char* classNames[] = { "Sloop","Brigantine","Frigate","Galleon","Man-o'-War" };
    SDL_Rect ep{ 1280 - 238, 8, 230, 56 };
    ui_.drawPanel(ep, { 40, 8, 8, 220 }, { 200, 70, 50, 255 }, 2);
    ui_.drawText(classNames[(int)enemy_.shipClass], ep.x + 8, ep.y + 6, { 255, 140, 120, 255 });
    ui_.drawText("Hull:", ep.x + 8, ep.y + 30, TXT);
    SDL_Rect eh{ ep.x + 58, ep.y + 32, ep.w - 68, 14 };
    SDL_Color ehc = enemy_.hullCur > enemy_.hullMax / 2
        ? SDL_Color{ 200, 120, 50, 255 } : SDL_Color{ 200, 50, 50, 255 };
    ui_.drawHealthBar(enemy_.hullCur, enemy_.hullMax, eh, ehc);

    // Speed — bottom-left
    static const char* spdLabels[] = { "ANCHORED","SLOW","MEDIUM","FULL SAIL" };
    static const SDL_Color spdCols[] = {
        { 120,120,120,255 }, { 80,160,220,255 }, { 100,210,120,255 }, { 255,220,80,255 }
    };
    SDL_Rect sp{ 8, 720 - 38, 220, 30 };
    ui_.drawPanel(sp, { 10, 20, 45, 200 }, { 70, 120, 155, 255 }, 1);
    ui_.drawText("Sails: ", sp.x + 8, sp.y + 6, DIM);
    ui_.drawText(spdLabels[playerSpeedLevel_], sp.x + 74, sp.y + 6, spdCols[playerSpeedLevel_]);

    // Broadside / reload — bottom-center
    float d    = distBetween(player_, enemy_);
    float bear = bearingTo(player_, enemy_);
    auto  cannon = getCannonConfig(player_.cannonTier);
    bool  arc  = inBroadside(player_.heading, bear);
    bool  inRng = d <= cannon.range;

    std::string msg; SDL_Color msgCol = DIM;
    if (player_.reloadTimer > 0.0f) {
        msg    = "RELOADING  " + std::to_string((int)std::ceil(player_.reloadTimer)) + "s";
        msgCol = { 180, 120, 50, 255 };
    } else if (arc && inRng) {
        msg    = "[SPACE]  FIRE BROADSIDE!";
        msgCol = RDY;
    } else if (!inRng) {
        msg = "Close distance to fire";
    } else {
        msg = "Maneuver to broadside";
    }
    SDL_Rect cp{ 1280/2 - 180, 720 - 38, 360, 30 };
    ui_.drawPanel(cp, { 10, 20, 45, 200 }, { 70, 120, 155, 255 }, 1);
    ui_.drawTextCentered(msg, cp, msgCol);

    // Reload progress bar
    if (player_.reloadTimer > 0.0f) {
        auto  cc   = getCannonConfig(player_.cannonTier);
        float frac = 1.0f - player_.reloadTimer / cc.reload;
        SDL_Rect rb{ 1280/2 - 180, 720 - 54, 360, 12 };
        SDL_SetRenderDrawColor(ui_.renderer(), 20, 20, 20, 200);
        SDL_RenderFillRect(ui_.renderer(), &rb);
        SDL_Rect rf{ rb.x, rb.y, (int)(rb.w * frac), rb.h };
        SDL_SetRenderDrawColor(ui_.renderer(), 255, 140, 30, 255);
        SDL_RenderFillRect(ui_.renderer(), &rf);
    }

    // Wind — top-center
    ui_.drawTextCentered("Wind: " + wind.directionName(),
                          { 1280/2 - 60, 10, 120, 22 }, DIM);

    ui_.drawText("Reach the edge to escape",        8, 720 - 62, DIM);
    ui_.drawText("[A/D] Turn    [W/S] Speed    [Space] Fire", 8, 720 - 80, { 60, 70, 80, 255 });
}

void CombatScreen::render(const Wind& wind) {
    SDL_SetRenderDrawColor(ui_.renderer(), 0, 0, 0, 255);
    SDL_RenderClear(ui_.renderer());
    renderBackground();
    renderFighter(enemy_,  false);
    renderFighter(player_, true);
    renderCannonballs();
    renderHUD(wind);
    if (surrenderPending_)
        renderSurrenderUI();
}
