#include "../../include/Strata/widgets/block.hpp"

namespace strata {

Block& Block::set_title(std::string title) {
    title_ = std::move(title);
    mark_dirty();
    return *this;
}

Block& Block::set_border_style(Style s) {
    border_style_ = s;
    mark_dirty();
    return *this;
}

Block& Block::set_title_style(Style s) {
    title_style_ = s;
    mark_dirty();
    return *this;
}

Block& Block::set_focused_border_style(Style s) {
    focused_border_style_ = s;
    has_focused_styles_ = true;
    mark_dirty();
    return *this;
}

Block& Block::set_focused_title_style(Style s) {
    focused_title_style_ = s;
    has_focused_styles_ = true;
    mark_dirty();
    return *this;
}

Widget* Block::set_inner(std::unique_ptr<Widget> w) {
    Widget* raw = w.get();
    w->parent_ = this;
    inner_ = std::move(w);
    mark_dirty();
    return raw;
}

void Block::render(Canvas& canvas) {
    bool use_focused = has_focused_styles_ && inner_ &&
                       (inner_->is_focused() || inner_->has_focused_descendant());
    const Style& border = use_focused ? focused_border_style_ : border_style_;
    const Style& title  = use_focused ? focused_title_style_  : title_style_;

    canvas.fill(U' ', border);
    canvas.draw_border(border);
    if (!title_.empty()) {
        canvas.draw_title(title_, title);
    }

    if (inner_) {
        Rect inner_abs = canvas.area().inner(1);
        if (!inner_abs.empty()) {
            inner_->last_rect_ = inner_abs;
            Canvas inner_canvas = canvas.sub_canvas(inner_abs);
            inner_->render(inner_canvas);
            inner_->dirty_ = false;
        }
    }
    dirty_ = false;
}

bool Block::handle_event(const Event& e) {
    if (inner_) return inner_->handle_event(e);
    return false;
}

void Block::for_each_child(const std::function<void(Widget&)>& visitor) {
    if (inner_) visitor(*inner_);
}

} // namespace strata
