#include "../../include/Strata/widgets/grid.hpp"
#include <algorithm>
#include <cmath>

namespace strata {

// ── Children management ───────────────────────────────────────────────────────

Widget* Grid::add(std::unique_ptr<Widget> widget) {
    Widget* raw = widget.get();
    widget->parent_ = this;
    children_.push_back(std::move(widget));
    mark_dirty();
    if (mounted_ && s_on_subtree_added_) s_on_subtree_added_(raw);
    return raw;
}

void Grid::remove(Widget* w) {
    for (size_t i = 0; i < children_.size(); ++i) {
        if (children_[i].get() == w) {
            if (mounted_ && s_on_subtree_removed_) s_on_subtree_removed_(children_[i].get());
            children_.erase(children_.begin() + i);
            mark_dirty();
            if (mounted_ && s_on_focus_rebuild_) s_on_focus_rebuild_();
            return;
        }
    }
}

void Grid::clear() {
    if (mounted_ && s_on_subtree_removed_)
        for (auto& child : children_) s_on_subtree_removed_(child.get());
    children_.clear();
    mark_dirty();
    if (mounted_ && s_on_focus_rebuild_) s_on_focus_rebuild_();
}

// ── Render ────────────────────────────────────────────────────────────────────

void Grid::render(Canvas& canvas) {
    if (children_.empty()) {
        dirty_ = false;
        return;
    }

    const Rect area = canvas.area();
    const int  n    = static_cast<int>(children_.size());

    // ── Step 1: compute effective column count ────────────────────────────────
    int num_cols;
    if (cols_ > 0) {
        num_cols = cols_;
    } else {
        // Auto: fit as many columns of at least min_col_width_ as possible.
        // Formula: num_cols = floor((W + col_gap_) / (min_col_width_ + col_gap_))
        int slot = min_col_width_ + col_gap_;
        num_cols = std::max(1, (area.width + col_gap_) / slot);
    }
    num_cols = std::min(num_cols, n); // never more columns than children

    // ── Step 2: compute row count ─────────────────────────────────────────────
    const int num_rows = (n + num_cols - 1) / num_cols; // ceil

    // ── Step 3: build column constraint vector ────────────────────────────────
    std::vector<Constraint> col_consts(num_cols);
    for (int c = 0; c < num_cols; ++c) {
        if (!col_constraints_.empty())
            col_consts[c] = col_constraints_[c % col_constraints_.size()];
        else
            col_consts[c] = Constraint::fill(1);
    }

    // ── Step 4: build row constraint vector ───────────────────────────────────
    std::vector<Constraint> row_consts(num_rows);
    for (int r = 0; r < num_rows; ++r) {
        if (!row_constraints_.empty())
            row_consts[r] = row_constraints_[r % row_constraints_.size()];
        else
            row_consts[r] = Constraint::fill(1);
    }

    // ── Step 5: split canvas into column and row slices ───────────────────────
    const auto col_rects = Layout(Layout::Direction::Horizontal)
                               .set_gap(col_gap_)
                               .split(area, col_consts);
    const auto row_rects = Layout(Layout::Direction::Vertical)
                               .set_gap(row_gap_)
                               .split(area, row_consts);

    // ── Step 6: render each child in its cell ─────────────────────────────────
    for (int i = 0; i < n; ++i) {
        const int col = i % num_cols;
        const int row = i / num_cols;

        if (col >= static_cast<int>(col_rects.size())) continue;
        if (row >= static_cast<int>(row_rects.size())) continue;

        const Rect& cr = col_rects[col];
        const Rect& rr = row_rects[row];

        Rect cell { cr.x, rr.y, cr.width, rr.height };
        if (cell.empty()) continue;

        children_[i]->last_rect_ = cell;
        Canvas child_canvas = canvas.sub_canvas(cell);
        children_[i]->render(child_canvas);
        children_[i]->dirty_ = false;
    }

    dirty_ = false;
}

// ── Traversal ─────────────────────────────────────────────────────────────────

void Grid::for_each_child(const std::function<void(Widget&)>& visitor) {
    for (auto& child : children_) visitor(*child);
}

} // namespace strata
