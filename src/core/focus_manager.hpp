#pragma once
#include <vector>

namespace strata {

class Widget;

class FocusManager {
    std::vector<Widget*> tab_order_;
    int current_idx_ = -1;

    struct Scope { std::vector<Widget*> tab_order; int idx; };
    std::vector<Scope> scope_stack_;

    void collect(Widget* w);
    void apply_focus(int idx);

public:
    // Rebuild the tab order via DFS using Widget::for_each_child
    void rebuild(Widget* root);

    void focus_next();
    void focus_prev();
    void focus_next_group();
    void focus_prev_group();
    void focus_next_local();
    void focus_prev_local();
    void focus(Widget* w);

    // Save current focus state and rebuild from root (for modal focus trapping)
    void push_scope(Widget* root);
    // Restore previously saved focus state
    void pop_scope();

    Widget* focused() const;
    bool    has_focus() const { return current_idx_ >= 0; }
};

} // namespace strata
