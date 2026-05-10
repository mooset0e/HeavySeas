#pragma once
#include <vector>
#include <utility>

struct ShippingRoute {
    int portA, portB;
    // Ocean-tile coordinates along the route, computed by A* at world generation.
    // Empty if no ocean path exists between the two ports.
    std::vector<std::pair<int,int>> path;
};
