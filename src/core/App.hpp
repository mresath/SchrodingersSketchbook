#pragma once

#include <SFML/Graphics.hpp>

#include "quantum/Simulation.hpp"
#include "render/Renderer.hpp"

class App
{
public:
    App();
    int run();

private:
    void processEvents();
    void update(float frameTime);
    void draw();

    void drawStats(float fps);
    void drawControls();
    void applyBrushAtCursor();
    void resetView();
    void resetWavefunction();

    sf::RenderWindow m_window;
    sf::View m_view;
    sf::Clock m_clock;

    Simulation m_simulation;
    Renderer m_renderer;

    float m_accumulatedZoom;
    sf::Vector2f m_lastMousePos;

    bool m_isPanning;
    bool m_isPainting;
    bool m_eraseBrush;
    bool m_paused;

    float m_accumulator;
    float m_lastFrameTime;

    float m_simDt;
    int m_simSubsteps;
    float m_brushRadius;
    float m_barrierStrength;
    bool m_showPotentialOverlay;
    bool m_enableRenormalization;

    GaussianPacketConfig m_packet;

    int m_fixedFlags;
};