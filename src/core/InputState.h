#pragma once
#include <SDL.h>
#include <unordered_set>

class InputState {
public:
    void beginFrame();
    void processEvent(const SDL_Event& e);
    void endFrame();

    bool held(SDL_Scancode sc) const;
    bool justPressed(SDL_Scancode sc) const;

private:
    const Uint8* keyState_ = nullptr;
    std::unordered_set<SDL_Scancode> pressed_;
};
