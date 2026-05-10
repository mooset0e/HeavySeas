#include "core/InputState.h"

void InputState::beginFrame() {
    pressed_.clear();
}

void InputState::processEvent(const SDL_Event& e) {
    if (e.type == SDL_KEYDOWN && !e.key.repeat)
        pressed_.insert(e.key.keysym.scancode);
}

void InputState::endFrame() {
    keyState_ = SDL_GetKeyboardState(nullptr);
}

bool InputState::held(SDL_Scancode sc) const {
    return keyState_ && keyState_[sc];
}

bool InputState::justPressed(SDL_Scancode sc) const {
    return pressed_.count(sc) > 0;
}
