#include <SFML/Graphics.hpp>
#include "Simulation.h"
#include "UI.h"

int main() {
    sf::RenderWindow window(sf::VideoMode({900, 620}), "Evolution Simulator");
    window.setFramerateLimit(60);

    Simulation sim;
    UI         ui;

    while (window.isOpen()) {
        while (const std::optional<sf::Event> event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
        }

        sim.update();

        window.clear(sf::Color(20, 20, 25));
        sim.draw(window);
        ui.draw(window, sim.getStats());
        window.display();
    }

    return 0;
}
