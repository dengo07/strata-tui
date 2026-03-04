#include "../../include/Strata/widgets/spinner.hpp"
#include "../../include/Strata/style/color.hpp"

namespace strata {

// Braille spinner frames (UTF-8 encoded)
static const char* const kSpinnerFrames[10] = {
    "\xe2\xa0\x8b",  // ⠋
    "\xe2\xa0\x99",  // ⠙
    "\xe2\xa0\xb9",  // ⠹
    "\xe2\xa0\xb8",  // ⠸
    "\xe2\xa0\xbc",  // ⠼
    "\xe2\xa0\xb4",  // ⠴
    "\xe2\xa0\xa6",  // ⠦
    "\xe2\xa0\xa7",  // ⠧
    "\xe2\xa0\x87",  // ⠇
    "\xe2\xa0\x8f",  // ⠏
};

Spinner::Spinner(std::string label)
    : label_(std::move(label)) {}

Spinner& Spinner::set_label(std::string label) {
    label_ = std::move(label);
    mark_dirty();
    return *this;
}

void Spinner::tick() {
    frame_ = (frame_ + 1) % kNumFrames;
    mark_dirty();
}

Spinner& Spinner::set_auto_animate(bool enable, int every) {
    auto_animate_  = enable;
    animate_every_ = every > 0 ? every : 1;
    render_count_  = 0;
    return *this;
}

void Spinner::render(Canvas& canvas) {
    canvas.fill(U' ');

    Style spin_s  = Style{}.with_fg(color::BrightCyan).with_bold();
    Style label_s = Style{}.with_fg(color::BrightBlack);

    // Center vertically
    int y = canvas.height() / 2;

    canvas.draw_text(0, y, kSpinnerFrames[frame_], spin_s);
    if (!label_.empty())
        canvas.draw_text(2, y, label_, label_s);

    dirty_ = false;

    // Auto-animate: advance frame every animate_every_ renders.
    // Sets s_needs_rerender_ so App re-marks root dirty after the frame completes,
    // ensuring continuous animation without stalling the dirty-flag cleanup.
    if (auto_animate_) {
        ++render_count_;
        if (render_count_ >= animate_every_) {
            render_count_ = 0;
            frame_ = (frame_ + 1) % kNumFrames;
            s_needs_rerender_ = true;
        }
    }
}

} // namespace strata
