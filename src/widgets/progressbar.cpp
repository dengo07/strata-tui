#include "../../include/Strata/widgets/progressbar.hpp"
#include "../../include/Strata/style/color.hpp"
#include <algorithm>
#include <string>

namespace strata {

ProgressBar& ProgressBar::set_value(float v) {
    value_ = std::clamp(v, 0.0f, 1.0f);
    mark_dirty();
    return *this;
}

ProgressBar& ProgressBar::set_value(int current, int total) {
    if (total > 0)
        value_ = std::clamp(static_cast<float>(current) / static_cast<float>(total), 0.0f, 1.0f);
    else
        value_ = 0.0f;
    mark_dirty();
    return *this;
}

ProgressBar& ProgressBar::set_show_percent(bool show) {
    show_percent_ = show;
    mark_dirty();
    return *this;
}

void ProgressBar::render(Canvas& canvas) {
    canvas.fill(U' ');

    int pct = static_cast<int>(value_ * 100.0f + 0.5f);

    // Reserve space for " XX%" suffix
    std::string pct_str;
    int bar_w = canvas.width();
    if (show_percent_) {
        pct_str = " " + std::to_string(pct) + "%";
        bar_w -= static_cast<int>(pct_str.size());
    }
    if (bar_w < 0) bar_w = 0;

    int filled = static_cast<int>(static_cast<float>(bar_w) * value_ + 0.5f);
    filled = std::clamp(filled, 0, bar_w);

    // Draw bar: U+2588 FULL BLOCK (█) / U+2591 LIGHT SHADE (░)
    for (int i = 0; i < bar_w; ++i) {
        bool f = (i < filled);
        canvas.draw_text(i, 0,
            f ? "\xe2\x96\x88" : "\xe2\x96\x91",
            f ? Style{}.with_fg(color::BrightCyan)
              : Style{}.with_fg(color::BrightBlack));
    }

    if (show_percent_ && !pct_str.empty()) {
        canvas.draw_text(bar_w, 0, pct_str, Style{}.with_fg(color::White));
    }

    dirty_ = false;
}

} // namespace strata
