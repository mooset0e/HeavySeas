# Heavy Seas — Layer 1 Implementation Plan
## Ships, Combat, Enemy AI, Captain & Game Time

> Status: **COMPLETE** — merged to master (commit 13a098c → 25634b5, 2026-05-10)
> Design evolved during implementation — see notes below each section.

---

## Nations

Every ship and port belongs to a nation:

| Nation | Role |
|---|---|
| Spain | Southern ports, gold-heavy galleons |
| England | Northern ports, strong navy |
| France | Western ports, fast frigates |
| Netherlands | Trading hubs, merchant fleets |
| Portugal | Eastern ports, exploration routes |
| Pirates | No nation — fly false colors until Spotter range |
| Independent | Free ports, neutral |

Ports are assigned nations at world generation. Navy ships fly their port's nation flag. Merchants fly their origin port's nation. Pirates fly a false nation flag, revealed as Jolly Roger only when a Spotter is on board.

Nation enum seeds the **per-nation reputation system** coming in Layer 3.

**Implemented:** `src/core/Nation.h` — enum class with all six nations + Independent. `World::nearestPortNation()` for encounter faction weighting.

---

## Part A — Ship Types

Five classes with distinct stats:

| Class | Speed | Turn | Hull | Crew | Cargo | Cannons | Tier |
|---|---|---|---|---|---|---|---|
| Sloop | 9.0 | 120°/s | 80 | 30 | 80 | 4 | Light |
| Brigantine | 7.5 | 90°/s | 140 | 60 | 150 | 8 | Light/Med |
| Frigate | 6.0 | 60°/s | 240 | 120 | 200 | 16 | Medium |
| Galleon | 4.5 | 40°/s | 360 | 180 | 500 | 12 | Med/Heavy |
| Man-o-War | 3.5 | 25°/s | 500 | 300 | 150 | 24 | Heavy |

Cannon tiers:

| Tier | Damage | Range | Reload | Accuracy |
|---|---|---|---|---|
| Light | 8 | 3 tiles | 3s | 65% |
| Medium | 14 | 5 tiles | 5s | 75% |
| Heavy | 22 | 7 tiles | 8s | 85% |

**Implemented:** `src/ship/ShipType.h/.cpp` — `ShipTypeDef` struct + `getShipTypeDef()`. All stats wired into both player and enemy movement/combat in `CombatScreen`.

---

## Part B — Enemy Ships on the Map

**Design change:** Visible patrolling enemy ships were replaced with a **JRPG-style random encounter system**. Invisible enemies and state-machine AI on the world map were found to be disruptive to sailing gameplay. Mission-specific ships remain as visible plot ships.

### Encounter accumulator
```
encounterAccum += distanceMoved * encounterRate * repMult * bountyMult
trigger when encounterAccum >= 1.0
```

### Encounter rate by location
```
baseRate = 0.003 + routeProximity² × 0.064
```
- Open ocean: ~1 encounter per 333 tiles moved
- On a shipping route: ~1 encounter per 15 tiles moved

### Reputation multipliers
- `repMult = 1.0 + infamy/100` — up to 2× at max infamy
- `bountyMult = 1.0 + bounty/500` — bounty hunters scale in

### Cooldowns
- 25 seconds between encounters
- 15-second grace period after leaving port

**Implemented:** encounter tick in `src/main.cpp`; faction/class generation in `src/encounter/EncounterGenerator.h/.cpp`

---

## Shipping Routes

Routes are computed at world generation using **A\* pathfinding on ocean tiles** — no land crossing.

- `World::nearestOcean()` — spiral search for embark tile adjacent to each port (ports sit on land)
- `World::astarOcean()` — 8-directional A*, ocean tiles only
- Each port connects to its 2 nearest neighbours
- Precomputed `routeProxGrid_` (per-tile float 0–1) for O(1) per-frame lookup
- Routes drawn on world map as connected line segments

**Implemented:** `src/world/ShippingRoute.h`, `src/world/World.cpp` (`buildRoutes`, `buildProximityGrid`, `astarOcean`, `nearestOcean`)

---

## Part C — Encounter Screen

When an encounter triggers, `GameMode::Encounter` activates with a menu built dynamically based on target faction and player reputation.

### Identification tiers
| Crew | Flag | Faction | Size |
|---|---|---|---|
| None | ✗ | ✗ | ✗ |
| Lookout | ✓ (may be false) | ✓ | ✓ |
| Spotter | ✓ (true) | ✓ | ✓ |

### Menu options by situation
- **Merchant (identified):** Engage / Hail & Trade / Rob & Plunder / Flee
- **Navy:** Engage / Hail (pass) / Flee
- **Unknown/Pirate:** Engage / Hail / Flee

### Rob outcome
Intimidation roll: `P(success) = 0.20 + infamy/150` (capped at 0.80). Success = full cargo, no combat. Failure = combat forced.

**Implemented:** `src/ui/EncounterScreen.h/.cpp`

---

## Part D — Combat System

Real-time arena combat in a dedicated `CombatScreen` (40×22 tile arena, 32 px/tile).

### Movement
- Both ships use `getShipTypeDef(shipClass).speed × wind.speedFactor()`
- `COMBAT_SPEED_SCALE = 0.25f` — all movement at 25% of world speed for meaningful positioning
- Flooding penalty below 20% hull: speed reduced to 5–20% of normal

### Cannon fire
- Player fires with **Space** when enemy is in broadside arc (±60°)
- **Broadside quality multiplier** — `broadsideQuality()` scales hit probability 0.3–1.0 by angle; perfect 90° = 1.0, arc edges (30°/150°) = 0.3. Applies to both player and enemy.
- Hit formula: `P(hit) = accuracy × (1 - dist/range) × (morale/100) × broadsideMult × crewAccuracyMult`
- **Triple volley** — every broadside fires 3 cannonballs at −5°/0°/+5° spread; each independently rolled for hit and damage
- **Arcing projectiles** — balls follow a `sin(progress×π)` arc, peaking 48px above the water at midpoint; shadow dot grows at peak; ball swells slightly as it rises; trail rendered behind
- **Crew-based reload and accuracy**:
  - Optimal: 3 crew per cannon → 1× reload speed, 100% accuracy
  - Short-handed: up to 3× slower reload, down to 60% accuracy
  - Applied to both player and enemy (as their crew is whittled down, they slow too)

### Cannon ranges
| Tier | Range (tiles) |
|---|---|
| Light | 6 |
| Medium | 9 |
| Heavy | 13 |

### Crew casualties from cannon fire
- Each point of hull damage has a **15% independent chance** to kill a crew member
- Expected crew deaths per hit: Light ~1.2, Medium ~2.1, Heavy ~3.3
- With 3 balls per volley, a well-placed heavy broadside can kill 6–10 crew in one salvo
- **Morale** drops 4 points per crew death, floored at 10
- As morale falls, hit accuracy degrades further (compounding pressure)
- HUD panels show hull bar, crew bar, and morale % for both ships

### Escape
- Enemy is clamped to arena above 20% hull
- Below 20% hull (flooding): enemy may drift outside arena to escape
- `CombatOutcome::EnemyEscaped` recorded

### Surrender
- Triggers once when enemy hull drops below 25%
- White-flag UI freezes combat; three choices:
  - **Accept:** capture gold + cargo + crew value → `EnemyCaptured`
  - **Sack:** plunder + hull scrap bonus, sets `sacked_` flag (+15 infamy) → `EnemySunk`
  - **Refuse:** dismiss, combat resumes

### Victory screen (`GameMode::CombatResult`)
Shown after every EnemySunk or EnemyCaptured outcome before returning to sailing.
- Displays gold seized and cargo plundered
- Title changes to `~ SACKED AND PLUNDERED ~` if the ship was sacked
- **Crew recruitment offer**: each surviving enemy crew member independently rolls to defect
  - Defection chance: `0.25 + (1 - enemyMorale/100) × 0.45` (25–70%, higher when morale is broken)
  - Offer capped at available bunk space on player ship
  - `[Y]` accept / `[N]` decline → `[Enter]` return to sailing

### Outcomes
| Outcome | Trigger |
|---|---|
| EnemySunk | Enemy hull = 0 |
| EnemyCaptured | Accepted surrender |
| PlayerSunk | Player hull = 0 |
| EnemyEscaped | Enemy drifts outside arena (hull < 20%) |
| PlayerFled | Player exits arena |

**Implemented:** `src/ui/CombatScreen.h/.cpp`, `src/combat/CombatState.h`, `src/combat/CombatSystem.h/.cpp`, `src/core/GameState.h` (`CombatResult` mode), `src/main.cpp` (victory screen + recruitment logic)

---

## Part E — Captain & Game Time

Basic stubs implemented. Full skill/age/survival system deferred to Layer 2.

**Implemented:** `src/captain/Captain.h` (struct placeholder), `src/captain/GameTime.h/.cpp` (day counter, season display)

---

## Reputation

```cpp
struct Reputation { float infamy = 0.0f; int bounty = 0; };
```

- Sacking a surrendered ship: +15 infamy
- Sinking a non-pirate: +5 infamy
- Affects encounter rate, faction hostility, and Rob intimidation roll

**Implemented:** `src/ship/Reputation.h`, wired into `ShipStats` and `main.cpp`

---

## Files Delivered

| File | Status |
|---|---|
| `src/core/Nation.h` | New |
| `src/ship/ShipType.h/.cpp` | New |
| `src/ship/EnemyShip.h` | New |
| `src/ship/EnemyManager.h/.cpp` | New (plot ships only) |
| `src/ship/Reputation.h` | New |
| `src/combat/CombatState.h` | New |
| `src/combat/CombatSystem.h/.cpp` | New |
| `src/encounter/EncounterGenerator.h/.cpp` | New |
| `src/ui/CombatScreen.h/.cpp` | New |
| `src/ui/EncounterScreen.h/.cpp` | New |
| `src/world/ShippingRoute.h` | New |
| `src/captain/Captain.h` | New (stub) |
| `src/captain/GameTime.h/.cpp` | New |
| `src/ship/ShipStats.h` | Modified |
| `src/world/World.h/.cpp` | Modified |
| `src/core/GameState.h` | Modified |
| `src/main.cpp` | Modified |
| `src/ui/HUD.h/.cpp` | Modified |
| `src/ui/PortScreen.h/.cpp` | Modified |
| `src/world/Port.h` | Modified |

---

## Layer 2 — Planned Next

- Captain skills (navigation, combat, trade) with real mechanical effects
- Per-nation reputation (not just global infamy)
- Ship capture / crew transfer / ship swap
- Full PortScreen economy: Shipyard purchases, cannon upgrades, crew hiring
- Captain age advancement and survival roll on sinking
- Boarding combat (crew vs crew)
