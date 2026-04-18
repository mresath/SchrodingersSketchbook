#include "Config.hpp"

#include <iostream>
#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include "core/UI.hpp"

int main()
{
    const int fixedFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;

    sf::RenderWindow window(sf::VideoMode(sf::Vector2u(DEF_WIDTH, DEF_HEIGHT)), "Schrödinger's Sketchbook");
    if (!ImGui::SFML::Init(window))
        return -1;
    window.setFramerateLimit(MAX_FPS);

    float accumulatedZoom = 1.0f;
    sf::Vector2f lastMousePos;
    bool isPanning = false;

    sf::View view(sf::FloatRect(sf::Vector2f(0, 0), sf::Vector2f(DEF_WIDTH, DEF_HEIGHT)));
    view.setCenter(sf::Vector2f(DEF_WIDTH / 2, DEF_HEIGHT / 2));
    window.setView(view);

    ImGui::PushStyleVarY(ImGuiStyleVar_ItemSpacing, Y_ITEM_SPACING);

    sf::Clock clock;
    bool paused = false;
    float accumulator = 0.0f;

    while (window.isOpen())
    {
        while (const std::optional<sf::Event> event = window.pollEvent())
        {
            sf::View newView(window.getView());

            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }
            else if (const sf::Event::Resized *resized = event->getIf<sf::Event::Resized>())
            {
                // Adjust view on window resize
                handleResize(&window, &newView, &view, resized);
            }
            else if (const auto *keyReleased = event->getIf<sf::Event::KeyReleased>())
            {
                // Toggle pause on 'P'
                if (keyReleased->code == sf::Keyboard::Key::P)
                {
                    paused = !paused;
                }
            }
            else if (!ImGui::GetIO().WantCaptureMouse)
            {
                // Make sure UI is not using the mouse before processing mouse events
                if (const auto *scroll = event->getIf<sf::Event::MouseWheelScrolled>())
                {
                    // Zoom in/out with mouse wheel
                    handleZoom(&window, &newView, &view, scroll->delta, &accumulatedZoom);
                }
                else if (const auto *mouseMoved = event->getIf<sf::Event::MouseMoved>())
                {
                    // Pan view when right mouse button is held
                    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
                    sf::Vector2f worldPos = window.mapPixelToCoords(mousePos);
                    if (isPanning)
                    {
                        handlePanMouse(&window, &newView, &view, mouseMoved, lastMousePos, accumulatedZoom);
                    }
                }
                else if (const auto *mouseDown = event->getIf<sf::Event::MouseButtonPressed>())
                {
                    if (mouseDown->button == sf::Mouse::Button::Middle)
                    {
                        // Reset view on middle mouse button
                        newView.setSize(sf::Vector2f(DEF_WIDTH, DEF_HEIGHT));
                        newView.setCenter(sf::Vector2f(0, DEF_HEIGHT / 2));
                        accumulatedZoom = 1.0f;
                    }
                    else if (mouseDown->button == sf::Mouse::Button::Right)
                    {
                        // Start panning on right mouse button
                        isPanning = true;
                        lastMousePos = sf::Vector2f(sf::Mouse::getPosition(window));
                    }
                }
                else if (const auto *mouseUp = event->getIf<sf::Event::MouseButtonReleased>())
                {
                    if (mouseUp->button == sf::Mouse::Button::Right)
                    {
                        // Stop panning on right mouse button release
                        isPanning = false;
                    }
                }
            }

            view = newView;
            window.setView(view);
        }

        sf::Time dtTime = clock.restart();
        float frameTime = dtTime.asSeconds();
        if (frameTime > MAX_DT)
            frameTime = MAX_DT;

        ImGui::SFML::Update(window, dtTime);

        float fps = frameTime > 0.0f ? 1.0f / frameTime : 0.0f;
        ImGui::SetNextWindowPos(ImVec2(window.getSize().x * 0.5f, 10), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
        ImGui::Begin("Stats", nullptr, fixedFlags);
        ImGui::Text("FPS: %.1f  |  Calc Freq: %.0f Hz", fps, CALC_FREQ);
        ImGui::End();

        if (!paused)
        {
            float fixedDt = 1.0f / CALC_FREQ;
            accumulator += frameTime;

            while (accumulator >= fixedDt)
            {
                // world.update(fixedDt);
                accumulator -= fixedDt;
            }
        }
        else
        {
            std::string pauseText = "Paused";

            ImGui::Begin("Stats", nullptr, fixedFlags);
            auto statsWidth = ImGui::GetWindowSize().x;
            auto pauseTextWidth = ImGui::CalcTextSize(pauseText.c_str()).x;
            ImGui::SetCursorPosX((statsWidth - pauseTextWidth) * 0.5f);
            ImGui::Text("%s", pauseText.c_str());
            ImGui::End();
        }

        window.clear(BACKGROUND_COLOR);

        // world.draw(&window);

        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}