#pragma once
#include "../render/canvas.hpp"
#include "../events/event.hpp"
#include <functional>
#include<string>

namespace strata {

class App;
class Container;
class FocusManager;
class Block;
class ScrollView;
class Modal;
class ReactiveContainer;

class Widget {
    friend class App;
    friend class Container;
    friend class FocusManager;
    friend class Block;
    friend class ScrollView;
    friend class Modal;
    friend class ReactiveContainer;

protected:
    Widget* parent_  = nullptr;
    bool    dirty_   = true;
    bool    focused_ = false;
    bool    mounted_ = false;  // true after on_mount(), false after on_unmount()
    Rect    last_rect_;  // Last rendered bounding rect (absolute terminal coords)
    std::string focus_group_;
    // Set by animated widgets during render(); cleared by App after each frame.
    // Causes App to re-mark root dirty so animation continues next frame.
    static bool s_needs_rerender_;
    // Set by App during run() so Container/ScrollView can trigger lifecycle hooks dynamically.
    static std::function<void(Widget*)> s_on_subtree_added_;    // mount subtree + rebuild focus
    static std::function<void(Widget*)> s_on_subtree_removed_;  // unmount subtree ONLY (no rebuild)
    static std::function<void(Widget*)> s_on_mount_subtree_;    // mount subtree ONLY (no rebuild)
    static std::function<void()>        s_on_focus_rebuild_;    // rebuild focus (called after erase)

    // Lifecycle hooks — override in subclasses as needed
    virtual void on_mount()   {}
    virtual void on_unmount() {}
    virtual void on_focus()   { mark_dirty(); }
    virtual void on_blur()    { mark_dirty(); }

    // Mark this widget and all ancestors dirty (triggers re-render)
    void mark_dirty();

public:
    // Size hints for layout engine
    int min_width  = 0;
    int min_height = 0;
    // Weight for Constraint::fill() when not using explicit constraints
    int flex = 1;
    // Tab order index: lower values appear earlier in Tab cycling (same value = DFS order)
    int tab_index = 0;

    // Pure virtual: draw widget into canvas.
    // All coordinates passed to canvas are relative to canvas's origin.
    virtual void render(Canvas& canvas) = 0;

    // Return true if the event was consumed (stops bubbling)
    virtual bool handle_event(const Event& e) { (void)e; return false; }

    // Return true if this widget can receive keyboard focus via Tab cycling
    virtual bool is_focusable() const { return false; }

    // Visit all direct children. Override in composite widgets (Container, Block, etc.)
    // This replaces dynamic_cast for traversal — avoids RTTI in FocusManager / mount DFS.
    virtual void for_each_child(const std::function<void(Widget&)>& /*visitor*/) {}

    virtual ~Widget() = default;

    bool is_dirty()   const { return dirty_; }
    bool is_focused() const { return focused_; }
    Widget* parent()  const { return parent_; }

    // Returns true if any descendant widget currently has keyboard focus.
    bool has_focused_descendant();
    
    const std::string& focus_group() const { return focus_group_; }
    Widget* set_focus_group(std::string group_name) { 
        focus_group_ = std::move(group_name); 
        return this; 
    }
};

} // namespace strata
