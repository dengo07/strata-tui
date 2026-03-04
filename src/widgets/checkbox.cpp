#include "../../include/Strata/widgets/checkbox.hpp"
#include "../../include/Strata/style/color.hpp"

namespace strata {

Checkbox::Checkbox(std::string label, bool checked)
    : label_(std::move(label)), checked_(checked) {}

Checkbox& Checkbox::set_label(std::string label) {
    label_ = std::move(label);
    mark_dirty();
    return *this;
}

Checkbox& Checkbox::set_checked(bool checked) {
    checked_ = checked;
    mark_dirty();
    return *this;
}

void Checkbox::render(Canvas& canvas) {
    canvas.fill(U' ');

    Style check_s = checked_
        ? Style{}.with_fg(color::BrightGreen).with_bold()
        : Style{}.with_fg(color::BrightBlack);

    Style label_s = is_focused()
        ? Style{}.with_fg(color::BrightWhite).with_bold()
        : Style{}.with_fg(color::White);

    canvas.draw_text(0, 0, checked_ ? "[x]" : "[ ]", check_s);
    canvas.draw_text(4, 0, label_, label_s);

    dirty_ = false;
}

bool Checkbox::handle_event(const Event& e) {
    const KeyEvent* ke = as_key(e);
    if (!ke) return false;

    if (ke->key == ' ' || ke->key == '\n' || ke->key == '\r' || ke->key == 10) {
        checked_ = !checked_;
        mark_dirty();
        if (on_change) on_change(checked_);
        return true;
    }
    return false;
}

} // namespace strata
