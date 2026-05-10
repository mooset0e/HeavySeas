#pragma once

enum class Nation : int {
    Spain = 0, England, France, Netherlands, Portugal, Pirates, Independent, COUNT
};

inline const char* nationName(Nation n) {
    switch (n) {
        case Nation::Spain:        return "Spain";
        case Nation::England:      return "England";
        case Nation::France:       return "France";
        case Nation::Netherlands:  return "Netherlands";
        case Nation::Portugal:     return "Portugal";
        case Nation::Pirates:      return "Pirates";
        case Nation::Independent:  return "Independent";
        default:                   return "Unknown";
    }
}
