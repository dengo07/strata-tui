#pragma once
#include "style.hpp"
#include <cstdint>

namespace strata {

struct Cell {
    char32_t ch = U' ';
    Style style;

    bool operator==(const Cell& o) const { return ch == o.ch && style == o.style; }
    bool operator!=(const Cell& o) const { return !(*this == o); }
};

} // namespace strata
