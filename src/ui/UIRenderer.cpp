#include "ui/UIRenderer.h"

UIRenderer::UIRenderer(SDL_Renderer* renderer, TTF_Font* font)
    : renderer_(renderer), font_(font) {}

void UIRenderer::drawPanel(SDL_Rect bounds, SDL_Color bg, SDL_Color border, int borderWidth) {
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer_, bg.r, bg.g, bg.b, bg.a);
    SDL_RenderFillRect(renderer_, &bounds);

    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(renderer_, border.r, border.g, border.b, border.a);
    for (int i = 0; i < borderWidth; ++i) {
        SDL_Rect r{ bounds.x + i, bounds.y + i, bounds.w - 2 * i, bounds.h - 2 * i };
        SDL_RenderDrawRect(renderer_, &r);
    }
}

void UIRenderer::drawText(const std::string& text, int x, int y, SDL_Color color) {
    if (text.empty() || !font_) return;
    SDL_Surface* surf = TTF_RenderText_Blended(font_, text.c_str(), color);
    if (!surf) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer_, surf);
    SDL_FreeSurface(surf);
    if (!tex) return;
    int w, h;
    SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
    SDL_Rect dst{ x, y, w, h };
    SDL_RenderCopy(renderer_, tex, nullptr, &dst);
    SDL_DestroyTexture(tex);
}

void UIRenderer::drawTextCentered(const std::string& text, SDL_Rect bounds, SDL_Color color) {
    if (text.empty() || !font_) return;
    SDL_Surface* surf = TTF_RenderText_Blended(font_, text.c_str(), color);
    if (!surf) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer_, surf);
    SDL_FreeSurface(surf);
    if (!tex) return;
    int w, h;
    SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
    SDL_Rect dst{
        bounds.x + (bounds.w - w) / 2,
        bounds.y + (bounds.h - h) / 2,
        w, h
    };
    SDL_RenderCopy(renderer_, tex, nullptr, &dst);
    SDL_DestroyTexture(tex);
}

void UIRenderer::drawMenuItem(const std::string& label, SDL_Rect bounds, bool selected) {
    if (selected) {
        SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(renderer_, 40, 60, 110, 255);
        SDL_RenderFillRect(renderer_, &bounds);
        drawText("> " + label, bounds.x + 16, bounds.y + (bounds.h - 20) / 2, { 255, 220, 80, 255 });
    } else {
        drawText("  " + label, bounds.x + 16, bounds.y + (bounds.h - 20) / 2, { 200, 180, 140, 255 });
    }
}

void UIRenderer::drawHealthBar(int current, int maximum, SDL_Rect bounds, SDL_Color fill) {
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(renderer_, 30, 30, 30, 255);
    SDL_RenderFillRect(renderer_, &bounds);
    if (maximum > 0 && current > 0) {
        int fw = (int)((float)current / maximum * bounds.w);
        SDL_Rect fr{ bounds.x, bounds.y, fw, bounds.h };
        SDL_SetRenderDrawColor(renderer_, fill.r, fill.g, fill.b, fill.a);
        SDL_RenderFillRect(renderer_, &fr);
    }
    SDL_SetRenderDrawColor(renderer_, 90, 90, 90, 255);
    SDL_RenderDrawRect(renderer_, &bounds);
}
