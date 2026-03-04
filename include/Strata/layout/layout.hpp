#pragma once
#include "constraint.hpp"
#include "../core/rect.hpp"
#include <vector>

namespace strata {

class Layout {
public:
    enum class Direction { Horizontal, Vertical };
    enum class Justify   { Start, Center, End, SpaceBetween, SpaceAround };

    explicit Layout(Direction dir = Direction::Vertical);

    Layout& set_gap(int gap);
    Layout& set_justify(Justify j);

    // Split area into N Rects according to constraints.
    // Returns absolute Rects (offset by area.x, area.y).
    // Output vector length equals constraints.size().
    std::vector<Rect> split(Rect area, const std::vector<Constraint>& constraints) const;

private:
    Direction dir_;
    int       gap_     = 0;
    Justify   justify_ = Justify::Start;
};

} // namespace strata
