#include "../../include/Strata/layout/layout.hpp"
#include <algorithm>
#include <numeric>

namespace strata {

Layout::Layout(Direction dir) : dir_(dir) {}

Layout& Layout::set_gap(int gap)      { gap_     = gap; return *this; }
Layout& Layout::set_justify(Justify j) { justify_ = j;   return *this; }

std::vector<Rect> Layout::split(Rect area, const std::vector<Constraint>& constraints) const {
    if (constraints.empty()) return {};

    const int n       = static_cast<int>(constraints.size());
    const bool horiz  = (dir_ == Direction::Horizontal);
    const int total   = horiz ? area.width : area.height;
    const int total_gap = gap_ * std::max(0, n - 1);
    const int available = std::max(0, total - total_gap);

    std::vector<int> sizes(n, 0);
    int allocated = 0;
    int fill_weight_total = 0;

    // Pass 1: resolve Fixed, Percentage, Min
    for (int i = 0; i < n; ++i) {
        const auto& c = constraints[i];
        switch (c.type) {
            case Constraint::Type::Fixed:
                sizes[i] = c.value;
                allocated += sizes[i];
                break;
            case Constraint::Type::Percentage:
                sizes[i] = (available * c.value) / 100;
                allocated += sizes[i];
                break;
            case Constraint::Type::Min:
                sizes[i] = c.value; // tentative minimum
                allocated += sizes[i];
                break;
            case Constraint::Type::Max:
                // Treat as fill with a cap; resolved in pass 2
                fill_weight_total += 1;
                break;
            case Constraint::Type::Fill:
                fill_weight_total += c.value;
                break;
        }
    }

    // Pass 2: distribute remaining space among Fill / Max widgets
    int remaining = std::max(0, available - allocated);

    if (fill_weight_total > 0 && remaining > 0) {
        int last_fill_idx = -1;
        int distributed = 0;

        for (int i = 0; i < n; ++i) {
            const auto& c = constraints[i];
            if (c.type == Constraint::Type::Fill) {
                sizes[i] = (remaining * c.value) / fill_weight_total;
                distributed += sizes[i];
                last_fill_idx = i;
            } else if (c.type == Constraint::Type::Max) {
                sizes[i] = remaining / fill_weight_total;
                sizes[i] = std::min(sizes[i], c.value); // apply cap
                distributed += sizes[i];
                last_fill_idx = i;
            }
        }

        // Give integer rounding remainder to the last Fill widget
        if (last_fill_idx >= 0) {
            sizes[last_fill_idx] += (remaining - distributed);
            // Re-apply Max cap after adjustment
            if (constraints[last_fill_idx].type == Constraint::Type::Max) {
                sizes[last_fill_idx] = std::min(sizes[last_fill_idx], constraints[last_fill_idx].value);
            }
        }
    }

    // Pass 3: convert sizes to Rects with justify alignment
    // true_remaining is 0 when Fill constraints absorbed all space (justify is a no-op)
    int sum_sizes   = std::accumulate(sizes.begin(), sizes.end(), 0);
    int true_remaining = std::max(0, available - sum_sizes);

    // Per-interval gaps (between consecutive items); default is gap_
    std::vector<int> gaps(std::max(0, n - 1), gap_);
    int start_offset = horiz ? area.x : area.y;

    if (true_remaining > 0) {
        switch (justify_) {
            case Justify::Start:
                break;
            case Justify::Center:
                start_offset += true_remaining / 2;
                break;
            case Justify::End:
                start_offset += true_remaining;
                break;
            case Justify::SpaceBetween:
                if (n > 1) {
                    int extra    = true_remaining / (n - 1);
                    int leftover = true_remaining % (n - 1);
                    for (int i = 0; i < n - 1; ++i)
                        gaps[i] = gap_ + extra + (i < leftover ? 1 : 0);
                }
                break;
            case Justify::SpaceAround: {
                int slot = true_remaining / n;
                start_offset += slot / 2;
                for (int i = 0; i < n - 1; ++i)
                    gaps[i] = gap_ + slot;
                break;
            }
        }
    }

    std::vector<Rect> rects(n);
    int pos = start_offset;

    for (int i = 0; i < n; ++i) {
        if (horiz) {
            rects[i] = {pos, area.y, sizes[i], area.height};
        } else {
            rects[i] = {area.x, pos, area.width, sizes[i]};
        }
        if (i < n - 1) pos += sizes[i] + gaps[i];
    }

    return rects;
}

} // namespace strata
