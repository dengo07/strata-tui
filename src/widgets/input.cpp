#include "../../include/Strata/widgets/input.hpp"
#include "../../include/Strata/style/color.hpp"
#include <algorithm>

namespace strata {

// ncurses key codes (no ncurses.h needed)
static constexpr int K_LEFT      = 260;
static constexpr int K_RIGHT     = 261;
static constexpr int K_HOME      = 262;
static constexpr int K_BACKSPACE = 263;
static constexpr int K_DELETE    = 330;
static constexpr int K_END       = 360;

Input& Input::set_placeholder(std::string placeholder) {
    placeholder_ = std::move(placeholder);
    mark_dirty();
    return *this;
}

Input& Input::set_value(std::string value) {
    value_  = std::move(value);
    cursor_ = static_cast<int>(value_.size());
    mark_dirty();
    return *this;
}

void Input::render(Canvas& canvas) {
    canvas.fill(U' ');

    int w = canvas.width();

    // Show placeholder when empty and unfocused
    if (value_.empty() && !is_focused() && !placeholder_.empty()) {
        canvas.draw_text(0, 0, placeholder_, Style{}.with_fg(color::BrightBlack));
        dirty_ = false;
        return;
    }

    // Scroll window so cursor stays visible
    int vis_w = std::max(0, w - 1);
    int start = 0;
    if (cursor_ > vis_w) start = cursor_ - vis_w;

    int val_len = static_cast<int>(value_.size());

    // Draw visible portion of value
    int vis_len = std::min(val_len - start, vis_w);
    if (vis_len > 0)
        canvas.draw_text(0, 0, value_.substr(start, vis_len),
                         Style{}.with_fg(color::BrightWhite));

    // Draw cursor (block highlight on the character at the cursor, or space)
    if (is_focused()) {
        int cx = cursor_ - start;
        if (cx >= 0 && cx < w) {
            Style cursor_s = Style{}.with_fg(color::Black).with_bg(color::BrightWhite);
            std::string ch = (cursor_ < val_len)
                ? std::string(1, value_[cursor_])
                : " ";
            canvas.draw_text(cx, 0, ch, cursor_s);
        }
    }

    dirty_ = false;
}

bool Input::handle_event(const Event& e) {
    const KeyEvent* ke = as_key(e);
    if (!ke) return false;

    int key = ke->key;
    int len = static_cast<int>(value_.size());

    // Enter / submit
    if (key == 10 || key == '\n' || key == '\r') {
        if (on_submit) on_submit(value_);
        return true;
    }

    // Left arrow
    if (key == K_LEFT) {
        if (cursor_ > 0) { --cursor_; mark_dirty(); }
        return true;
    }

    // Right arrow
    if (key == K_RIGHT) {
        if (cursor_ < len) { ++cursor_; mark_dirty(); }
        return true;
    }

    // Home
    if (key == K_HOME) {
        cursor_ = 0;
        mark_dirty();
        return true;
    }

    // End
    if (key == K_END) {
        cursor_ = len;
        mark_dirty();
        return true;
    }

    // Backspace
    if (key == K_BACKSPACE || key == 127 || key == 8) {
        if (cursor_ > 0) {
            value_.erase(cursor_ - 1, 1);
            --cursor_;
            mark_dirty();
            if (on_change) on_change(value_);
        }
        return true;
    }

    // Delete
    if (key == K_DELETE) {
        if (cursor_ < len) {
            value_.erase(cursor_, 1);
            mark_dirty();
            if (on_change) on_change(value_);
        }
        return true;
    }

    // Ctrl+U — clear line
    if (key == 21) {
        value_.clear();
        cursor_ = 0;
        mark_dirty();
        if (on_change) on_change(value_);
        return true;
    }

    // Printable ASCII
    if (key >= 32 && key < 127) {
        value_.insert(cursor_, 1, static_cast<char>(key));
        ++cursor_;
        mark_dirty();
        if (on_change) on_change(value_);
        return true;
    }

    return false;
}

} // namespace strata
