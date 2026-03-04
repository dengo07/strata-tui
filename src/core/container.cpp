#include "../../include/Strata/core/container.hpp"

namespace strata {

Container::Container(Layout layout) : layout_(layout) {}

Widget* Container::add(std::unique_ptr<Widget> widget, Constraint c) {
    Widget* raw = widget.get();
    widget->parent_ = this;
    children_.push_back(std::move(widget));
    constraints_.push_back(c);
    mark_dirty();
    return raw;
}

void Container::render(Canvas& canvas) {
    if (children_.empty()) {
        dirty_ = false;
        return;
    }

    auto rects = layout_.split(canvas.area(), constraints_);

    for (size_t i = 0; i < children_.size(); ++i) {
        if (i >= rects.size() || rects[i].empty()) continue;
        children_[i]->last_rect_ = rects[i];
        Canvas child_canvas = canvas.sub_canvas(rects[i]);
        children_[i]->render(child_canvas);
        children_[i]->dirty_ = false;
    }
    dirty_ = false;
}

bool Container::handle_event(const Event& e) {
    // Containers don't consume events themselves;
    // routing is handled by App (focused widget → bubble up via parent_)
    (void)e;
    return false;
}

void Container::for_each_child(const std::function<void(Widget&)>& visitor) {
    for (auto& child : children_) {
        visitor(*child);
    }
}

} // namespace strata
