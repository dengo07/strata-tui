#pragma once
#include "../core/widget.hpp"
#include "../layout/constraint.hpp"
#include "../layout/layout.hpp"
#include <functional>
#include <memory>
#include <vector>

namespace strata {

// ── ReactiveContainer ────────────────────────────────────────────────────────
// A container that rebuilds its children list whenever a reactive source fires.
// Intended to be created and wired by the ForEach DSL descriptor — not usually
// constructed directly.
//
// Lifecycle:
//   on_mount()   → calls rebuild_fn_() to populate initial children, then
//                  subscribes to the reactive source.
//   reactive fires → refresh(): unmount old, rebuild, mount new, rebuild focus.
//   on_unmount() → unsubscribes from the reactive source.
//
class ReactiveContainer : public Widget {
public:
    explicit ReactiveContainer(
        Layout layout = Layout(Layout::Direction::Vertical));

    // Called by the ForEach builder during rebuild to add a child widget.
    void add(std::unique_ptr<Widget> w,
             Constraint main  = Constraint::fill(),
             Constraint cross = Constraint::fill());

    // Wiring — set by ForEach before the widget is mounted.
    void set_rebuild(std::function<void()> fn);
    void set_subscribe(std::function<int(std::function<void()>)> fn);
    void set_unsubscribe(std::function<void(int)> fn);

    // Widget interface
    void render(Canvas& canvas) override;
    bool handle_event(const Event& e) override { (void)e; return false; }
    void for_each_child(const std::function<void(Widget&)>& visitor) override;

protected:
    void on_mount()   override;
    void on_unmount() override;

private:
    void refresh();

    Layout                               layout_;
    std::vector<std::unique_ptr<Widget>> children_;
    std::vector<Constraint>              constraints_;
    std::vector<Constraint>              cross_constraints_;

    std::function<void()>                            rebuild_fn_;
    std::function<int(std::function<void()>)>        subscribe_fn_;
    std::function<void(int)>                         unsubscribe_fn_;
    int                                              listener_id_ = -1;
};

} // namespace strata
