#pragma once
#include <algorithm>

namespace strata {

struct Rect {
    int x = 0, y = 0, width = 0, height = 0;

    constexpr int right()  const { return x + width; }
    constexpr int bottom() const { return y + height; }

    constexpr bool contains(int px, int py) const {
        return px >= x && px < right() && py >= y && py < bottom();
    }

    constexpr Rect inner(int padding) const {
        return inner(padding, padding, padding, padding);
    }

    constexpr Rect inner(int top, int right_pad, int bottom_pad, int left) const {
        int new_x = x + left;
        int new_y = y + top;
        int new_w = std::max(0, width  - left - right_pad);
        int new_h = std::max(0, height - top  - bottom_pad);
        return {new_x, new_y, new_w, new_h};
    }

    constexpr bool empty() const { return width <= 0 || height <= 0; }

    constexpr bool operator==(const Rect& o) const {
        return x == o.x && y == o.y && width == o.width && height == o.height;
    }
    constexpr bool operator!=(const Rect& o) const { return !(*this == o); }
};

} // namespace strata
