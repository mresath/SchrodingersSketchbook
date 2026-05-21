#pragma once

#include <SFML/Graphics.hpp>

#include <complex>
#include <cstddef>
#include <vector>

struct GaussianPacketConfig
{
    sf::Vector2f center;
    sf::Vector2f momentum;
    float sigma;
};

class Simulation
{
public:
    Simulation(std::size_t width, std::size_t height, float domainWidth, float domainHeight);

    void resetWavefunction(const GaussianPacketConfig &config);
    void step(float dt, int substeps);

    void clearPotential();
    void paintPotential(const sf::Vector2f &worldPos, float worldRadius, float value, bool erase);
    void addVerticalBarrier(float xCenter, float thickness, float value);
    void addDoubleSlit(float xCenter, float thickness, float slitGap, float slitHeight, float value);

    void setRenormalizationEnabled(bool enabled);
    bool isRenormalizationEnabled() const;

    void setCoordScale(float scale);
    float coordScale() const;

    void setSolverIterations(int iterations);
    int solverIterations() const;

    void setHbar(float hbar);
    float hbar() const;

    void setMass(float mass);
    float mass() const;

    float computeNorm() const;
    float computeMaxDensity() const;

    std::size_t width() const;
    std::size_t height() const;
    float domainWidth() const;
    float domainHeight() const;

    const std::vector<std::complex<float>> &state() const;
    const std::vector<float> &potential() const;

private:
    std::size_t index(std::size_t x, std::size_t y) const;
    void renormalize();
    std::size_t clampToX(float worldX) const;
    std::size_t clampToY(float worldY) const;

    std::size_t m_width;
    std::size_t m_height;
    float m_domainWidth;
    float m_domainHeight;
    float m_dx;
    float m_dy;
    float m_coordScale;
    int m_solverIterations;
    float m_hbar;
    float m_mass;

    bool m_renormalize;

    std::vector<std::complex<float>> m_psi;
    std::vector<std::complex<float>> m_nextPsi;
    std::vector<std::complex<float>> m_workPsi;
    std::vector<float> m_potential;
};