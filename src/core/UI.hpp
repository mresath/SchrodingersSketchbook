#pragma once

#include "Config.hpp"

#include <SFML/Graphics.hpp>

#include <algorithm>

inline sf::FloatRect calculateLetterboxViewport(const sf::Vector2u &windowSize, const sf::Vector2f &viewSize)
{
    const float windowAspect = static_cast<float>(windowSize.x) / static_cast<float>(windowSize.y);
    const float viewAspect = viewSize.x / viewSize.y;

    float viewportWidth = 1.0f;
    float viewportHeight = 1.0f;
    float viewportLeft = 0.0f;
    float viewportTop = 0.0f;

    if (windowAspect > viewAspect)
    {
        viewportWidth = viewAspect / windowAspect;
        viewportLeft = (1.0f - viewportWidth) * 0.5f;
    }
    else if (windowAspect < viewAspect)
    {
        viewportHeight = windowAspect / viewAspect;
        viewportTop = (1.0f - viewportHeight) * 0.5f;
    }

    return sf::FloatRect(sf::Vector2f(viewportLeft, viewportTop), sf::Vector2f(viewportWidth, viewportHeight));
}

inline void clampViewToWorld(sf::View *view)
{
    sf::Vector2f size = view->getSize();
    size.x = std::min(size.x, DEF_WIDTH);
    size.y = std::min(size.y, DEF_HEIGHT);
    view->setSize(size);

    sf::Vector2f center = view->getCenter();
    const float halfWidth = size.x * 0.5f;
    const float halfHeight = size.y * 0.5f;

    if (size.x >= DEF_WIDTH)
    {
        center.x = DEF_WIDTH * 0.5f;
    }
    else
    {
        center.x = std::clamp(center.x, halfWidth, DEF_WIDTH - halfWidth);
    }

    if (size.y >= DEF_HEIGHT)
    {
        center.y = DEF_HEIGHT * 0.5f;
    }
    else
    {
        center.y = std::clamp(center.y, halfHeight, DEF_HEIGHT - halfHeight);
    }

    view->setCenter(center);
}

inline void handleResize(sf::Window *window, sf::View *newView, sf::View *oldView, const sf::Event::Resized *resized)
{
    (void)window;
    (void)oldView;
    (void)resized;

    clampViewToWorld(newView);
    newView->setViewport(sf::FloatRect(sf::Vector2f(0.0f, 0.0f), sf::Vector2f(1.0f, 1.0f)));
}

inline void handleZoom(sf::RenderWindow *window, sf::View *newView, sf::View *oldView, float delta, float *accumulatedZoom)
{
    auto viewSize = oldView->getSize();
    float decFactor = 1 - ZOOM_STEP;
    float incFactor = 1 + ZOOM_STEP;

    // Get mouse position in world coordinates before zoom
    sf::Vector2i mousePixelPos = sf::Mouse::getPosition(*window);
    sf::Vector2f mouseWorldPosBefore = window->mapPixelToCoords(mousePixelPos, *oldView);

    // Apply zoom
    if (delta > 0 && viewSize.x * decFactor >= DEF_WIDTH / 2.5 && viewSize.y * decFactor >= DEF_HEIGHT / 2.5)
    {
        newView->zoom(decFactor);
        *accumulatedZoom /= incFactor;
    }
    else if (delta < 0 && viewSize.x * incFactor <= DEF_WIDTH && viewSize.y * incFactor <= DEF_HEIGHT)
    {
        newView->zoom(incFactor);
        *accumulatedZoom /= decFactor;
    }

    // Get mouse position in world coordinates after zoom
    sf::Vector2f mouseWorldPosAfter = window->mapPixelToCoords(mousePixelPos, *newView);

    // Adjust view center to keep mouse position fixed in world space
    sf::Vector2f offset = mouseWorldPosBefore - mouseWorldPosAfter;
    newView->move(offset);

    clampViewToWorld(newView);
}

inline void handlePanMouse(sf::RenderWindow *window, sf::View *newView, sf::View *oldView, const sf::Event::MouseMoved *mouseMoved, sf::Vector2f &lastMousePos, float accumulatedZoom)
{
    // Move based on mouse delta
    sf::Vector2f currentMousePos = sf::Vector2f(mouseMoved->position.x, mouseMoved->position.y);
    sf::Vector2f deltaPos = lastMousePos - currentMousePos;
    deltaPos *= accumulatedZoom;

    newView->move(deltaPos);
    lastMousePos = currentMousePos;

    clampViewToWorld(newView);
}

inline void handlePanKeyboard(sf::RenderWindow *window, sf::View *newView, sf::View *oldView)
{
    sf::Vector2f movement(0.f, 0.f);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
    {
        movement.y -= PAN_SPEED;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
    {
        movement.y += PAN_SPEED;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
    {
        movement.x -= PAN_SPEED;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
    {
        movement.x += PAN_SPEED;
    }

    newView->move(movement);

    clampViewToWorld(newView);
}