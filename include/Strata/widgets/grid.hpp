#pragma once
#include "../core/widget.hpp"
#include "../layout/layout.hpp"
#include "../layout/constraint.hpp"
#include <vector>
#include <memory>
#include <utility>
#include <functional>
#include <algorithm>

namespace strata {

// Grid — two-dimensional layout container.
//
// Children are placed left-to-right, top-to-bottom (row-major order).
//
// Column count can be:
//   - Fixed:  set_cols(n)
//   - Auto:   leave cols at 0 (default) — computed each frame from
//             available width and min_col_width (default 10).
//
// Sizing:
//   - Column widths default to equal fill; override with set_col_constraints().
//   - Row heights default to equal fill;   override with set_row_constraints().
//   - Constraint vectors are re-used cyclically if shorter than actual count.
//
// Gaps:
//   set_gap(g)      — both axes
//   set_col_gap(g)  — horizontal gap between columns
//   set_row_gap(g)  — vertical gap between rows
class Grid : public Widget {
protected:
    std::vector<std::unique_ptr<Widget>> children_;

    int  cols_          = 0;   // 0 = auto-compute
    int  min_col_width_ = 10;  // used when cols_ == 0
    int  col_gap_       = 0;
    int  row_gap_       = 0;

    // Per-column / per-row constraint templates.
    // Reused cyclically when the vector is shorter than the actual count.
    std::vector<Constraint> col_constraints_; // empty = uniform fill(1)
    std::vector<Constraint> row_constraints_; // empty = uniform fill(1)

public:
    Grid() = default;

    // ── Configuration ────────────────────────────────────────────────────────

    // Fixed column count. 0 = auto (uses min_col_width).
    Grid& set_cols(int n)          { cols_ = n; mark_dirty(); return *this; }

    // Minimum column width for auto-column mode (ignored when cols_ > 0).
    Grid& set_min_col_width(int n) { min_col_width_ = std::max(1, n); mark_dirty(); return *this; }

    // Set both col_gap and row_gap.
    Grid& set_gap(int g)           { col_gap_ = row_gap_ = g; mark_dirty(); return *this; }
    Grid& set_col_gap(int g)       { col_gap_ = g; mark_dirty(); return *this; }
    Grid& set_row_gap(int g)       { row_gap_ = g; mark_dirty(); return *this; }

    // Per-column size constraints. Reused cyclically if fewer than num_cols.
    // e.g. {fixed(20), fill()} alternates fixed/fill columns.
    Grid& set_col_constraints(std::vector<Constraint> cc) {
        col_constraints_ = std::move(cc); mark_dirty(); return *this;
    }

    // Per-row size constraints. Reused cyclically if fewer than num_rows.
    Grid& set_row_constraints(std::vector<Constraint> rc) {
        row_constraints_ = std::move(rc); mark_dirty(); return *this;
    }

    // ── Children ─────────────────────────────────────────────────────────────

    Widget* add(std::unique_ptr<Widget> widget);

    template<typename W, typename... Args>
    W* add(Args&&... args) {
        auto w = std::make_unique<W>(std::forward<Args>(args)...);
        W* raw = w.get();
        add(std::move(w));
        return raw;
    }

    void remove(Widget* w);
    void clear();

    const std::vector<std::unique_ptr<Widget>>& children() const { return children_; }

    // ── Widget interface ─────────────────────────────────────────────────────

    void render(Canvas& canvas) override;
    bool handle_event(const Event& e) override { (void)e; return false; }
    void for_each_child(const std::function<void(Widget&)>& visitor) override;
};

} // namespace strata
