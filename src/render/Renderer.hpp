#pragma once

#include <SFML/Graphics.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

class Simulation;

class Renderer
{
public:
    Renderer(std::size_t width, std::size_t height, float worldWidth, float worldHeight);

    void update(const Simulation &simulation, bool showPotentialOverlay);
    void draw(sf::RenderTarget &target) const;

private:
    std::vector<std::uint8_t> m_pixels;
    sf::Texture m_texture;
    sf::Sprite m_sprite;
};