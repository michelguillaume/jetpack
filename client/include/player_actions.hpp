#ifndef PLAYER_ACTIONS_HPP_
#define PLAYER_ACTIONS_HPP_

#include <cstdint>

/**
 * @brief Enumeration representing all possible player actions
 * for a networked Jetpack Joyride game on PC.
 */
enum class PlayerAction : uint16_t {
  ActivateJetpack = 1 << 0,
  MoveLeft        = 1 << 1,
  MoveRight       = 1 << 2,
  Shoot           = 1 << 3
};

namespace player_actions {

/**
 * @brief Bitwise OR operator for combining actions.
 */
inline uint16_t operator|(const uint16_t lhs, PlayerAction rhs) {
  return lhs | static_cast<uint16_t>(rhs);
}

/**
 * @brief Bitwise AND operator for checking specific actions.
 */
inline uint16_t operator&(const uint16_t lhs, PlayerAction rhs) {
  return lhs & static_cast<uint16_t>(rhs);
}

/**
 * @brief Bitwise OR assignment operator for combining actions.
 */
inline uint16_t& operator|=(uint16_t& lhs, PlayerAction rhs) {
  lhs |= static_cast<uint16_t>(rhs);
  return lhs;
}

/**
 * @brief Bitwise AND assignment operator for removing actions.
 */
inline uint16_t& operator&=(uint16_t& lhs, PlayerAction rhs) {
  lhs &= static_cast<uint16_t>(rhs);
  return lhs;
}

/**
 * @brief Bitwise NOT operator for negating an action.
 */
inline uint16_t operator~(PlayerAction action) {
  return ~static_cast<uint16_t>(action);
}

}  // namespace player_actions

#endif  // PLAYER_ACTIONS_HPP_
