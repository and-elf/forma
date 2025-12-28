#pragma once

#include <cstddef>
namespace forma::limits {
    inline constexpr std::size_t max_properties = 16;
    inline constexpr std::size_t max_children   = 16;
    inline constexpr std::size_t max_events     = 16;
    inline constexpr std::size_t max_methods    = 16;
    inline constexpr std::size_t max_free_whens = 8;
}
