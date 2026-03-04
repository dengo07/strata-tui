#include "../../include/Strata/widgets/select.hpp"
#include "../../include/Strata/style/color.hpp"
#include <algorithm>

namespace strata {

static const std::string kEmpty;

// ncurses key codes
static constexpr int K_LEFT  = 260;
static constexpr int K_RIGHT = 261;
static constexpr int K_UP    = 259;
static constexpr int K_DOWN  = 258;

Select& Select::set_items(std::vector<std::string> items) {
    items_ = std::move(items);
    if (!items_.empty())
        selected_ = std::clamp(selected_, 0, static_cast<int>(items_.size()) - 1);
    else
        selected_ = 0;
    mark_dirty();
    return *this;
}

Select& Select::set_selected(int idx) {
    if (!items_.empty())
        selected_ = std::clamp(idx, 0, static_cast<int>(items_.size()) - 1);
    mark_dirty();
    return *this;
}

const std::string& Select::selected_item() const {
    if (items_.empty()) return kEmpty;
    return items_[selected_];
}

void Select::render(Canvas& canvas) {
    canvas.fill(U' ');

    if (items_.empty()) { dirty_ = false; return; }

    bool focused = is_focused();
    const std::string& item = items_[selected_];

    // ◀ U+25C0 = \xe2\x97\x80   ▶ U+25B6 = \xe2\x96\xb6
    Style arrow_s = Style{}.with_fg(color::BrightCyan).with_bold();
    Style text_s  = focused
        ? Style{}.with_fg(color::BrightWhite).with_bold()
        : Style{}.with_fg(color::White);

    // Total display width: 2 (arrow+space) + item + 2 (space+arrow) = item.size() + 4
    int item_len  = static_cast<int>(item.size());
    int total_len = item_len + 4;
    int x = std::max(0, (canvas.width() - total_len) / 2);

    if (focused) {
        canvas.draw_text(x,          0, "\xe2\x97\x80 ", arrow_s);   // ◀ + space
        canvas.draw_text(x + 2,      0, item,             text_s);
        canvas.draw_text(x + 2 + item_len, 0, " \xe2\x96\xb6", arrow_s); // space + ▶
    } else {
        canvas.draw_text(x + 2, 0, item, text_s);
    }

    dirty_ = false;
}

bool Select::handle_event(const Event& e) {
    if (items_.empty()) return false;

    int n = static_cast<int>(items_.size());
    bool changed = false;

    if (is_key(e, K_LEFT)  || is_char(e, 'h') || is_char(e, 'k')) {
        selected_ = (selected_ > 0) ? selected_ - 1 : n - 1;
        changed = true;
    } else if (is_key(e, K_RIGHT) || is_char(e, 'l') || is_char(e, 'j')) {
        selected_ = (selected_ + 1 < n) ? selected_ + 1 : 0;
        changed = true;
    }

    if (changed) {
        mark_dirty();
        if (on_change) on_change(selected_, items_[selected_]);
        return true;
    }
    return false;
}

} // namespace strata
