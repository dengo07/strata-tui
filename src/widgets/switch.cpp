#include "../../include/Strata/widgets/switch.hpp"
#include "../../include/Strata/style/color.hpp"

namespace strata {

Switch::Switch(std::string label, bool on)
    : label_(std::move(label)), on_(on) {}

Switch& Switch::set_label(std::string label) {
    label_ = std::move(label);
    mark_dirty();
    return *this;
}

Switch& Switch::set_on(bool on) {
    on_ = on;
    mark_dirty();
    return *this;
}

void Switch::render(Canvas& canvas) {
    canvas.fill(U' ');

    bool focused = is_focused();

    // Indicator: "[ ON ]" or "[OFF ]" — always 6 chars wide
    const char* ind_text = on_ ? "[ ON ]" : "[OFF ]";
    constexpr int ind_len = 6;

    Style label_s = focused
        ? Style{}.with_fg(color::BrightWhite).with_bold()
        : Style{}.with_fg(color::White);

    Style ind_s = on_
        ? Style{}.with_fg(color::BrightGreen).with_bold()
        : Style{}.with_fg(color::BrightBlack);

    Style dash_s = Style{}.with_fg(color::BrightBlack);

    int w    = canvas.width();
    int llen = static_cast<int>(label_.size());
    int ind_x = w - ind_len;
    if (ind_x < 0) ind_x = 0;

    canvas.draw_text(0, 0, label_, label_s);

    // Fill gap with " ─── "   (U+2500 = \xe2\x94\x80)
    int gap_start = llen;
    int gap_end   = ind_x;
    if (gap_end > gap_start + 2) {
        std::string gap = " ";
        for (int i = gap_start + 1; i < gap_end - 1; ++i)
            gap += "\xe2\x94\x80";
        gap += " ";
        canvas.draw_text(gap_start, 0, gap, dash_s);
    }

    canvas.draw_text(ind_x, 0, ind_text, ind_s);

    dirty_ = false;
}

bool Switch::handle_event(const Event& e) {
    const KeyEvent* ke = as_key(e);
    if (!ke) return false;

    if (ke->key == ' ' || ke->key == '\n' || ke->key == '\r' || ke->key == 10) {
        on_ = !on_;
        mark_dirty();
        if (on_change) on_change(on_);
        return true;
    }
    return false;
}

} // namespace strata
