#pragma once
#include <vector>
#include <string>

struct MenuItem {
    std::string label;
    std::string description;
};

class Menu {
public:
    Menu() = default;
    explicit Menu(std::vector<MenuItem> items);

    void moveUp();
    void moveDown();

    int                        selectedIndex() const { return selected_; }
    const MenuItem&            selected()      const { return items_[selected_]; }
    const std::vector<MenuItem>& items()       const { return items_; }

private:
    std::vector<MenuItem> items_;
    int selected_ = 0;
};
