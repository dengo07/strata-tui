#pragma once
#include "../core/widget.hpp"
#include "../layout/layout.hpp"
#include "../layout/constraint.hpp"
#include <vector>
#include <memory>
#include <functional>

namespace strata {

// ScrollView — vertical (or horizontal) container with scroll offset.
// - is_focusable() == true so it always receives keyboard events.
// - j/k/Down/Up scroll by 1; PgDn/PgUp scroll by (vis_h - 1).
// - Auto-scrolls to keep the focused descendant visible.
class ScrollView : public Widget {
    std::vector<std::unique_ptr<Widget>> children_;
    std::vector<Constraint>              constraints_;
    Layout                               layout_;
    int                                  scroll_y_ = 0;

public:
    explicit ScrollView(Layout layout = Layout(Layout::Direction::Vertical));

    // Add a widget with an explicit constraint
    Widget* add(std::unique_ptr<Widget> w, Constraint c = Constraint::fill());

    // Construct in-place with explicit constraint (first arg)
    template<typename W, typename... Args>
    W* add(Constraint c, Args&&... args) {
        auto w = std::make_unique<W>(std::forward<Args>(args)...);
        W* raw = w.get();
        add(std::move(w), c);
        return raw;
    }

    // Construct in-place, constraint derived from widget's flex field
    template<typename W, typename... Args>
    W* add(Args&&... args) {
        auto w = std::make_unique<W>(std::forward<Args>(args)...);
        W* raw = w.get();
        add(std::move(w), Constraint::fill(raw->flex));
        return raw;
    }

    void scroll_to(int y);
    void scroll_by(int delta);
    int  scroll_y() const { return scroll_y_; }

    bool is_focusable() const override { return true; }
    void render(Canvas& canvas) override;
    bool handle_event(const Event& e) override;
    void for_each_child(const std::function<void(Widget&)>& visitor) override;
};

} // namespace strata
