#include <SFML/Graphics.hpp>
#include <vector>
#include <random>
#include <cmath>
#include <algorithm> // NEW: For std::clamp

struct Creature {
    sf::CircleShape shape;
    sf::Vector2f velocity;
    float energy = 100.f; 
};

struct Food {
    sf::CircleShape shape;
};

int main() {
    sf::RenderWindow window(sf::VideoMode({800, 600}), "Evolution Simulator");
    window.setFramerateLimit(60);

    std::vector<Creature> population;
    std::vector<Food> foodSupply;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> posDist(50.f, 750.f); 
    std::uniform_real_distribution<float> velDist(-3.f, 3.f);   
    std::uniform_int_distribution<int> colorDist(50, 255);      
    std::uniform_real_distribution<float> sizeDist(4.f, 10.f);  
    
    // Mutation distributions
    std::uniform_real_distribution<float> mutationDist(-1.f, 1.f);
    std::uniform_int_distribution<int> colorMutation(-20, 20);

    // Spawn initial population
    for (int i = 0; i < 30; i++) {
        Creature c;
        c.shape.setRadius(sizeDist(gen));
        c.shape.setFillColor(sf::Color(colorDist(gen), colorDist(gen), colorDist(gen)));
        c.shape.setPosition({posDist(gen), posDist(gen)});
        c.velocity = {velDist(gen), velDist(gen)};
        population.push_back(c);
    }

    // Spawn initial food
    for (int i = 0; i < 150; i++) {
        Food f;
        f.shape.setRadius(3.f); 
        f.shape.setFillColor(sf::Color::Green);
        f.shape.setPosition({posDist(gen), posDist(gen)});
        foodSupply.push_back(f);
    }

    while (window.isOpen()) {
        while (const std::optional<sf::Event> event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
        }

        // NEW: Slowly regrow food like grass (10% chance every frame)
        std::uniform_int_distribution<int> chance100(1, 100);
        if (chance100(gen) <= 10 && foodSupply.size() < 200) {
            Food f;
            f.shape.setRadius(3.f); 
            f.shape.setFillColor(sf::Color::Green);
            f.shape.setPosition({posDist(gen), posDist(gen)});
            foodSupply.push_back(f);
        }

        std::vector<Creature> newBabies; // Hold babies until the loop finishes

        for (auto& c : population) {
            // Move & Bounce
            sf::Vector2f pos = c.shape.getPosition();
            pos.x += c.velocity.x;
            pos.y += c.velocity.y;
            float radius = c.shape.getRadius();
            if (pos.x <= 0 || pos.x >= 800 - (radius * 2)) c.velocity.x *= -1;
            if (pos.y <= 0 || pos.y >= 600 - (radius * 2)) c.velocity.y *= -1;
            c.shape.setPosition(pos);

            // Burn Energy (big/fast burns more)
            float speed = std::hypot(c.velocity.x, c.velocity.y);
            c.energy -= (radius * 0.015f) + (speed * 0.02f); 

            // Eat Food
            for (auto it = foodSupply.begin(); it != foodSupply.end(); ) {
                float dx = c.shape.getPosition().x - it->shape.getPosition().x;
                float dy = c.shape.getPosition().y - it->shape.getPosition().y;
                if (std::hypot(dx, dy) < c.shape.getRadius() + it->shape.getRadius()) {
                    c.energy += 40.f;          
                    it = foodSupply.erase(it); 
                } else {
                    ++it;
                }
            }

            // NEW: REPRODUCTION & MUTATION
            if (c.energy >= 200.f) {
                Creature child = c; // Clone the parent's genetics

                // Mutate Size (don't let it get smaller than 2 pixels)
                float newRadius = std::max(2.f, c.shape.getRadius() + mutationDist(gen));
                child.shape.setRadius(newRadius);

                // Mutate Speed
                child.velocity.x += mutationDist(gen);
                child.velocity.y += mutationDist(gen);

                // Mutate Color (Clamp keeps it between 0 and 255 so it doesn't crash)
                sf::Color pColor = c.shape.getFillColor();
                int newR = std::clamp((int)pColor.r + colorMutation(gen), 0, 255);
                int newG = std::clamp((int)pColor.g + colorMutation(gen), 0, 255);
                int newB = std::clamp((int)pColor.b + colorMutation(gen), 0, 255);
                child.shape.setFillColor(sf::Color(newR, newG, newB));

                // Reset energies
                c.energy = 100.f;
                child.energy = 100.f;

                // Spawn child slightly offset so they aren't stuck together
                child.shape.move({newRadius * 2, newRadius * 2});
                newBabies.push_back(child);
            }
        }

        // Add the new babies to the main population
        population.insert(population.end(), newBabies.begin(), newBabies.end());

        // The Grim Reaper
        for (auto it = population.begin(); it != population.end(); ) {
            if (it->energy <= 0) it = population.erase(it);
            else ++it;
        }

        // Draw everything
        window.clear(sf::Color(20, 20, 25));
        for (const auto& f : foodSupply) window.draw(f.shape);
        for (const auto& c : population) window.draw(c.shape);
        window.display();
    }

    return 0;
}