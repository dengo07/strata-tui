#include "../../include/Strata/widgets/label.hpp"
#include <sstream>

namespace strata {

Label::Label(std::string text, Style style)
    : text_(std::move(text)), style_(style) {}

Label& Label::set_text(std::string text) {
    text_ = std::move(text);
    mark_dirty();
    return *this;
}

Label& Label::set_style(Style style) {
    style_ = style;
    mark_dirty();
    return *this;
}

Label& Label::set_wrap(bool wrap) {
    wrap_ = wrap;
    mark_dirty();
    return *this;
}

void Label::render(Canvas& canvas) {
    if (!wrap_) {
        canvas.draw_text(0, 0, text_, style_);
    } else {
        // Simple word-wrap
        int max_w = canvas.width();
        if (max_w <= 0) { dirty_ = false; return; }

        int row = 0;
        std::istringstream stream(text_);
        std::string word;
        std::string current_line;

        while (stream >> word) {
            if (current_line.empty()) {
                current_line = word;
            } else if (static_cast<int>(current_line.size() + 1 + word.size()) <= max_w) {
                current_line += ' ';
                current_line += word;
            } else {
                if (row >= canvas.height()) break;
                canvas.draw_text(0, row++, current_line, style_);
                current_line = word;
            }
        }
        if (!current_line.empty() && row < canvas.height()) {
            canvas.draw_text(0, row, current_line, style_);
        }
    }
    dirty_ = false;
}

} // namespace strata
