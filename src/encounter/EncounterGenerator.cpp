#include "encounter/EncounterGenerator.h"
#include "ship/ShipType.h"
#include <random>
#include <algorithm>

EnemyShip generateEncounter(const EncounterContext& ctx, unsigned int seed) {
    std::mt19937 rng(seed);
    auto randF = [&](float lo, float hi) {
        return std::uniform_real_distribution<float>(lo, hi)(rng);
    };
    auto randI = [&](int lo, int hi) {     // inclusive
        return std::uniform_int_distribution<int>(lo, hi)(rng);
    };

    // --- Faction weights ---
    // Merchants cluster on shipping routes; navy patrols near ports;
    // pirates lurk in open water; bounty hunters appear when player has a price on their head.
    float wMerchant = 1.0f + ctx.routeProximity * 3.0f;
    float wNavy     = 1.0f + ctx.portProximity  * 2.0f;
    float wPirate   = 1.0f + (1.0f - ctx.routeProximity) * 0.8f;
    float wBounty   = (ctx.bounty > 300 || ctx.infamy > 40.0f)
                      ? 0.5f + ctx.infamy / 80.0f : 0.0f;

    // Navy is even more aggressive when player is a known enemy
    if (ctx.navyHostile) {
        wNavy   *= 1.5f;
        wBounty += 1.0f;
    }

    float total = wMerchant + wNavy + wPirate + wBounty;
    float roll  = randF(0.0f, total);

    Faction   faction;
    bool      isBountyHunter = false;

    if (roll < wMerchant) {
        faction = Faction::Merchant;
    } else if (roll < wMerchant + wNavy) {
        faction = Faction::Navy;
    } else if (roll < wMerchant + wNavy + wPirate) {
        faction = Faction::Pirate;
    } else {
        faction       = Faction::Navy;   // bounty hunters fly naval colours
        isBountyHunter = true;
    }

    // --- Ship class ---
    ShipClass sc;
    if (faction == Faction::Merchant) {
        // Merchants travel light
        sc = (randI(0, 2) == 0) ? ShipClass::Brigantine : ShipClass::Sloop;
    } else if (faction == Faction::Pirate) {
        // Pirates are opportunistic — anything up to a frigate
        int r = randI(0, 9);
        if      (r < 5) sc = ShipClass::Sloop;
        else if (r < 8) sc = ShipClass::Brigantine;
        else            sc = ShipClass::Frigate;
    } else {
        // Navy / bounty hunter — scale with player infamy
        int r = randI(0, 9);
        if (ctx.infamy < 30.0f) {
            sc = (r < 6) ? ShipClass::Sloop : ShipClass::Brigantine;
        } else if (ctx.infamy < 60.0f) {
            sc = (r < 4) ? ShipClass::Brigantine : ShipClass::Frigate;
        } else {
            sc = (r < 3) ? ShipClass::Frigate : ShipClass::Galleon;
        }
    }
    if (isBountyHunter) {
        sc = (ctx.bounty > 3000) ? ShipClass::ManOWar : ShipClass::Frigate;
    }

    // --- Cannon tier ---
    CannonTier ct;
    if (faction == Faction::Merchant)
        ct = CannonTier::Light;
    else if (sc >= ShipClass::Frigate)
        ct = (randI(0,1) == 0) ? CannonTier::Medium : CannonTier::Heavy;
    else
        ct = (faction == Faction::Navy) ? CannonTier::Medium : CannonTier::Light;

    // --- Build the ship ---
    const ShipTypeDef& def = getShipTypeDef(sc);

    EnemyShip e;
    e.faction   = faction;
    e.nation    = ctx.nearestNation;

    // Pirates may fly false colours
    if (faction == Faction::Pirate && randI(0, 2) == 0) {
        // Pick a random 'plausible' nation — just cycle through enum
        e.displayNation = static_cast<Nation>(randI(0, (int)Nation::Pirates - 1));
    } else {
        e.displayNation = e.nation;
    }

    e.shipClass   = sc;
    e.cannonTier  = ct;
    e.hullMax     = e.hullCur  = def.hullMax;
    e.crewMax     = e.crewCur  = def.crewMax;
    e.cargoMax    = def.cargoMax;
    e.cargoCur    = (faction == Faction::Merchant)
                    ? randI(def.cargoMax / 2, def.cargoMax)
                    : randI(0, def.cargoMax / 4);
    e.gold        = (faction == Faction::Merchant)
                    ? randI(100, 500)
                    : randI(20, 150);
    if (isBountyHunter) e.gold += randI(200, 600);
    e.cannonCount = def.cannonCount;
    e.morale      = randF(75.0f, 100.0f);

    e.state      = AIState::Chasing;   // already engaging the player
    e.isVisible  = false;              // encounter-only; lives outside the world map

    return e;
}
