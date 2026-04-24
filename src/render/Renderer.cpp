#include "render/Renderer.hpp"

#include "quantum/Simulation.hpp"

#include <algorithm>
#include <cmath>
#include <complex>
#include <stdexcept>

Renderer::Renderer(std::size_t width, std::size_t height, float worldWidth, float worldHeight)
    : m_pixels(width * height * 4, 0),
      m_sprite(m_texture)
{
    if (!m_texture.resize(sf::Vector2u(static_cast<unsigned>(width), static_cast<unsigned>(height))))
    {
        throw std::runtime_error("Failed to create wavefunction texture.");
    }

    m_sprite.setTexture(m_texture, true);
    m_sprite.setPosition(sf::Vector2f(0.0f, 0.0f));
    m_sprite.setScale(sf::Vector2f(worldWidth / static_cast<float>(width), worldHeight / static_cast<float>(height)));
}

void Renderer::update(const Simulation &simulation, bool showPotentialOverlay)
{
    const std::vector<std::complex<float>> &psi = simulation.state();
    const std::vector<float> &potential = simulation.potential();

    float maxDensity = simulation.computeMaxDensity();
    if (maxDensity < 1e-8f)
    {
        maxDensity = 1e-8f;
    }

    float maxPotential = 0.0f;
    if (showPotentialOverlay)
    {
        for (float value : potential)
        {
            maxPotential = std::max(maxPotential, value);
        }
        if (maxPotential < 1e-8f)
        {
            maxPotential = 1.0f;
        }
    }

    for (std::size_t i = 0; i < psi.size(); ++i)
    {
        const float density = std::norm(psi[i]);
        const float normalized = std::clamp(std::sqrt(density / maxDensity), 0.0f, 1.0f);

        float red = 40.0f + 180.0f * normalized;
        float green = 50.0f + 200.0f * normalized;
        float blue = 90.0f + 140.0f * normalized;

        if (showPotentialOverlay)
        {
            const float pot = std::clamp(potential[i] / maxPotential, 0.0f, 1.0f);
            red = std::clamp(red + 190.0f * pot, 0.0f, 255.0f);
            green = std::clamp(green - 90.0f * pot, 0.0f, 255.0f);
            blue = std::clamp(blue - 100.0f * pot, 0.0f, 255.0f);
        }

        const std::size_t base = i * 4;
        m_pixels[base + 0] = static_cast<std::uint8_t>(red);
        m_pixels[base + 1] = static_cast<std::uint8_t>(green);
        m_pixels[base + 2] = static_cast<std::uint8_t>(blue);
        m_pixels[base + 3] = 255;
    }

    m_texture.update(m_pixels.data());
}

void Renderer::draw(sf::RenderTarget &target) const
{
    target.draw(m_sprite);
}