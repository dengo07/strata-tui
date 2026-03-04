#include "../../include/Strata/core/alert_manager.hpp"
#include "../../include/Strata/style/color.hpp"
#include <algorithm>
#include <sstream>

namespace strata {

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

int AlertManager::add(std::string title, std::string message, Level level) {
    int id = next_id_++;
    alerts_.push_back({id, std::move(title), std::move(message), level});
    dirty_ = true;
    return id;
}

void AlertManager::dismiss(int id) {
    auto it = std::find_if(alerts_.begin(), alerts_.end(),
                           [id](const Alert& a){ return a.id == id; });
    if (it != alerts_.end()) {
        alerts_.erase(it);
        dirty_ = true;
    }
}

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------

void AlertManager::render(Canvas& canvas) {
    if (alerts_.empty()) return;

    const int screen_w = canvas.width();
    const int screen_h = canvas.height();

    // Start stacking from 1 row above the bottom edge (1-row margin)
    int y_cursor = screen_h - 1;

    // Render bottom-to-top so that the first alert is lowest
    for (auto it = alerts_.rbegin(); it != alerts_.rend(); ++it) {
        const Alert& alert = *it;

        // Word-wrap the message into at most kMaxMsgLines lines
        std::vector<std::string> msg_lines;
        if (!alert.message.empty()) {
            msg_lines = wrap(alert.message, kWidth - 4); // 2 border + 2 padding
            if ((int)msg_lines.size() > kMaxMsgLines)
                msg_lines.resize(kMaxMsgLines);
        }

        // Height: 2 (borders) + 1 (title) + message lines, minimum 3
        int h = 2 + 1 + (int)msg_lines.size();
        if (h < 3) h = 3;

        int x = screen_w - kWidth - 1;
        if (x < 0) break; // terminal too narrow to show alerts

        int y = y_cursor - h;

        // Don't render if it would be fully off-screen
        if (y < 0 && y + h <= 0) break;

        Rect alert_rect{x, y, kWidth, h};
        Canvas ac = canvas.sub_canvas(alert_rect);

        // Background fill
        ac.fill(U' ', Style{}.with_bg(color::Default));

        // Border
        ac.draw_border(border_style_for(alert.level));

        // Level label in the top border
        ac.draw_title(" " + level_label(alert.level) + " ", title_style_for(alert.level));

        // Title line (row 1, col 2)
        if (!alert.title.empty()) {
            Style title_style;
            title_style.bold = true;
            ac.draw_text(2, 1, alert.title, title_style);
        }

        // Message lines
        for (int i = 0; i < (int)msg_lines.size(); ++i) {
            ac.draw_text(2, 2 + i, msg_lines[i], {});
        }

        // Advance cursor upward (gap of 1 row between alerts)
        y_cursor = y - 1;
        if (y_cursor <= 0) break;
    }
}

// ---------------------------------------------------------------------------
// Style helpers
// ---------------------------------------------------------------------------

Style AlertManager::border_style_for(Level l) const {
    Style s;
    switch (l) {
        case Level::Info:    s.fg = color::Cyan;    break;
        case Level::Success: s.fg = color::Green;   break;
        case Level::Warning: s.fg = color::Yellow;  break;
        case Level::Error:   s.fg = color::Red;     break;
    }
    return s;
}

Style AlertManager::title_style_for(Level l) const {
    Style s;
    s.bold = true;
    switch (l) {
        case Level::Info:    s.fg = color::Cyan;    break;
        case Level::Success: s.fg = color::Green;   break;
        case Level::Warning: s.fg = color::Yellow;  break;
        case Level::Error:   s.fg = color::Red;     break;
    }
    return s;
}

std::string AlertManager::level_label(Level l) const {
    switch (l) {
        case Level::Info:    return "● Info";
        case Level::Success: return "● Success";
        case Level::Warning: return "● Warning";
        case Level::Error:   return "● Error";
    }
    return "";
}

// ---------------------------------------------------------------------------
// Word-wrap helper
// ---------------------------------------------------------------------------

std::vector<std::string> AlertManager::wrap(const std::string& text, int width) {
    std::vector<std::string> lines;
    if (text.empty() || width <= 0) return lines;

    std::istringstream stream(text);
    std::string word;
    std::string current;

    while (stream >> word) {
        if (current.empty()) {
            current = word;
        } else if ((int)(current.size() + 1 + word.size()) <= width) {
            current += ' ';
            current += word;
        } else {
            lines.push_back(current);
            current = word;
        }
    }
    if (!current.empty()) lines.push_back(current);

    return lines;
}

} // namespace strata
