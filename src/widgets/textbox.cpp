#include "../../include/Strata/widgets/textbox.hpp"
#include "../../include/Strata/style/color.hpp"
#include <sstream>
#include <algorithm>

namespace strata {

TextBox& TextBox::set_text(std::string text) {
    lines_.clear();
    scroll_ = 0;
    std::istringstream ss(text);
    std::string line;
    while (std::getline(ss, line))
        lines_.push_back(std::move(line));
    mark_dirty();
    return *this;
}

TextBox& TextBox::set_wrap(bool wrap) {
    wrap_ = wrap;
    mark_dirty();
    return *this;
}

TextBox& TextBox::append(std::string line) {
    lines_.push_back(std::move(line));
    // Sentinel: clamp happens in render() to scroll to bottom
    scroll_ = static_cast<int>(lines_.size()) + 9999;
    mark_dirty();
    return *this;
}

TextBox& TextBox::clear() {
    lines_.clear();
    scroll_ = 0;
    mark_dirty();
    return *this;
}

std::vector<std::string> TextBox::display_lines(int width) const {
    if (!wrap_ || width <= 0) return lines_;

    std::vector<std::string> result;
    for (const auto& line : lines_) {
        if (line.empty()) { result.push_back(""); continue; }

        std::istringstream ss(line);
        std::string word, current;
        while (ss >> word) {
            if (current.empty()) {
                current = word;
            } else if (static_cast<int>(current.size() + 1 + word.size()) <= width) {
                current += ' ';
                current += word;
            } else {
                result.push_back(std::move(current));
                current = word;
            }
        }
        if (!current.empty()) result.push_back(std::move(current));
    }
    return result;
}

void TextBox::render(Canvas& canvas) {
    canvas.fill(U' ');

    auto disp  = display_lines(canvas.width());
    int  total = static_cast<int>(disp.size());
    int  h     = canvas.height();

    // Clamp scroll (handles both normal and sentinel "scroll to bottom")
    scroll_ = std::clamp(scroll_, 0, std::max(0, total - h));

    Style s = Style{}.with_fg(color::White);

    for (int i = 0; i < h && (i + scroll_) < total; ++i)
        canvas.draw_text(0, i, disp[i + scroll_], s);

    dirty_ = false;
}

bool TextBox::handle_event(const Event& e) {
    // Down: key 258 or 'j'
    if (is_key(e, 258) || is_char(e, 'j')) {
        ++scroll_;
        mark_dirty();
        return true;
    }
    // Up: key 259 or 'k'
    if (is_key(e, 259) || is_char(e, 'k')) {
        if (scroll_ > 0) --scroll_;
        mark_dirty();
        return true;
    }
    return false;
}

} // namespace strata
