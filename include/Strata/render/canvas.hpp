#pragma once
#include "backend.hpp"
#include "../core/rect.hpp"
#include "../style/style.hpp"
#include <string_view>

namespace strata {

class Canvas {
    Backend& backend_;
    Rect area_;

public:
    Canvas(Backend& backend, Rect area);

    // All x, y coordinates are RELATIVE to this canvas's top-left origin.
    // Draw calls that fall outside the canvas bounds are silently clipped.
    void draw_text(int x, int y, std::string_view text, const Style& style = {});
    void draw_cell(int x, int y, const Cell& cell);

    // Fill entire canvas area with a character
    void fill(char32_t ch = U' ', const Style& style = {});

    // Draw a single-line border around the canvas perimeter using Unicode box chars
    void draw_border(const Style& style = {});

    // Draw a title string into the top border (call after draw_border)
    void draw_title(std::string_view title, const Style& style = {});

    // Create a child Canvas clipped to abs_rect (absolute terminal coordinates).
    // The result is clamped to this canvas's bounds.
    Canvas sub_canvas(Rect abs_rect) const;

    Rect area()   const { return area_; }
    int  width()  const { return area_.width; }
    int  height() const { return area_.height; }
};

} // namespace strata
