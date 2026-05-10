#pragma once

enum class GameMode { Sailing, Port };

struct GameState {
    GameMode mode           = GameMode::Sailing;
    int      activePortIndex = -1;
};
