#pragma once
#include "../core/widget.hpp"
#include "../style/style.hpp"
#include <string>

namespace strata {

// Animated braille spinner: ⠋ ⠙ ⠹ ⠸ ⠼ ⠴ ⠦ ⠧ ⠇ ⠏
// Non-focusable. Call tick() to advance manually, or set_auto_animate(true) for
// continuous animation driven by the render loop (~10fps on a 60fps loop).
class Spinner : public Widget {
    std::string label_;
    int  frame_        = 0;
    bool auto_animate_ = false;
    int  animate_every_ = 6;   // advance frame every N render calls
    int  render_count_  = 0;

    static constexpr int kNumFrames = 10;

public:
    explicit Spinner(std::string label = "");

    Spinner& set_label(std::string label);
    // Advance one frame and mark dirty (triggers re-render on next frame)
    void tick();
    // Enable/disable automatic frame advance driven by the render loop.
    // every: advance frame once every this many render calls (default 6 → ~10fps @ 60fps)
    Spinner& set_auto_animate(bool enable, int every = 6);

    bool is_focusable() const override { return false; }
    void render(Canvas& canvas) override;
};

} // namespace strata
