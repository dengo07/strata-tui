#include "../../include/Strata/core/widget.hpp"

namespace strata {

bool Widget::s_needs_rerender_ = false;

void Widget::mark_dirty() {
    dirty_ = true;
    if (parent_) parent_->mark_dirty();
}

bool Widget::has_focused_descendant() {
    bool found = false;
    for_each_child([&found](Widget& child) {
        if (!found && (child.is_focused() || child.has_focused_descendant()))
            found = true;
    });
    return found;
}

} // namespace strata
