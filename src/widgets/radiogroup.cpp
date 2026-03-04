#include "../../include/Strata/widgets/radiogroup.hpp"
#include "../../include/Strata/style/color.hpp"
#include <algorithm>

namespace strata {

static const std::string kEmpty;

RadioGroup& RadioGroup::add_item(std::string item) {
    items_.push_back(std::move(item));
    mark_dirty();
    return *this;
}

RadioGroup& RadioGroup::set_items(std::vector<std::string> items) {
    items_ = std::move(items);
    if (!items_.empty())
        selected_ = std::clamp(selected_, 0, static_cast<int>(items_.size()) - 1);
    else
        selected_ = 0;
    mark_dirty();
    return *this;
}

RadioGroup& RadioGroup::set_selected(int idx) {
    if (!items_.empty())
        selected_ = std::clamp(idx, 0, static_cast<int>(items_.size()) - 1);
    mark_dirty();
    return *this;
}

const std::string& RadioGroup::selected_item() const {
    if (items_.empty()) return kEmpty;
    return items_[selected_];
}

void RadioGroup::render(Canvas& canvas) {
    canvas.fill(U' ');

    int h = canvas.height();
    int n = static_cast<int>(items_.size());

    // Keep selected item in view
    if (selected_ < scroll_) scroll_ = selected_;
    if (selected_ >= scroll_ + h) scroll_ = selected_ - h + 1;
    scroll_ = std::clamp(scroll_, 0, std::max(0, n - h));

    bool focused = is_focused();

    for (int i = 0; i < h; ++i) {
        int idx = i + scroll_;
        if (idx >= n) break;

        bool sel = (idx == selected_);

        // (●) U+25CF = \xe2\x97\x8f  (○) U+25CB = \xe2\x97\x8b
        const char* bullet = sel ? "(\xe2\x97\x8f) " : "(\xe2\x97\x8b) ";

        Style bullet_s = sel
            ? Style{}.with_fg(color::BrightCyan).with_bold()
            : Style{}.with_fg(color::BrightBlack);

        Style label_s = (sel && focused)
            ? Style{}.with_fg(color::BrightWhite).with_bold()
            : sel
                ? Style{}.with_fg(color::White).with_bold()
                : Style{}.with_fg(color::White);

        canvas.draw_text(0, i, bullet, bullet_s);
        // "(●) " = 4 display chars: ( ● ) space
        canvas.draw_text(4, i, items_[idx], label_s);
    }

    dirty_ = false;
}

bool RadioGroup::handle_event(const Event& e) {
    if (items_.empty()) return false;

    int n = static_cast<int>(items_.size());

    // Navigate down
    if (is_key(e, 258) || is_char(e, 'j')) {
        if (selected_ + 1 < n) {
            ++selected_;
            mark_dirty();
        }
        return true;
    }

    // Navigate up
    if (is_key(e, 259) || is_char(e, 'k')) {
        if (selected_ > 0) {
            --selected_;
            mark_dirty();
        }
        return true;
    }

    // Select
    if (is_char(e, ' ') || is_key(e, 10) || is_key(e, '\n') || is_key(e, '\r')) {
        if (on_change) on_change(selected_, items_[selected_]);
        return true;
    }

    return false;
}

} // namespace strata
