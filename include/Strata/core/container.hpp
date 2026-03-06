#pragma once
#include "widget.hpp"
#include "../layout/layout.hpp"
#include "../layout/constraint.hpp"
#include <vector>
#include <memory>
#include <utility>

namespace strata {

class Container : public Widget {
protected:
    std::vector<std::unique_ptr<Widget>> children_;
    std::vector<Constraint> constraints_;
    std::vector<Constraint> cross_constraints_;   // per-child cross-axis size
    Layout layout_;
    Layout::Align cross_align_ = Layout::Align::Start; // where child sits when cross < full

public:
    explicit Container(Layout layout = Layout(Layout::Direction::Vertical));

    // Add widget with explicit main-axis and optional cross-axis constraint.
    // cross = fill() means stretch to the full cross-axis size (default behaviour).
    Widget* add(std::unique_ptr<Widget> widget,
                Constraint main  = Constraint::fill(),
                Constraint cross = Constraint::fill());

    // Construct widget in-place; main constraint derived from widget's flex field
    template<typename W, typename... Args>
    W* add(Args&&... args) {
        auto w = std::make_unique<W>(std::forward<Args>(args)...);
        W* raw = w.get();
        add(std::move(w), Constraint::fill(raw->flex));
        return raw;
    }

    // Construct widget in-place with explicit main-axis constraint
    template<typename W, typename... Args>
    W* add(Constraint main, Args&&... args) {
        auto w = std::make_unique<W>(std::forward<Args>(args)...);
        W* raw = w.get();
        add(std::move(w), main);
        return raw;
    }

    // Cross-axis alignment applied when a child's cross size is less than the full slot.
    Container& set_cross_align(Layout::Align a) { cross_align_ = a; return *this; }

    // Remove a widget previously added via add(). Calls on_unmount() if mounted.
    void remove(Widget* w);

    void render(Canvas& canvas) override;
    bool handle_event(const Event& e) override;
    void for_each_child(const std::function<void(Widget&)>& visitor) override;

    const std::vector<std::unique_ptr<Widget>>& children() const { return children_; }
    void clear();
};

} // namespace strata
