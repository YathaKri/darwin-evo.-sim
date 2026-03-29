#include "UI.h"
#include <string>

UI::UI() {
    m_fontLoaded = m_font.openFromFile("C:/Windows/Fonts/consola.ttf") ||
                   m_font.openFromFile("C:/Windows/Fonts/arial.ttf")   ||
                   m_font.openFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf");

    m_panel.setSize({100.f, 620.f});
    m_panel.setPosition({800.f, 0.f});
    m_panel.setFillColor(sf::Color(15, 15, 20, 230));

    m_divider.setSize({1.f, 620.f});
    m_divider.setPosition({800.f, 0.f});
    m_divider.setFillColor(sf::Color(60, 60, 70));
}

sf::Text UI::makeLabel(const std::string& str, float x, float y, unsigned size) {
    sf::Text t(m_font, str, size);
    t.setFillColor(sf::Color(220, 220, 220));
    t.setPosition({x, y});
    return t;
}

void UI::drawStat(sf::RenderWindow& window, const std::string& label,
                  const std::string& value, float x, float& y) {
    auto lbl = makeLabel(label, x, y, 11);
    lbl.setFillColor(sf::Color(130, 130, 150));
    window.draw(lbl);
    y += 14.f;

    auto val = makeLabel(value, x, y, 15);
    val.setFillColor(sf::Color(240, 240, 240));
    window.draw(val);
    y += 48.f;
}

void UI::draw(sf::RenderWindow& window, const SimStats& stats) {
    window.draw(m_panel);
    window.draw(m_divider);

    if (!m_fontLoaded) return;

    float lx = 808.f;
    float ly = 12.f;

    auto title = makeLabel("STATS", lx, ly, 14);
    title.setFillColor(sf::Color(140, 200, 255));
    window.draw(title);
    ly += 22.f;

    sf::RectangleShape sep({84.f, 1.f});
    sep.setPosition({lx, ly});
    sep.setFillColor(sf::Color(60, 60, 80));
    window.draw(sep);
    ly += 8.f;

    drawStat(window, "POP",      std::to_string(stats.population),  lx, ly);
    drawStat(window, "PEAK",     std::to_string(stats.peakPop),     lx, ly);
    drawStat(window, "FOOD",     std::to_string(stats.foodCount),   lx, ly);
    drawStat(window, "BIRTHS",   std::to_string(stats.totalBirths), lx, ly);
    drawStat(window, "AVG SZ",   std::to_string(stats.avgSize).substr(0, 4),  lx, ly);
    drawStat(window, "AVG SPD",  std::to_string(stats.avgSpeed).substr(0, 4), lx, ly);
}
