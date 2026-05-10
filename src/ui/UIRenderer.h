#pragma once
#include <SDL.h>
#include <SDL_ttf.h>
#include <string>

class UIRenderer {
public:
    UIRenderer(SDL_Renderer* renderer, TTF_Font* font);

    void drawPanel(SDL_Rect bounds, SDL_Color bg, SDL_Color border, int borderWidth = 2);
    void drawText(const std::string& text, int x, int y, SDL_Color color);
    void drawTextCentered(const std::string& text, SDL_Rect bounds, SDL_Color color);
    void drawMenuItem(const std::string& label, SDL_Rect bounds, bool selected);
    void drawHealthBar(int current, int maximum, SDL_Rect bounds, SDL_Color fill);

    SDL_Renderer* renderer() const { return renderer_; }

private:
    SDL_Renderer* renderer_;
    TTF_Font*     font_;
};
