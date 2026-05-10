#include "ui/Menu.h"
#include <algorithm>

Menu::Menu(std::vector<MenuItem> items) : items_(std::move(items)) {}

void Menu::moveUp() {
    if (!items_.empty())
        selected_ = (selected_ - 1 + (int)items_.size()) % (int)items_.size();
}

void Menu::moveDown() {
    if (!items_.empty())
        selected_ = (selected_ + 1) % (int)items_.size();
}
