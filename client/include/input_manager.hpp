#ifndef INPUT_MANAGER_HPP_
#define INPUT_MANAGER_HPP_

#include <cstdint>
#include <functional>
#include <unordered_map>
#include "player_actions.hpp"
#include "window_manager.hpp"

namespace jetpack {

class InputManager {
 public:
  struct PlayerInput {
    uint16_t actions;
    float dir_x;
    float dir_y;
  };

  using InputCallback = std::function<void(PlayerInput&&)>;

  /**
   * @brief Constructor for the InputManager.
   *
   * @param callback Function called when input is captured to process or send these data,
   *                 for example by serializing them into a network packet.
   * @param windowManager Reference to the WindowManager to convert mouse coordinates.
   */
  explicit InputManager(InputCallback callback, WindowManager& windowManager)
      : callback_(std::move(callback)), windowManager_(windowManager) {}

  /**
   * @brief Handles SFML events related to player inputs.
   *
   * This method processes keyboard and mouse events. For keyboard events, a bitmask is updated
   * using a mapping that associates specific keys with actions. For mouse events, the mouse
   * position is converted to world coordinates using the WindowManager.
   *
   * @param event The SFML event to process.
   */
  void HandleEvent(const sf::Event& event) {
    static const std::unordered_map<sf::Keyboard::Key, uint16_t> keyToAction{
        {sf::Keyboard::Key::Space,  static_cast<uint16_t>(PlayerAction::ActivateJetpack)},
        {sf::Keyboard::Key::Left,   static_cast<uint16_t>(PlayerAction::MoveLeft)},
        {sf::Keyboard::Key::Right,  static_cast<uint16_t>(PlayerAction::MoveRight)},
        {sf::Keyboard::Key::X,      static_cast<uint16_t>(PlayerAction::Shoot)}
    };

    bool actionsChanged = false;

    if (event.type == sf::Event::KeyPressed) {
      const sf::Keyboard::Key keyCode = event.key.code;
      auto it = keyToAction.find(keyCode);
      if (it != keyToAction.end()) {
        const uint16_t previousActions = current_actions_;
        current_actions_ |= it->second;
        actionsChanged = (current_actions_ != previousActions);
      }
    }
    else if (event.type == sf::Event::KeyReleased) {
      const sf::Keyboard::Key keyCode = event.key.code;
      auto it = keyToAction.find(keyCode);
      if (it != keyToAction.end()) {
        const uint16_t previousActions = current_actions_;
        current_actions_ &= ~it->second;
        actionsChanged = (current_actions_ != previousActions);
      }
    }
    else if (event.type == sf::Event::MouseMoved) {
      const int mouseX = event.mouseMove.x;
      const int mouseY = event.mouseMove.y;
      mouse_position_ = windowManager_.MouseToWorldCoordinates(mouseX, mouseY);
      actionsChanged = true;
    }

    if (actionsChanged) {
      PlayerInput input{ current_actions_,
                         mouse_position_.first,
                         mouse_position_.second };
      callback_(std::move(input));
    }
  }

 private:
  InputCallback callback_;
  WindowManager& windowManager_;
  uint16_t current_actions_ = 0;
  std::pair<float, float> mouse_position_ = {0.f, 0.f};
};

}  // namespace jetpack

#endif  // INPUT_MANAGER_HPP_
