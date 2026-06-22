#ifndef PROP_NAMES_H
#define PROP_NAMES_H

#include <string_view>

// Nombres de props especiales del mapa (NPCs estáticos).
namespace PropNames {
constexpr std::string_view PRIEST = "sacerdote";
constexpr std::string_view HEALER = "sanadora";
constexpr std::string_view MERCHANT = "comerciante";
}  // namespace PropNames

#endif
