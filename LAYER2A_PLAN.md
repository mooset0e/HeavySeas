# Heavy Seas — Layer 2a Implementation Plan
## Trade Economy

> Status: APPROVED — ready to implement. Do not start until user confirms in a new session.

---

## Overview

Layer 2a introduces a full trade economy. Every port in the world is assigned a production profile and demand profile at world generation. Prices are computed from supply, demand, and distance to producers. They drift over time as players trade. The Trading Post becomes a functional buy/sell screen. The Tavern sells trade rumors — with a chance of outright lies. Per-port reputation is introduced as a data structure with a price modifier hook, ready for full enforcement in Layer 2b.

---

## Commodities

Nine tradeable goods. Each has a base price (global average in gold per unit):

| Commodity | Base Price | Notes |
|---|---|---|
| Rum | 12 | Produced from sugar, high demand everywhere |
| Sugar | 8 | Raw material, produced in warm southern ports |
| Tobacco | 14 | Luxury good, high value far from producers |
| Spices | 22 | Highest base value, rare producers |
| Timber | 6 | Heavy / bulky, cheaper near forested ports |
| Iron | 18 | Industrial, rare producers, always in demand |
| Cloth | 10 | Widely traded, moderate margins |
| Fur | 16 | Northern ports produce, southern ports pay well |
| Hemp | 9 | Rope and sail material, steady demand at shipyards |

---

## Port Economy Generation

Run once during `World::generate()`, after `placeTowns()`.

### Step 1 — Nation Affinity Weights

Each nation has weighted affinity for producing and demanding specific goods. These weights seed randomised assignment — they are probabilities, not guarantees, so the world stays surprising.

| Nation | Likely Produces | Likely Demands |
|---|---|---|
| Spain | Sugar, Tobacco, Spices | Iron, Cloth, Timber |
| England | Iron, Cloth, Timber | Rum, Spices, Fur |
| France | Cloth, Rum, Hemp | Spices, Fur, Iron |
| Netherlands | Spices, Cloth, Hemp | Timber, Fur, Sugar |
| Portugal | Spices, Sugar, Timber | Fur, Cloth, Iron |
| Independent | Rum, Fur, Hemp | Iron, Sugar, Tobacco |

### Step 2 — Assign Producers and Demanders

For each port:
1. Roll 1–2 **primary goods** (produced) from the nation's weighted list, de-duplicated across nearby ports where possible so the map has geographic spread.
2. Roll 1–2 **demanded goods** from goods it does *not* produce, weighted toward nation demand list.
3. Everything else is **neutral** at that port.

No commodity should have fewer than 2 producers across the whole world map. If a random assignment would violate this, re-roll or force-assign a spare port.

### Step 3 — Compute Base Prices Per Port

For each port `p` and each commodity `c`:

```
producers     = all ports that produce c
avgDist       = average distance from p to the nearest 3 producers of c
                (or all producers if fewer than 3 exist)
distScore     = clamp(avgDist / 40.0, 0.0, 1.0)   // 40 tiles = max meaningful distance

if p produces c:
    price[p][c] = basePrice[c] * uniform(0.40, 0.60)

else if p demands c:
    price[p][c] = basePrice[c] * (1.30 + distScore * 0.70)   // 130–200% of base

else (neutral):
    price[p][c] = basePrice[c] * uniform(0.85, 1.10)

sellPrice[p][c] = price[p][c] * 0.75   // ports always buy lower than they sell
```

Prices are stored as integers (gold per unit). Floats are rounded at generation time.

### Step 4 — Generate Trade Rumors

For each port, generate **3–5 trade rumors** pointing to other ports. Rumor selection favours pairs with large price differentials (profitable tips).

```
profitScore(src, dst, c) = sellPrice[dst][c] - buyPrice[src][c]
```

Rumors are ranked by `profitScore`. The top-scoring pairs become rumors. Each rumor records:
- Source port (where you buy this rumor)
- Target port (the tip points here)
- Commodity
- Whether the tip is "producer" (`"Port X grows the finest tobacco"`) or "demander" (`"Port X pays well for iron"`)
- A `isLie` flag — set to true with **20% probability** at generation time. Lie rumors point to a real port but invert the actual situation (claim it produces when it doesn't, or claim high prices when they're low).
- Cost to purchase: `clamp(profitScore * 0.4, 50, 200)` gold

---

## Price Drift

Prices drift over in-game time to simulate supply and demand responding to trade.

### Drift Rules (applied each time the player leaves a port after trading)

```
for each commodity c traded at port p this visit:
    if player BOUGHT c:
        price[p][c]     += units_bought * 0.15   // supply depletes → price rises
        sellPrice[p][c]  = price[p][c] * 0.75
    if player SOLD c:
        price[p][c]     -= units_sold * 0.12     // supply increases → price falls
        sellPrice[p][c]  = price[p][c] * 0.75

// Prices decay slowly back toward their generated baseline over time
// Applied each time ANY port is visited (time-based passive reversion)
for each port p, each commodity c:
    price[p][c] += (basePrice[p][c] - price[p][c]) * 0.08   // 8% reversion per port visit
```

Hard floors and ceilings:
```
price[p][c] = clamp(price[p][c], basePrice[c] * 0.25, basePrice[c] * 3.0)
```

This means: if you flood a port with rum, the price crashes but never below 25% of global base. If you drain a port of iron, it can spike to 3× but no higher.

---

## Cargo System

### ShipStats changes

```cpp
static constexpr int NUM_COMMODITIES = 9;
std::array<int, NUM_COMMODITIES> cargo = {};   // units held per commodity

int cargoUsed() const {
    int total = 0;
    for (int v : cargo) total += v;
    return total;
}
int cargoFree(ShipClass cls) const {
    return getShipTypeDef(cls).cargoMax - cargoUsed();
}
```

Cargo is lost on ship capture. Cargo is preserved on escape. Cargo is transferred partially on sinking (already handled in combat loot).

---

## Per-Port Reputation (Data Structure Only)

Stored per port, enforced fully in Layer 2b.

```cpp
struct PortReputation {
    int standing = 0;   // -100 to +100, starts neutral

    // Layer 2b will enforce these thresholds:
    // 50+      Beloved   — discounts, free rumors, exclusive quests
    // 20-49    Respected — small discounts, better rumors
    // -19–19   Neutral   — normal service
    // -20–-49  Distrusted — prices +20%, rumors refused
    // -50–-79  Hostile   — no service, guards watch
    // -80–-100 Enemy     — guards attack, barred entry
};
```

### Price modifier hook (always 1.0 in 2a, wired for 2b)

```cpp
float portPriceModifier(int standing) {
    // 2b will fill this in. For now, always neutral.
    return 1.0f;
}
```

This is called in the Trading Post price display so 2b can flip the switch without touching UI code.

---

## Trading Post UI

Activated from the Port Screen → Trading Post option.

### Layout

```
╔══════════════════════════════════════════════════════╗
║  TRADING POST — Port Royal          Cargo: 23 / 80  ║
╠══════════════════════════════════════════════════════╣
║  Commodity      Buy     Sell    Held    Qty          ║
║  ─────────────────────────────────────────────────  ║
║  Rum             14       10      0     [  0 ]       ║
║  Sugar            5        3      8     [  8 ]  ◄    ║
║  Tobacco         28       21      0     [  0 ]       ║
║  Spices          44       33      2     [  2 ]       ║
║  Timber           7        5     13     [ 13 ]       ║
║  Iron            36       27      0     [  0 ]       ║
║  Cloth           11        8      0     [  0 ]       ║
║  Fur             20       15      0     [  0 ]       ║
║  Hemp             8        6      0     [  0 ]       ║
╠══════════════════════════════════════════════════════╣
║  [Up/Down] Select   [Left/Right] Qty   [Enter] Trade ║
║  [B] Buy mode       [S] Sell mode      [Esc] Leave   ║
╚══════════════════════════════════════════════════════╝
```

### Interaction

- **Buy mode**: adjusting qty and pressing Enter purchases that many units from the port. Port must have stock. Player gold must cover cost. Cargo space must be free.
- **Sell mode**: adjusting qty and pressing Enter sells that many units from player cargo. Player must have that many held.
- Each port has limited **stock** per commodity (replenishes over time). Producers have high stock (50–100 units), neutral ports moderate (10–30), demanding ports low (0–10).
- Prices shown are per-unit. Total cost/gain shown dynamically as qty changes.

---

## Tavern — Rumors

Activated from Port Screen → Tavern → "Buy Trade Information".

### Rumor List Display

Shows 2–3 rumors available at this port (not all rumors are revealed, just their teaser):

```
╔══════════════════════════════════════════════╗
║  TAVERN — Tortuga                            ║
║  "Hear anything worth knowing, for a price." ║
╠══════════════════════════════════════════════╣
║  A sailor leans in...                        ║
║                                              ║
║  "Word is there's a port to the north-east   ║
║   sitting on a fortune in tobacco. Cheap     ║
║   as dirt there, they say."                  ║
║                                    [80 gold] ║
║                                              ║
║  "The merchants of a southern port are       ║
║   desperate for iron. Paying double."        ║
║                                   [140 gold] ║
║                                              ║
║  [Up/Down] Select    [Enter] Purchase        ║
║  [Esc] Leave                                 ║
╚══════════════════════════════════════════════╝
```

- Rumor text is generated from templates at world gen, filled with direction hints ("to the north-east") rather than exact port names until purchased.
- After purchase, the full rumor is revealed: port name, commodity, and whether they produce or demand it. The port's buy/sell prices appear on the Trading Post screen marked with a `(rumor)` tag.
- **Lie rumors**: if `isLie == true`, the revealed port and commodity are plausible but wrong. The player only discovers the lie on arrival. No in-game mechanism to detect lies before purchase — that's a Layer 2b scam confrontation feature.

---

## New Files

| File | Purpose |
|---|---|
| `src/trade/Commodity.h` | `enum class Commodity`, display names, base prices array |
| `src/trade/PortMarket.h` | `PortMarket` struct (prices, sell prices, stock, rumors, rep); `TradeRumor` struct |
| `src/trade/TradeSystem.h/.cpp` | `generateMarkets(World&, ports[], seed)` — assigns producers, computes prices, generates rumors; `applyDrift(PortMarket&, commodity, unitsBought, unitsSold)` |
| `src/ui/TradeScreen.h/.cpp` | Trading Post UI — buy/sell interface |
| `src/ui/TavernScreen.h/.cpp` | Tavern UI — rumor purchase, flavor text |

---

## Modified Files

| File | Change |
|---|---|
| `src/ship/ShipStats.h` | Add `cargo[NUM_COMMODITIES]`, `cargoUsed()`, `cargoFree()` |
| `src/world/Port.h` | Add `PortMarket market` and `PortReputation rep` fields |
| `src/world/World.h/.cpp` | Call `TradeSystem::generateMarkets()` after `placeTowns()` |
| `src/ui/PortScreen.h/.cpp` | Wire Trading Post → TradeScreen; wire Tavern → TavernScreen |
| `src/core/GameState.h` | Add `GameMode::Trading`, `GameMode::Tavern` |
| `src/main.cpp` | Handle new game modes, apply cargo drift on port exit |

---

## Build Order

1. `src/trade/Commodity.h` — enum and base prices
2. `src/trade/PortMarket.h` — structs (no logic yet)
3. `src/ship/ShipStats.h` — add cargo fields
4. `src/world/Port.h` — add market and rep fields
5. `src/trade/TradeSystem.h/.cpp` — generation + drift logic
6. `src/world/World.cpp` — call generateMarkets()
7. `src/ui/TradeScreen.h/.cpp` — buy/sell UI
8. `src/ui/TavernScreen.h/.cpp` — rumor UI
9. `src/ui/PortScreen.h/.cpp` — wire both screens
10. `src/core/GameState.h` — new modes
11. `src/main.cpp` — mode handling + drift on exit

---

## Out of Scope (Layer 2b)

- Scam confrontation mechanic (demand refund, threaten, fight)
- Per-port standing enforced: price inflation, service refusal, guard hostility
- Per-nation standing changes from combat outcomes
- Reputation UI in HUD
- Fighting-in-port consequences
