#include "../../include/Strata/widgets/scroll_view.hpp"
#include "../../include/Strata/events/event.hpp"
#include "../../include/Strata/style/cell.hpp"
#include "../../include/Strata/style/color.hpp"
#include <algorithm>

namespace strata {

ScrollView::ScrollView(Layout layout) : layout_(layout) {}

Widget* ScrollView::add(std::unique_ptr<Widget> w, Constraint c) {
    w->parent_ = this;
    children_.push_back(std::move(w));
    constraints_.push_back(c);
    mark_dirty();
    return children_.back().get();
}

void ScrollView::scroll_to(int y) {
    scroll_y_ = y;
    mark_dirty();
}

void ScrollView::scroll_by(int delta) {
    scroll_y_ += delta;
    mark_dirty();
}

void ScrollView::render(Canvas& canvas) {
    dirty_ = false;
    if (children_.empty()) return;

    const Rect& area = canvas.area();
    int vis_h = area.height;
    int vis_w = area.width;

    // First layout pass with full width — used to determine total content height
    auto rects = layout_.split(area, constraints_);
    int content_h = 0;
    for (const auto& r : rects)
        content_h = std::max(content_h, (r.y - area.y) + r.height);

    // If content overflows and there is room, reserve the rightmost column for
    // the scrollbar and redo layout in the narrower area.
    const bool need_bar = (content_h > vis_h) && (vis_w > 1);
    if (need_bar) {
        Rect narrow = {area.x, area.y, vis_w - 1, vis_h};
        rects = layout_.split(narrow, constraints_);
        // Heights are unchanged for a vertical layout, so content_h stays valid.
    }

    // Auto-scroll to keep focused descendant visible
    for (int i = 0; i < static_cast<int>(children_.size()); ++i) {
        Widget* child = children_[i].get();
        if (child->is_focused() || child->has_focused_descendant()) {
            int child_top = rects[i].y - area.y;
            int child_bot = child_top + rects[i].height;
            if (child_top < scroll_y_)         scroll_y_ = child_top;
            if (child_bot > scroll_y_ + vis_h) scroll_y_ = child_bot - vis_h;
            break;
        }
    }

    // Clamp scroll_y_ to valid range
    const int max_scroll = std::max(0, content_h - vis_h);
    scroll_y_ = std::max(0, std::min(scroll_y_, max_scroll));

    // Render each child shifted by scroll offset
    for (int i = 0; i < static_cast<int>(children_.size()); ++i) {
        Rect shifted = {rects[i].x, rects[i].y - scroll_y_, rects[i].width, rects[i].height};
        children_[i]->last_rect_ = shifted;
        Canvas child_canvas = canvas.sub_canvas(shifted);
        if (!child_canvas.area().empty())
            children_[i]->render(child_canvas);
        children_[i]->dirty_ = false;
    }

    // ── Scrollbar ────────────────────────────────────────────────────────────
    if (need_bar) {
        const int bar_x = vis_w - 1;

        // Thumb height: proportional to the visible fraction, minimum 1 row
        const int thumb_h = std::max(1, vis_h * vis_h / content_h);

        // Thumb top: maps scroll_y_ ∈ [0, max_scroll] → [0, vis_h - thumb_h]
        const int thumb_top = (max_scroll > 0)
            ? (scroll_y_ * (vis_h - thumb_h)) / max_scroll
            : 0;

        const Style track_style = Style{}.with_fg(color::BrightBlack);
        const Style thumb_style = is_focused()
            ? Style{}.with_fg(color::BrightWhite)
            : Style{}.with_fg(color::White);

        for (int y = 0; y < vis_h; ++y) {
            const bool in_thumb = (y >= thumb_top && y < thumb_top + thumb_h);
            canvas.draw_cell(bar_x, y, Cell{
                in_thumb ? U'\u2588' : U'\u2591',   // █ or ░
                in_thumb ? thumb_style : track_style
            });
        }
    }
}

bool ScrollView::handle_event(const Event& e) {
    if (!std::holds_alternative<KeyEvent>(e)) return false;
    const auto& ke = std::get<KeyEvent>(e);

    // Use the last known rendered height for PgDn/PgUp
    int vis_h = std::max(1, last_rect_.height);
    int delta = 0;

    // j/k and ↓/↑ are intentionally NOT handled here so they bubble up to
    // the application's on_event handler for cursor-based navigation.
    if      (ke.key == 338) delta =  std::max(1, vis_h - 1); // PgDn
    else if (ke.key == 339) delta = -std::max(1, vis_h - 1); // PgUp

    if (delta != 0) {
        scroll_y_ += delta;
        mark_dirty();
        return true;
    }
    return false;
}

void ScrollView::for_each_child(const std::function<void(Widget&)>& visitor) {
    for (auto& child : children_) {
        visitor(*child);
    }
}

} // namespace strata
