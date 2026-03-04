#include "../../include/Strata/widgets/button.hpp"
#include "../../include/Strata/style/color.hpp"

namespace strata {

Button::Button(std::string label)
    : label_(std::move(label))
{
    style_         = Style{}.with_fg(color::White);
    focused_style_ = Style{}.with_fg(color::Black).with_bg(color::BrightWhite).with_bold();
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

void Button::render(Canvas& canvas) {
    canvas.fill(U' ');

    const Style& s = is_focused() ? focused_style_ : style_;

    // Build label: "[ Label ]"
    std::string text = "[" + label_ + "]";

    // Center vertically
    int y = canvas.height() / 2;

    // Center horizontally
    int x = (canvas.width() - static_cast<int>(text.size())) / 2;
    if (x < 0) x = 0;

    canvas.draw_text(x, y, text, s);
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
