#pragma once
#include <SFML/Graphics.hpp>
#include "Simulation.h"

class UI {
public:
    UI();

    void draw(sf::RenderWindow& window, const SimStats& stats);

private:
    sf::Font m_font;
    bool     m_fontLoaded = false;

    sf::RectangleShape m_panel;
    sf::RectangleShape m_divider;

    sf::Text makeLabel(const std::string& str, float x, float y, unsigned size = 16);
    void     drawStat(sf::RenderWindow& window, const std::string& label,
                      const std::string& value, float x, float& y);
};
