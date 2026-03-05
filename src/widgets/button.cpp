#include "../../include/Strata/widgets/button.hpp"
#include "../../include/Strata/style/color.hpp"
#include "../../include/Strata/style/cell.hpp"
#include <variant>

namespace strata {

// Derive a darker shadow color from a button background color:
// BrightX  → X (the dim variant)
// dark/default → BrightBlack (dark gray)
static Color derive_shadow(Color bg) {
    if (auto* nc = std::get_if<NamedColor>(&bg)) {
        if (nc->is_bright())
            return NamedColor{static_cast<NamedColor::Value>(nc->value - 8)};
    }
    return color::BrightBlack;
}

Button::Button(std::string label)
    : label_(std::move(label))
{
    style_         = Style{}.with_fg(color::White).with_bg(color::Blue).with_bold();
    focused_style_ = Style{}.with_fg(color::White).with_bg(color::BrightBlue).with_bold();
}

Button& Button::set_label(std::string label) {
    label_ = std::move(label);
    mark_dirty();
    return *this;
}

Button& Button::set_style(Style style) {
    style_ = style;
    mark_dirty();
    return *this;
}

Button& Button::set_focused_style(Style style) {
    focused_style_ = style;
    mark_dirty();
    return *this;
}

Button& Button::set_shadow_style(Style style) {
    shadow_style_     = style;
    has_shadow_style_ = true;
    mark_dirty();
    return *this;
}

void Button::render(Canvas& canvas) {
    const bool focused = is_focused();
    const Style& s = focused ? focused_style_ : style_;

    int h = canvas.height();
    int w = canvas.width();

    // Reserve bottom row for shadow when there's room and not focused
    int body_h = (h >= 2 && !focused) ? h - 1 : h;

    if (body_h >= 3 && w >= 4) {
        // Tier 1: Rounded bordered style
        // Top row: ╭───╮
        canvas.draw_cell(0, 0, Cell{U'\u256D', s});
        for (int x = 1; x < w - 1; ++x)
            canvas.draw_cell(x, 0, Cell{U'\u2500', s});
        canvas.draw_cell(w - 1, 0, Cell{U'\u256E', s});

        // Middle rows: │   │
        for (int row = 1; row < body_h - 1; ++row) {
            canvas.draw_cell(0, row, Cell{U'\u2502', s});
            canvas.draw_text(1, row, std::string(w - 2, ' '), s);
            canvas.draw_cell(w - 1, row, Cell{U'\u2502', s});
        }

        // Bottom row: ╰───╯
        canvas.draw_cell(0, body_h - 1, Cell{U'\u2570', s});
        for (int x = 1; x < w - 1; ++x)
            canvas.draw_cell(x, body_h - 1, Cell{U'\u2500', s});
        canvas.draw_cell(w - 1, body_h - 1, Cell{U'\u256F', s});

        // Label centered in interior rows (1..body_h-2)
        int interior_h = body_h - 2;
        int label_row = 1 + interior_h / 2;
        std::string padded = " " + label_ + " ";
        int x = (w - static_cast<int>(padded.size())) / 2;
        if (x < 1) x = 1;
        canvas.draw_text(x, label_row, padded, s);
    } else {
        // Tier 2: Filled block
        for (int row = 0; row < body_h; ++row)
            canvas.draw_text(0, row, std::string(w, ' '), s);

        std::string padded = " " + label_ + " ";
        int y = body_h / 2;
        int x = (w - static_cast<int>(padded.size())) / 2;
        if (x < 0) x = 0;
        canvas.draw_text(x, y, padded, s);
    }

    // Half-block shadow: ▀ (top half = button color, bottom half = shadow color)
    if (h >= 2 && !focused) {
        Color shadow_bg = has_shadow_style_ ? shadow_style_.bg : derive_shadow(s.bg);
        Style half_s = Style{}.with_fg(s.bg).with_bg(shadow_bg);
        for (int x = 0; x < w; ++x)
            canvas.draw_cell(x, h - 1, Cell{U'\u2580', half_s});
    }

    dirty_ = false;
}

bool Button::handle_event(const Event& e) {
    const KeyEvent* ke = as_key(e);
    if (!ke) return false;

    if (ke->key == ' ' || ke->key == '\n' || ke->key == '\r' || ke->key == 10) {
        if (on_click) on_click();
        return true;
    }
    return false;
}

} // namespace strata
