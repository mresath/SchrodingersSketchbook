#include "core/App.hpp"

#include "Config.hpp"
#include "core/UI.hpp"

#include <imgui-SFML.h>
#include <imgui.h>

#include <optional>
#include <stdexcept>
#include <string>

App::App()
    : m_window(sf::VideoMode(sf::Vector2u(static_cast<unsigned>(DEF_WIDTH), static_cast<unsigned>(DEF_HEIGHT))), "Schrodinger's Sketchbook"),
      m_view(sf::FloatRect(sf::Vector2f(0.0f, 0.0f), sf::Vector2f(DEF_WIDTH, DEF_HEIGHT))),
      m_simulation(180, 120, DEF_WIDTH, DEF_HEIGHT),
      m_renderer(m_simulation.width(), m_simulation.height(), DEF_WIDTH, DEF_HEIGHT),
      m_accumulatedZoom(1.0f),
      m_lastMousePos(0.0f, 0.0f),
      m_isPanning(false),
      m_isPainting(false),
      m_eraseBrush(false),
      m_paused(false),
      m_accumulator(0.0f),
      m_lastFrameTime(0.0f),
      m_simDt(0.001f),
      m_simSubsteps(1),
      m_brushRadius(14.0f),
      m_barrierStrength(30.0f),
      m_showPotentialOverlay(true),
      m_enableRenormalization(true),
      m_packet{{DEF_WIDTH * 0.2f, DEF_HEIGHT * 0.5f}, {6.0f, 0.0f}, 22.0f},
      m_fixedFlags(ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse)
{
    if (!ImGui::SFML::Init(m_window))
    {
        throw std::runtime_error("Failed to initialize ImGui-SFML.");
    }

    m_window.setFramerateLimit(MAX_FPS);
    m_view.setCenter(sf::Vector2f(DEF_WIDTH / 2.0f, DEF_HEIGHT / 2.0f));
    m_view.setViewport(calculateLetterboxViewport(m_window.getSize(), m_view.getSize()));
    m_window.setView(m_view);

    ImGui::PushStyleVarY(ImGuiStyleVar_ItemSpacing, Y_ITEM_SPACING);

    resetWavefunction();
}

int App::run()
{
    while (m_window.isOpen())
    {
        processEvents();

        const sf::Time dtTime = m_clock.restart();
        const float rawFrameTime = dtTime.asSeconds();
        float simFrameTime = rawFrameTime;
        if (simFrameTime > MAX_DT)
        {
            simFrameTime = MAX_DT;
        }

        m_lastFrameTime = rawFrameTime;

        ImGui::SFML::Update(m_window, dtTime);

        update(simFrameTime);
        draw();
    }

    ImGui::PopStyleVar();
    ImGui::SFML::Shutdown();
    return 0;
}

void App::processEvents()
{
    while (const std::optional<sf::Event> event = m_window.pollEvent())
    {
        ImGui::SFML::ProcessEvent(m_window, *event);

        sf::View newView(m_window.getView());

        if (event->is<sf::Event::Closed>())
        {
            m_window.close();
        }
        else if (const sf::Event::Resized *resized = event->getIf<sf::Event::Resized>())
        {
            handleResize(&m_window, &newView, &m_view, resized);
        }
        else if (const auto *keyReleased = event->getIf<sf::Event::KeyReleased>())
        {
            if (keyReleased->code == sf::Keyboard::Key::P)
            {
                m_paused = !m_paused;
            }
            else if (keyReleased->code == sf::Keyboard::Key::R)
            {
                resetWavefunction();
            }
            else if (keyReleased->code == sf::Keyboard::Key::C)
            {
                m_simulation.clearPotential();
            }
        }

        if (!ImGui::GetIO().WantCaptureMouse)
        {
            if (const auto *scroll = event->getIf<sf::Event::MouseWheelScrolled>())
            {
                handleZoom(&m_window, &newView, &m_view, scroll->delta, &m_accumulatedZoom);
            }
            else if (const auto *mouseMoved = event->getIf<sf::Event::MouseMoved>())
            {
                if (m_isPanning)
                {
                    handlePanMouse(&m_window, &newView, &m_view, mouseMoved, m_lastMousePos, m_accumulatedZoom);
                }
                if (m_isPainting)
                {
                    applyBrushAtCursor();
                }
            }
            else if (const auto *mouseDown = event->getIf<sf::Event::MouseButtonPressed>())
            {
                if (mouseDown->button == sf::Mouse::Button::Middle)
                {
                    resetView();
                    newView = m_view;
                }
                else if (mouseDown->button == sf::Mouse::Button::Right)
                {
                    m_isPanning = true;
                    m_lastMousePos = sf::Vector2f(sf::Mouse::getPosition(m_window));
                }
                else if (mouseDown->button == sf::Mouse::Button::Left)
                {
                    m_isPainting = true;
                    m_eraseBrush = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift);
                    applyBrushAtCursor();
                }
            }
            else if (const auto *mouseUp = event->getIf<sf::Event::MouseButtonReleased>())
            {
                if (mouseUp->button == sf::Mouse::Button::Right)
                {
                    m_isPanning = false;
                }
                else if (mouseUp->button == sf::Mouse::Button::Left)
                {
                    m_isPainting = false;
                }
            }
        }

        m_view = newView;
        m_window.setView(m_view);
    }
}

void App::update(float frameTime)
{
    m_simulation.setRenormalizationEnabled(m_enableRenormalization);

    if (!m_paused)
    {
        m_accumulator += frameTime;
        const float fixedDt = 1.0f / CALC_FREQ;
        int updatesThisFrame = 0;

        while (m_accumulator >= fixedDt && updatesThisFrame < MAX_UPDATES_PER_FRAME)
        {
            m_simulation.step(m_simDt, m_simSubsteps);
            m_accumulator -= fixedDt;
            ++updatesThisFrame;
        }

        if (updatesThisFrame == MAX_UPDATES_PER_FRAME && m_accumulator >= fixedDt)
        {
            m_accumulator = fixedDt;
        }
    }

    m_renderer.update(m_simulation, m_showPotentialOverlay);
}

void App::draw()
{
    const float fps = m_lastFrameTime > 0.0f ? (1.0f / m_lastFrameTime) : 0.0f;

    drawStats(fps);
    drawControls();

    m_window.clear(BACKGROUND_COLOR);
    m_renderer.draw(m_window);

    ImGui::SFML::Render(m_window);
    m_window.display();
}

void App::drawStats(float fps)
{
    ImGui::SetNextWindowPos(ImVec2(m_window.getSize().x * 0.5f, 10.0f), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
    ImGui::Begin("Stats", nullptr, m_fixedFlags);
    ImGui::Text("FPS: %.1f | Calc Freq: %.0f Hz", fps, CALC_FREQ);
    ImGui::Text("Norm: %.5f", m_simulation.computeNorm());
    ImGui::Text("State: %s", m_paused ? "Paused" : "Running");
    ImGui::End();
}

void App::drawControls()
{
    ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Always);
    ImGui::Begin("Sandbox", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    if (ImGui::Button(m_paused ? "Resume" : "Pause"))
    {
        m_paused = !m_paused;
    }

    ImGui::SameLine();
    if (ImGui::Button("Step"))
    {
        m_simulation.step(m_simDt, m_simSubsteps);
    }

    if (ImGui::Button("Reset Packet"))
    {
        resetWavefunction();
    }

    ImGui::SameLine();
    if (ImGui::Button("Clear Potential"))
    {
        m_simulation.clearPotential();
    }

    if (ImGui::Button("Preset: Center Wall"))
    {
        m_simulation.addVerticalBarrier(DEF_WIDTH * 0.5f, 16.0f, m_barrierStrength);
    }

    if (ImGui::Button("Preset: Double Slit"))
    {
        m_simulation.addDoubleSlit(DEF_WIDTH * 0.5f, 14.0f, 60.0f, 32.0f, m_barrierStrength);
    }

    ImGui::Separator();

    ImGui::SliderFloat("Sim dt", &m_simDt, 0.0001f, 0.0015f, "%.4f");
    ImGui::SliderInt("Substeps", &m_simSubsteps, 1, 3);

    int solverIters = m_simulation.solverIterations();
    if (ImGui::SliderInt("Solver iters", &solverIters, 2, 24))
    {
        m_simulation.setSolverIterations(solverIters);
    }

    float coordScale = m_simulation.coordScale();
    if (ImGui::SliderFloat("Coord scale", &coordScale, 8.0f, 96.0f, "%.1f"))
    {
        m_simulation.setCoordScale(coordScale);
        resetWavefunction();
    }

    float hbar = m_simulation.hbar();
    if (ImGui::SliderFloat("Hbar", &hbar, 0.1f, 10.0f, "%.2f"))
    {
        m_simulation.setHbar(hbar);
        resetWavefunction();
    }

    ImGui::SliderFloat("Barrier", &m_barrierStrength, 1.0f, 150.0f, "%.1f");
    ImGui::SliderFloat("Brush radius", &m_brushRadius, 2.0f, 50.0f, "%.1f");
    ImGui::Checkbox("Potential overlay", &m_showPotentialOverlay);
    ImGui::Checkbox("Renormalize", &m_enableRenormalization);

    ImGui::Separator();

    bool packetChanged = false;
    packetChanged |= ImGui::SliderFloat2("Packet center", &m_packet.center.x, 0.0f, DEF_WIDTH, "%.1f");
    packetChanged |= ImGui::SliderFloat2("Packet momentum", &m_packet.momentum.x, -12.0f, 12.0f, "%.2f");
    packetChanged |= ImGui::SliderFloat("Packet sigma", &m_packet.sigma, 4.0f, 80.0f, "%.1f");
    float mass = m_simulation.mass();
    if (ImGui::SliderFloat("Packet mass", &mass, 0.1f, 10.0f, "%.2f"))
    {
        m_simulation.setMass(mass);
        packetChanged = true;
    }
    if (packetChanged)
    {
        resetWavefunction();
    }

    ImGui::Text("Paint: LMB | Erase: Shift+LMB");
    ImGui::Text("Pan: RMB | Reset view: MMB");
    ImGui::Text("Shortcuts: P pause, R reset, C clear potential");

    ImGui::End();
}

void App::applyBrushAtCursor()
{
    const sf::Vector2i pixelPos = sf::Mouse::getPosition(m_window);
    const sf::Vector2f worldPos = m_window.mapPixelToCoords(pixelPos, m_view);
    m_simulation.paintPotential(worldPos, m_brushRadius, m_barrierStrength, m_eraseBrush);
}

void App::resetView()
{
    m_view.setSize(sf::Vector2f(DEF_WIDTH, DEF_HEIGHT));
    m_view.setCenter(sf::Vector2f(DEF_WIDTH * 0.5f, DEF_HEIGHT * 0.5f));
    m_view.setViewport(calculateLetterboxViewport(m_window.getSize(), m_view.getSize()));
    m_accumulatedZoom = 1.0f;
    m_window.setView(m_view);
}

void App::resetWavefunction()
{
    m_simulation.resetWavefunction(m_packet);
}