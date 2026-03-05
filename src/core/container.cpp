#include "../../include/Strata/core/container.hpp"
#include <algorithm>

namespace strata {

Container::Container(Layout layout) : layout_(layout) {}

Widget* Container::add(std::unique_ptr<Widget> widget, Constraint main, Constraint cross) {
    Widget* raw = widget.get();
    widget->parent_ = this;
    children_.push_back(std::move(widget));
    constraints_.push_back(main);
    cross_constraints_.push_back(cross);
    mark_dirty();
    return raw;
}

// Resolve a cross-axis constraint to an absolute size given the full cross slot.
// fill() / min() → full slot (stretch).
// fixed(n) / max(n) → min(n, full).
// percentage(p)     → full * p / 100.
static int resolve_cross(Constraint c, int full) {
    switch (c.type) {
        case Constraint::Type::Fixed:
        case Constraint::Type::Max:
            return std::min(c.value, full);
        case Constraint::Type::Percentage:
            return std::max(0, std::min((full * c.value) / 100, full));
        case Constraint::Type::Fill:
        case Constraint::Type::Min:
        default:
            return full; // stretch
    }
}

void Container::render(Canvas& canvas) {
    if (children_.empty()) {
        dirty_ = false;
        return;
    }

    const bool horiz = (layout_.direction() == Layout::Direction::Horizontal);
    auto rects = layout_.split(canvas.area(), constraints_);

    for (size_t i = 0; i < children_.size(); ++i) {
        if (i >= rects.size() || rects[i].empty()) continue;

        Rect r = rects[i];

        // Apply cross-axis constraint when set.
        if (i < cross_constraints_.size()) {
            int full_cross  = horiz ? r.height : r.width;
            int child_cross = resolve_cross(cross_constraints_[i], full_cross);

            if (child_cross < full_cross) {
                int offset = 0;
                switch (cross_align_) {
                    case Layout::Align::Start:  offset = 0; break;
                    case Layout::Align::Center: offset = (full_cross - child_cross) / 2; break;
                    case Layout::Align::End:    offset = full_cross - child_cross; break;
                }
                if (horiz) { r.y += offset; r.height = child_cross; }
                else       { r.x += offset; r.width  = child_cross; }
            }
        }

        children_[i]->last_rect_ = r;
        Canvas child_canvas = canvas.sub_canvas(r);
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
