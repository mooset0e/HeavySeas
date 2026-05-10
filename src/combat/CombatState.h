#pragma once

enum class CombatPhase   { None, Active, Boarding };
enum class CombatOutcome { Ongoing, EnemySunk, EnemyCaptured, PlayerSunk, PlayerCaptured, Escaped };

struct CombatState {
    CombatPhase   phase   = CombatPhase::None;
    CombatOutcome outcome = CombatOutcome::Ongoing;
};
