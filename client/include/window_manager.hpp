#ifndef WINDOW_MANAGER_HPP_
#define WINDOW_MANAGER_HPP_

#include <SFML/Graphics.hpp>
#include <utility>

namespace jetpack {

class WindowManager {
public:
  /**
   * @brief Constructs the window manager by creating a new SFML window.
   */
  WindowManager()
      : window(sf::VideoMode({1200u, 1000u}), "Game Client", sf::Style::Titlebar | sf::Style::Close)
  {
    window.setVerticalSyncEnabled(true);
  }

  /**
   * @brief Provides access to the underlying SFML render window.
   *
   * @return Reference to the SFML RenderWindow.
   */
  sf::RenderWindow& getWindow() {
    return window;
  }

  /**
   * @brief Converts screen (pixel) coordinates to world coordinates using the active view.
   *
   * This function delegates to SFML's mapPixelToCoords.
   *
   * @param mouseX Mouse X position in pixels.
   * @param mouseY Mouse Y position in pixels.
   * @return A pair (world_x, world_y) representing the converted coordinates.
   */
  [[nodiscard]] std::pair<float, float> MouseToWorldCoordinates(const int mouseX, const int mouseY) const {
    sf::Vector2f worldCoords = window.mapPixelToCoords({ mouseX, mouseY });
    return { worldCoords.x, worldCoords.y };
  }

  /**
   * @brief An alias for MouseToWorldCoordinates to match naming in InputManager.
   *
   * @param mouseX Mouse X position in pixels.
   * @param mouseY Mouse Y position in pixels.
   * @return A pair (world_x, world_y) representing the converted coordinates.
   */
  [[nodiscard]] std::pair<float, float> ConvertMouseCoordinates(const int mouseX, const int mouseY) const {
    return MouseToWorldCoordinates(mouseX, mouseY);
  }

private:
  sf::RenderWindow window;
};

} // namespace jetpack

#endif // WINDOW_MANAGER_HPP_
