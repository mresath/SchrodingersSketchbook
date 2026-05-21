#include "quantum/Simulation.hpp"

#include <algorithm>
#include <cmath>
#include <complex>
#include <stdexcept>

namespace
{
    constexpr float MIN_SIGMA = 1.0f;
}

Simulation::Simulation(std::size_t width, std::size_t height, float domainWidth, float domainHeight)
    : m_width(width),
      m_height(height),
      m_domainWidth(domainWidth),
      m_domainHeight(domainHeight),
      m_dx(domainWidth / static_cast<float>(width - 1)),
      m_dy(domainHeight / static_cast<float>(height - 1)),
      m_coordScale(48.0f),
      m_solverIterations(8),
      m_hbar(1.0f),
      m_mass(1.0f),
      m_renormalize(true),
      m_psi(width * height, std::complex<float>(0.0f, 0.0f)),
      m_nextPsi(width * height, std::complex<float>(0.0f, 0.0f)),
      m_workPsi(width * height, std::complex<float>(0.0f, 0.0f)),
      m_potential(width * height, 0.0f)
{
    if (width < 3 || height < 3)
    {
        throw std::invalid_argument("Simulation grid dimensions must be >= 3.");
    }
    if (domainWidth <= 0.0f || domainHeight <= 0.0f)
    {
        throw std::invalid_argument("Simulation domain size must be positive.");
    }
}

void Simulation::resetWavefunction(const GaussianPacketConfig &config)
{
    const float sigma = std::max(config.sigma / m_coordScale, MIN_SIGMA / m_coordScale);
    const sf::Vector2f center(config.center.x / m_coordScale, config.center.y / m_coordScale);
    const float invTwoSigmaSq = 1.0f / (2.0f * sigma * sigma);

    for (std::size_t y = 0; y < m_height; ++y)
    {
        const float worldY = static_cast<float>(y) * (m_dy / m_coordScale);
        for (std::size_t x = 0; x < m_width; ++x)
        {
            const float worldX = static_cast<float>(x) * (m_dx / m_coordScale);
            const float dx = worldX - center.x;
            const float dy = worldY - center.y;
            const float radiusSq = dx * dx + dy * dy;
            const float envelope = std::exp(-radiusSq * invTwoSigmaSq);
            const float phase = config.momentum.x * dx + config.momentum.y * dy;
            m_psi[index(x, y)] = std::polar(envelope, phase);
        }
    }

    renormalize();
}

void Simulation::step(float dt, int substeps)
{
    if (dt <= 0.0f || substeps <= 0)
    {
        return;
    }

    const float dxScaled = m_dx / m_coordScale;
    const float dyScaled = m_dy / m_coordScale;
    const float dx2 = dxScaled * dxScaled;
    const float dy2 = dyScaled * dyScaled;
    const float invDx2 = 1.0f / dx2;
    const float invDy2 = 1.0f / dy2;
    const std::complex<float> imag(0.0f, 1.0f);
    const float dtPerSubstep = dt / static_cast<float>(substeps);

    for (int stepIndex = 0; stepIndex < substeps; ++stepIndex)
    {
        const float halfPotentialPhase = -0.5f * dtPerSubstep / m_hbar;
        for (std::size_t i = 0; i < m_psi.size(); ++i)
        {
            if (m_potential[i] != 0.0f)
            {
                m_psi[i] *= std::polar(1.0f, halfPotentialPhase * m_potential[i]);
            }
        }

        const std::complex<float> alpha = imag * (0.25f * dtPerSubstep * m_hbar / (2.0f * m_mass));
        const std::complex<float> diag = std::complex<float>(1.0f, 0.0f) + alpha * (2.0f * (invDx2 + invDy2));

        for (std::size_t y = 1; y + 1 < m_height; ++y)
        {
            for (std::size_t x = 1; x + 1 < m_width; ++x)
            {
                const std::size_t i = index(x, y);
                const std::complex<float> center = m_psi[i];
                const std::complex<float> laplacian =
                    (m_psi[index(x + 1, y)] - 2.0f * center + m_psi[index(x - 1, y)]) * invDx2 +
                    (m_psi[index(x, y + 1)] - 2.0f * center + m_psi[index(x, y - 1)]) * invDy2;

                m_workPsi[i] = center + alpha * laplacian;
                m_nextPsi[i] = center;
            }
        }

        for (std::size_t x = 0; x < m_width; ++x)
        {
            m_workPsi[index(x, 0)] = std::complex<float>(0.0f, 0.0f);
            m_workPsi[index(x, m_height - 1)] = std::complex<float>(0.0f, 0.0f);
            m_nextPsi[index(x, 0)] = std::complex<float>(0.0f, 0.0f);
            m_nextPsi[index(x, m_height - 1)] = std::complex<float>(0.0f, 0.0f);
        }
        for (std::size_t y = 0; y < m_height; ++y)
        {
            m_workPsi[index(0, y)] = std::complex<float>(0.0f, 0.0f);
            m_workPsi[index(m_width - 1, y)] = std::complex<float>(0.0f, 0.0f);
            m_nextPsi[index(0, y)] = std::complex<float>(0.0f, 0.0f);
            m_nextPsi[index(m_width - 1, y)] = std::complex<float>(0.0f, 0.0f);
        }

        for (int iter = 0; iter < m_solverIterations; ++iter)
        {
            for (std::size_t y = 1; y + 1 < m_height; ++y)
            {
                for (std::size_t x = 1; x + 1 < m_width; ++x)
                {
                    const std::size_t i = index(x, y);
                    const std::complex<float> neighbors =
                        alpha * invDx2 * (m_nextPsi[index(x + 1, y)] + m_nextPsi[index(x - 1, y)]) +
                        alpha * invDy2 * (m_nextPsi[index(x, y + 1)] + m_nextPsi[index(x, y - 1)]);

                    m_nextPsi[i] = (m_workPsi[i] + neighbors) / diag;
                }
            }
        }

        m_psi.swap(m_nextPsi);

        for (std::size_t i = 0; i < m_psi.size(); ++i)
        {
            if (m_potential[i] != 0.0f)
            {
                m_psi[i] *= std::polar(1.0f, halfPotentialPhase * m_potential[i]);
            }
        }
    }

    if (m_renormalize)
    {
        renormalize();
    }
}

void Simulation::clearPotential()
{
    std::fill(m_potential.begin(), m_potential.end(), 0.0f);
}

void Simulation::paintPotential(const sf::Vector2f &worldPos, float worldRadius, float value, bool erase)
{
    const std::size_t cx = clampToX(worldPos.x);
    const std::size_t cy = clampToY(worldPos.y);

    const float clampedRadius = std::max(worldRadius, 1.0f);
    const std::size_t rx = static_cast<std::size_t>(std::max(1.0f, clampedRadius / m_dx));
    const std::size_t ry = static_cast<std::size_t>(std::max(1.0f, clampedRadius / m_dy));

    const std::size_t xMin = (cx > rx) ? cx - rx : 0;
    const std::size_t xMax = std::min(m_width - 1, cx + rx);
    const std::size_t yMin = (cy > ry) ? cy - ry : 0;
    const std::size_t yMax = std::min(m_height - 1, cy + ry);

    for (std::size_t y = yMin; y <= yMax; ++y)
    {
        const float wy = static_cast<float>(y) * m_dy;
        const float dy = wy - worldPos.y;

        for (std::size_t x = xMin; x <= xMax; ++x)
        {
            const float wx = static_cast<float>(x) * m_dx;
            const float dx = wx - worldPos.x;
            if ((dx * dx + dy * dy) <= clampedRadius * clampedRadius)
            {
                m_potential[index(x, y)] = erase ? 0.0f : value;
            }
        }
    }
}

void Simulation::addVerticalBarrier(float xCenter, float thickness, float value)
{
    const float halfThickness = std::max(1.0f, thickness) * 0.5f;
    const float xStart = std::max(0.0f, xCenter - halfThickness);
    const float xEnd = std::min(m_domainWidth, xCenter + halfThickness);

    const std::size_t ixStart = clampToX(xStart);
    const std::size_t ixEnd = clampToX(xEnd);

    for (std::size_t y = 0; y < m_height; ++y)
    {
        for (std::size_t x = ixStart; x <= ixEnd; ++x)
        {
            m_potential[index(x, y)] = value;
        }
    }
}

void Simulation::addDoubleSlit(float xCenter, float thickness, float slitGap, float slitHeight, float value)
{
    addVerticalBarrier(xCenter, thickness, value);

    const float centerY = 0.5f * m_domainHeight;
    const float halfGap = 0.5f * std::max(10.0f, slitGap);
    const float halfHeight = 0.5f * std::max(10.0f, slitHeight);

    const float topSlitCenter = centerY - halfGap;
    const float bottomSlitCenter = centerY + halfGap;

    const float halfThickness = std::max(1.0f, thickness) * 0.5f;
    const float xStart = std::max(0.0f, xCenter - halfThickness);
    const float xEnd = std::min(m_domainWidth, xCenter + halfThickness);

    const std::size_t ixStart = clampToX(xStart);
    const std::size_t ixEnd = clampToX(xEnd);

    const auto carveSlit = [&](float slitCenterY)
    {
        const float yStart = std::max(0.0f, slitCenterY - halfHeight);
        const float yEnd = std::min(m_domainHeight, slitCenterY + halfHeight);
        const std::size_t iyStart = clampToY(yStart);
        const std::size_t iyEnd = clampToY(yEnd);

        for (std::size_t y = iyStart; y <= iyEnd; ++y)
        {
            for (std::size_t x = ixStart; x <= ixEnd; ++x)
            {
                m_potential[index(x, y)] = 0.0f;
            }
        }
    };

    carveSlit(topSlitCenter);
    carveSlit(bottomSlitCenter);
}

void Simulation::setRenormalizationEnabled(bool enabled)
{
    m_renormalize = enabled;
}

bool Simulation::isRenormalizationEnabled() const
{
    return m_renormalize;
}

void Simulation::setCoordScale(float scale)
{
    m_coordScale = std::clamp(scale, 8.0f, 128.0f);
}

float Simulation::coordScale() const
{
    return m_coordScale;
}

void Simulation::setSolverIterations(int iterations)
{
    m_solverIterations = std::clamp(iterations, 2, 40);
}

int Simulation::solverIterations() const
{
    return m_solverIterations;
}

void Simulation::setHbar(float hbar)
{
    m_hbar = std::max(0.001f, hbar);
}

float Simulation::hbar() const
{
    return m_hbar;
}

void Simulation::setMass(float mass)
{
    m_mass = std::max(0.001f, mass);
}

float Simulation::mass() const
{
    return m_mass;
}

float Simulation::computeNorm() const
{
    float sum = 0.0f;
    for (const std::complex<float> &value : m_psi)
    {
        sum += std::norm(value);
    }
    return sum * (m_dx / m_coordScale) * (m_dy / m_coordScale);
}

float Simulation::computeMaxDensity() const
{
    float maxDensity = 0.0f;
    for (const std::complex<float> &value : m_psi)
    {
        maxDensity = std::max(maxDensity, std::norm(value));
    }
    return maxDensity;
}

std::size_t Simulation::width() const
{
    return m_width;
}

std::size_t Simulation::height() const
{
    return m_height;
}

float Simulation::domainWidth() const
{
    return m_domainWidth;
}

float Simulation::domainHeight() const
{
    return m_domainHeight;
}

const std::vector<std::complex<float>> &Simulation::state() const
{
    return m_psi;
}

const std::vector<float> &Simulation::potential() const
{
    return m_potential;
}

std::size_t Simulation::index(std::size_t x, std::size_t y) const
{
    return y * m_width + x;
}

void Simulation::renormalize()
{
    const float norm = computeNorm();
    if (norm <= 1e-8f)
    {
        return;
    }

    const float invSqrtNorm = 1.0f / std::sqrt(norm);
    for (std::complex<float> &value : m_psi)
    {
        value *= invSqrtNorm;
    }
}

std::size_t Simulation::clampToX(float worldX) const
{
    const float clamped = std::clamp(worldX, 0.0f, m_domainWidth);
    const std::size_t x = static_cast<std::size_t>(clamped / m_dx);
    return std::min(m_width - 1, x);
}

std::size_t Simulation::clampToY(float worldY) const
{
    const float clamped = std::clamp(worldY, 0.0f, m_domainHeight);
    const std::size_t y = static_cast<std::size_t>(clamped / m_dy);
    return std::min(m_height - 1, y);
}