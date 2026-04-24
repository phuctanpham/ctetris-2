#pragma once
#include <SFML/Graphics.hpp>

namespace layout {
    inline sf::RenderWindow create916Window(int height) {
        int width = height * 9 / 16;
        sf::ContextSettings settings;
        settings.antiAliasingLevel = 0;
        settings.depthBits = 0;
        settings.stencilBits = 0;

#ifdef __APPLE__
        return sf::RenderWindow(
            sf::VideoMode(sf::Vector2u(width, height)),
            "GameCore 9:16",
            sf::Style::Default,
            settings
        );
#else
        return sf::RenderWindow(
            sf::VideoMode(sf::Vector2u(width, height)),
            "GameCore 9:16",
            sf::Style::Close,
            settings
        );
#endif
    }
}
