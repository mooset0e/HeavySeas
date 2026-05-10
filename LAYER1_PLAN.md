# Heavy Seas — Layer 1 Implementation Plan
## Ships, Combat, Enemy AI, Captain & Game Time

> Status: APPROVED — ready to implement. Do not start until user confirms in a new session.

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

- Player starts in a **Sloop**
- Speed multiplies against `Wind::speedFactor`
- Each class has a minimum crew required to operate (enforces ship capture rules)
- Ships purchasable at Shipyard

Cannon tiers:

| Tier | Damage | Range | Reload | Accuracy |
|---|---|---|---|---|
| Light | 8 | 3 tiles | 3s | 65% |
| Medium | 14 | 5 tiles | 5s | 75% |
| Heavy | 22 | 7 tiles | 8s | 85% |

---

## Part B — Enemy Ships on the Map

Three ship categories:

| Category | Faction | Behavior | Turns hostile when |
|---|---|---|---|
| Pirate | Always hostile | Roams open sea, chases player | Always |
| Navy | Neutral | Patrols near ports | Rep < -30 OR player attacks first |
| Merchant | Neutral | Follows trade routes between ports | Player attacks first |

Counts on the map at spawn:
- Pirates: 3–5, scales up as player reputation drops
- Navy: 1–2 per port, always present
- Merchants: 3–5 roaming between ports

AI state machine per enemy:
```
Patrolling  → [player enters detection range 8 tiles]     → Chasing
Chasing     → [player within cannon range 4.5 tiles]      → Firing
Firing      → [player within boarding range 0.6 + hull<30%] → Boarding
Firing      → [hull < 25%]                                → Fleeing
Chasing     → [lost player for 12s]                       → ReturningHome
ReturningHome → [back in patrol zone]                     → Patrolling
```

All enemy ships affected by wind — player can outrun them with speed + wind angle advantage.

---

## Part C — Encounter Phase (pre-combat)

When an enemy enters detection range, `GameMode::Encounter` activates before any combat.

### Identification tiers (based on crew roles)

| Crew | Flag | Faction | Size | Class | Cannons | Hull Health | Crew Est. | Cargo Est. |
|---|---|---|---|---|---|---|---|---|
| None | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ |
| Lookout | ✓ (may be false) | ✓ | ✓ | ✗ | ✗ | ✗ | ✗ | ✗ |
| Spotter | ✓ (true) | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |

- Pirates show false nation flag to non-Spotter players
- Spotter sees through false colors, reveals Jolly Roger

### Encounter menu options
1. **Engage** — proceed to combat immediately
2. **Attempt to Evade** — evasion roll; success = escape, failure = combat forced

### Evasion formula
```
P(evade) = speedAdvantage × classModifier × healthModifier × windModifier

speedAdvantage = clamp(yourSpeed / enemySpeed, 0, 1.5)
classModifier:
  Sloop 1.3 / Brigantine 1.1 / Frigate 0.9 / Galleon 0.7 / Man-o-War 0.5
healthModifier = hullCur / hullMax
windModifier   = yourWindFactor / enemyWindFactor
```

Evasion success: enemy returns to patrol, brief detection cooldown.
Evasion failure: combat begins immediately, no second chance.

---

## Part D — Combat System

Real-time on the sailing map. No separate combat screen in Layer 1.

### Cannon fire (manual broadside)
- Player maneuvers enemy into ±60° arc off port or starboard side
- Press **Space** to fire
- Hit formula: `P(hit) = accuracy × (1 - dist/range) × (morale/100)`
- Damage: `cannon.damage ± small jitter`
- Reload timer must complete before firing again
- Enemy fires independently on their own reload timer

### Boarding (hull < 30% and within 0.6 tiles)
Every 2 seconds:
```
playerAttack = playerCrew × (morale/100) × rand(0.8–1.2)
enemyAttack  = enemyCrew  × (morale/100) × rand(0.8–1.2)
playerCrew  -= (int)(enemyAttack × 0.15)
enemyCrew   -= (int)(playerAttack × 0.15)
```
First side to 0 crew loses.

### Combat outcomes

| Outcome | Trigger | Result |
|---|---|---|
| Enemy sunk | Enemy hull = 0 | 50% cargo loot + all gold |
| Enemy captured | Enemy crew = 0 boarding | 100% cargo + gold; can swap ship if crew sufficient |
| Ship swap | Captured + enough crew | Player takes enemy ship |
| Player sunk | Player hull = 0 | Captain survival check |
| Player captured | Player crew = 0 boarding | Gold lost, ship lost, respawn nearest port |
| Escaped | Player exits detection range | Enemy returns to patrol |

### HUD additions during combat
- Enemy ship highlighted with colored indicator
- Cannon arc shown when enemy is in range
- Reload timer bar
- Morale displayed

---

## Part E — Captain & Game Time (Layer 1b — implement last)

### Captain struct
```
name
age          (starts ~25, advances with in-game days)
health       (0–100, permanently reduced on near-death)
skills[]     (navigation, combat, trade — affect outcomes)
```

Crew roles (affect encounter identification):
- **Lookout** — reveals flag, faction, rough size
- **Spotter** — reveals everything including true colors

### Game time
- Tracked in days
- Events advance the clock (sinkings, voyages, port visits)
- Age advances as days pass
- Displayed in HUD: "Year 3, Spring"

### Captain survival on sinking
```
P(survive) = (health/100) × ageModifier × skillBonus

ageModifier:
  age < 35  → 1.0
  age 35–55 → 0.8
  age 55–70 → 0.5
  age > 70  → 0.25

skillBonus: navigation skill adds up to +0.15
```

Survived: rescued to nearest port, health permanently -10 to -20, time advances several days.
Drowned: **game over**.

---

## New Files to Create

```
src/ship/ShipType.h/.cpp         — ShipClass, CannonConfig, ShipTypeDef, getShipTypeDef()
src/ship/EnemyShip.h             — Nation, Faction, AIState, PatrolZone, EnemyShip
src/ship/EnemyManager.h/.cpp     — Spawn, per-frame AI update, state machine
src/combat/CombatState.h         — CombatPhase, CombatOutcome, BoardingResult, CombatState
src/combat/CombatSystem.h/.cpp   — Cannon hit roll, boarding rounds, evasion roll, outcomes
src/encounter/EncounterScreen.h/.cpp — Pre-combat menu, identification display
src/captain/Captain.h            — Captain struct (name, age, health, skills, crew roles)
src/captain/GameTime.h/.cpp      — Day counter, season, age advancement
```

## Files to Modify

```
src/ship/ShipStats.h       — Add shipClass, cannonCount, reloadTimer, canFire, morale
src/world/Port.h           — Add nation, repairCostPerHP, hireCostPerCrew, sellsHeavyCannons
src/world/World.h/.cpp     — Assign nations to towns at generation
src/core/GameState.h       — Add Encounter + Combat to GameMode, activeCombatEnemy
src/main.cpp               — Wire EnemyManager, encounter + combat branches, captain survival
src/ui/HUD.h/.cpp          — Enemy indicators, cannon arc, reload bar, game time
src/ui/PortScreen.h/.cpp   — Real Shipyard transactions (repair, hire, buy ship)
```

## Build Order

1. `Nation` enum (add to a shared header)
2. `ShipType.h/.cpp`
3. `ShipStats.h` — expand fields
4. `Captain.h` + `GameTime.h/.cpp`
5. `Port.h` — add nation + economy fields
6. `World` — assign nations at generation
7. `GameState.h` — add Encounter + Combat modes
8. `EnemyShip.h`
9. `CombatState.h`
10. `CombatSystem.h/.cpp`
11. `EncounterScreen.h/.cpp`
12. `EnemyManager.h/.cpp`
13. `main.cpp` — wire everything
14. `HUD` update
15. `PortScreen` update — real Shipyard
