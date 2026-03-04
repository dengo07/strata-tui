#pragma once
#include "../core/widget.hpp"
#include "../style/style.hpp"

namespace strata {

// Horizontal progress bar: ████████░░░░ 60%
// Non-focusable; updated via set_value().
class ProgressBar : public Widget {
    float value_        = 0.0f;  // 0.0–1.0
    bool  show_percent_ = true;

public:
    // Set value in [0, 1]
    ProgressBar& set_value(float v);
    // Convenience: set_value(current, total)
    ProgressBar& set_value(int current, int total);
    ProgressBar& set_show_percent(bool show);

    float value() const { return value_; }

    bool is_focusable() const override { return false; }
    void render(Canvas& canvas) override;
};

} // namespace strata
