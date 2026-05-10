#pragma once
#include <SDL.h>
#include <vector>

class Wind;

struct Cloud {
    float wx, wy;   // world-pixel position
    float radius;
    float speed;
    Uint8 alpha;
};

class CloudSystem {
public:
    CloudSystem();
    void update(float dt, const Wind& wind);
    void render(SDL_Renderer* renderer, float camX, float camY);

private:
    std::vector<Cloud> clouds_;

    void fillCircle(SDL_Renderer* renderer, int cx, int cy, int r);
    void wrapCloud(Cloud& c);
};
