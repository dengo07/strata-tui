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
    Layout layout_;

public:
    explicit Container(Layout layout = Layout(Layout::Direction::Vertical));

    // Add widget with explicit constraint
    Widget* add(std::unique_ptr<Widget> widget, Constraint c = Constraint::fill());

    // Construct widget in-place with optional explicit constraint
    template<typename W, typename... Args>
    W* add(Args&&... args) {
        auto w = std::make_unique<W>(std::forward<Args>(args)...);
        W* raw = w.get();
        add(std::move(w), Constraint::fill(raw->flex));
        return raw;
    }

    template<typename W, typename... Args>
    W* add(Constraint c, Args&&... args) {
        auto w = std::make_unique<W>(std::forward<Args>(args)...);
        W* raw = w.get();
        add(std::move(w), c);
        return raw;
    }

    void render(Canvas& canvas) override;
    bool handle_event(const Event& e) override;
    void for_each_child(const std::function<void(Widget&)>& visitor) override;

    const std::vector<std::unique_ptr<Widget>>& children() const { return children_; }
    void clear() {
        children_.clear();
        mark_dirty();
    }
};

} // namespace strata
