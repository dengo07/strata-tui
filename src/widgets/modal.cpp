#include "../../include/Strata/widgets/modal.hpp"
#include "../../include/Strata/render/canvas.hpp"
#include "../../include/Strata/events/event.hpp"
#include "../../include/Strata/core/rect.hpp"

namespace strata {

Widget* Modal::set_inner(std::unique_ptr<Widget> w) {
    if (w) w->parent_ = this;
    inner_ = std::move(w);
    mark_dirty();
    return inner_.get();
}

void Modal::render(Canvas& canvas) {
    // 1. Dim the entire screen
    canvas.fill(U' ', overlay_style_);

    int screen_w = canvas.width();
    int screen_h = canvas.height();

    // 2. Compute centered box (clamped to screen)
    int w = std::min(preferred_w_, screen_w);
    int h = std::min(preferred_h_, screen_h);
    int bx = canvas.area().x + (screen_w - w) / 2;
    int by = canvas.area().y + (screen_h - h) / 2;
    Rect box_rect{bx, by, w, h};

    // 3. Fill box area with default background
    Canvas box_canvas = canvas.sub_canvas(box_rect);
    box_canvas.fill(U' ');

    // 4. Draw border and title
    box_canvas.draw_border(border_style_);
    if (!title_.empty()) {
        box_canvas.draw_title(title_, title_style_);
    }

    // 5. Render inner widget in the inner area (shrunk by border)
    if (inner_) {
        Rect inner_rect = box_rect.inner(1);
        if (!inner_rect.empty()) {
            inner_->last_rect_ = inner_rect;
            Canvas inner_canvas = canvas.sub_canvas(inner_rect);
            inner_->render(inner_canvas);
            inner_->dirty_ = false;
        }
    }

    dirty_ = false;
}

bool Modal::handle_event(const Event& e) {
    // Escape closes the modal
    if (is_key(e, 27)) {
        if (on_close_) on_close_();
        return true;
    }
    return false;
}

void Modal::for_each_child(const std::function<void(Widget&)>& visitor) {
    if (inner_) visitor(*inner_);
}

} // namespace strata
