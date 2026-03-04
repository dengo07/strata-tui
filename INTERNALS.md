# Strata — Internals

This document explains how the framework operates under the hood: the main loop, rendering pipeline, layout algorithm, event routing, focus system, and the DSL layer.

---

## Table of Contents

1. [Directory Layout](#1-directory-layout)
2. [Architecture Overview](#2-architecture-overview)
3. [The Main Loop](#3-the-main-loop)
4. [Dirty-Flag Rendering](#4-dirty-flag-rendering)
5. [The Backend Abstraction](#5-the-backend-abstraction)
6. [Double-Buffering](#6-double-buffering)
7. [Canvas & Coordinate Systems](#7-canvas--coordinate-systems)
8. [The Layout Algorithm](#8-the-layout-algorithm)
9. [Widget Lifecycle](#9-widget-lifecycle)
10. [Event Routing](#10-event-routing)
11. [FocusManager](#11-focusmanager)
12. [Unicode & Wide-Character Handling](#12-unicode--wide-character-handling)
13. [The DSL Layer — `ui.hpp`](#13-the-dsl-layer--uihpp)
14. [Color & Attribute Mapping](#14-color--attribute-mapping)

---

## 1. Directory Layout

```
include/Strata/          — Public API (header-only or paired with src/)
  core/
    rect.hpp             — Rect{x,y,w,h}  — constexpr value type
    widget.hpp           — Widget abstract base class
    container.hpp        — Container : Widget (layout + children)
    app.hpp              — App (entry point, owns root + backend + focus)
  events/
    event.hpp            — Event = variant<KeyEvent,MouseEvent,ResizeEvent>
  style/
    color.hpp            — Color = variant<NamedColor,RgbColor>
    style.hpp            — Style{fg,bg,bold,...}
    cell.hpp             — Cell{char32_t ch, Style}
  render/
    backend.hpp          — Backend (pure virtual interface)
    canvas.hpp           — Canvas (clipped drawing surface)
  layout/
    constraint.hpp       — Constraint{Type,value}
    layout.hpp           — Layout::split() declaration
  widgets/               — One header per widget

src/                     — Private implementations
  core/
    app.cpp
    container.cpp
    widget.cpp
    focus_manager.hpp    — FocusManager (private — not in public API)
    focus_manager.cpp
  render/
    canvas.cpp
    ncurses_backend.hpp  — NcursesBackend (private)
    ncurses_backend.cpp
  layout/
    layout.cpp
  widgets/
    *.cpp
```

`FocusManager` and `NcursesBackend` live in `src/` — they are **implementation details** and are never exposed to user code. User code only sees the `Backend` interface and is never required to include anything from `src/`.

---

## 2. Architecture Overview

```
App::run()
  │
  ├─ backend_->init()              NcursesBackend: initscr(), colors, mouse
  ├─ mount_all(root_)              DFS: call on_mount() on every widget
  ├─ focus_->rebuild(root_)        DFS collect focusable → stable_sort → focus[0]
  │
  └─ loop (16 ms poll → ~60 fps):
       │
       ├─ backend_->poll_event()   blocking up to 16 ms; returns optional<Event>
       │
       ├─ route_event()            Tab → focus_next(); Mouse → hit-test;
       │                           Key/Mouse → focused widget → bubble up parents
       │
       └─ render_frame()           if root dirty:
                                     backend_->clear()        reset back-buffer
                                     root_->render(canvas)    DFS draw
                                     backend_->flush()        diff → ncurses draw
                                     if s_needs_rerender_: mark root dirty again
```

**Key invariants:**
- The widget tree is never mutated during `render()` or event dispatch — only from callbacks
- Rendering only happens when `root_->is_dirty()` returns `true`
- A widget's `render()` call sets `dirty_ = false` at the end; the parent container does the same for the child

---

## 3. The Main Loop

**Source:** `src/core/app.cpp` — `App::run()`

```cpp
void App::run() {
    backend_->init();
    mount_all(root_.get());
    focus_->rebuild(root_.get());
    root_->mark_dirty();
    running_ = true;
    render_frame();                   // draw initial frame before first event

    while (running_) {
        auto event = backend_->poll_event(16);   // ms
        if (event) {
            if (as_resize(*event)) {
                root_->mark_dirty();             // resize: full redraw
            } else {
                route_event(*event);
            }
        }
        render_frame();
    }

    unmount_all(root_.get());
    backend_->shutdown();
}
```

The 16 ms timeout gives approximately 60 fps. The loop is **not** busy-waiting: `poll_event` blocks inside `ncurses`'s `wtimeout` + `wgetch` for up to 16 ms, then returns `nullopt` if no event arrived. `render_frame()` is then called regardless — if nothing changed (no dirty widget), it returns immediately without any drawing.

### Continuous Animation

`Spinner::render()` sets the static flag `Widget::s_needs_rerender_ = true` during its draw pass. After `render_frame()` calls `backend_->flush()`, it checks this flag:

```cpp
if (Widget::s_needs_rerender_) {
    Widget::s_needs_rerender_ = false;
    root_->mark_dirty();
}
```

This forces the next `render_frame()` call to redraw even if no event arrived — creating a continuous animation without a dedicated animation thread.

---

## 4. Dirty-Flag Rendering

**Source:** `src/core/widget.cpp`

Every `Widget` has a private `bool dirty_ = true`. When a setter modifies visual state, it calls `mark_dirty()`:

```cpp
void Widget::mark_dirty() {
    dirty_ = true;
    if (parent_) parent_->mark_dirty();
}
```

`mark_dirty()` propagates up the tree all the way to the root. `App::render_frame()` checks `root_->is_dirty()` and skips the entire draw pass if `false`. This means:

- A single widget change costs one tree walk up (O(depth), typically O(log N))
- Frames with no changes cost almost nothing (one `is_dirty()` check)
- The backend's `clear()` and `flush()` are never called for unchanged frames

After `render()` is called on a widget, it sets `dirty_ = false`. Container's `render()` does this for each child after calling `child->render()`.

---

## 5. The Backend Abstraction

**Source:** `include/Strata/render/backend.hpp`

```cpp
class Backend {
public:
    virtual void init()     = 0;
    virtual void shutdown() = 0;
    virtual int  width()  const = 0;
    virtual int  height() const = 0;
    virtual void draw_cell(int x, int y, const Cell& cell) = 0;
    virtual void clear()  = 0;
    virtual void flush()  = 0;
    virtual std::optional<Event> poll_event(int timeout_ms) = 0;
    virtual ~Backend() = default;
};
```

`App` holds `unique_ptr<Backend>`. The default implementation is `NcursesBackend` (`src/render/ncurses_backend.hpp/cpp`), which is never exposed in the public API. To swap backends (e.g., for a testing backend or a different terminal library), pass a custom `unique_ptr<Backend>` to `App`'s constructor:

```cpp
App app(std::make_unique<MyCustomBackend>());
```

`draw_cell(x, y, cell)` is the only drawing primitive. Everything the widget system draws ultimately flows through individual `draw_cell` calls. `clear()` resets the back-buffer; `flush()` pushes the back-buffer to the screen.

---

## 6. Double-Buffering

**Source:** `src/render/ncurses_backend.cpp`

`NcursesBackend` maintains two `std::vector<Cell>` buffers:

- **`back_buf_`** — written during the current frame's draw pass via `draw_cell()`
- **`front_buf_`** — the last frame that was actually rendered to the terminal

`clear()` fills `back_buf_` with blank cells. Each call to `draw_cell(x, y, cell)` writes into `back_buf_[y * w + x]`.

`flush()` walks both buffers in lockstep:

```
for each (x, y):
    if back_buf_[y*w+x] != front_buf_[y*w+x]:
        render_cell(x, y, back_buf_[y*w+x])   // call mvadd_wch
        front_buf_[y*w+x] = back_buf_[y*w+x]  // sync
```

This means ncurses `mvadd_wch` is called only for cells that actually changed. In practice, most frames only change a handful of cells (cursor position on an Input widget, or one spinner character), so the terminal receives very little data.

`front_buf_` is initialised with `Cell{U'\0', {}}` — a sentinel that never matches any real cell. This forces a complete redraw on the very first `flush()`.

**Why not use ncurses's own double-buffering?**
Ncurses already has an internal buffer, but it operates at a higher level and still does attribute comparisons internally. By maintaining our own buffers in terms of our own `Cell{char32_t, Style}` type, we:
1. Compare using our own equality (character + full Style), not ncurses internals
2. Avoid unnecessary `mvadd_wch` calls for unchanged cells before ncurses even gets involved
3. Keep rendering decoupled from ncurses details

---

## 7. Canvas & Coordinate Systems

**Source:** `include/Strata/render/canvas.hpp`, `src/render/canvas.cpp`

A `Canvas` wraps a `Backend&` and an **absolute** `Rect` (terminal screen coordinates). All drawing methods take **relative** coordinates — (0,0) is the canvas's own top-left corner.

```
Terminal screen (absolute):
  +--------------------+
  |                    |
  |    +--------+      |  ← Canvas: area_ = {x=10, y=5, w=8, h=4}
  |    |0,0→    |      |     draw_text(0, 0, ...) → terminal (10, 5)
  |    |        |      |     draw_text(3, 2, ...) → terminal (13, 7)
  |    +--------+      |
  |                    |
  +--------------------+
```

**`sub_canvas(Rect abs_rect)`** creates a child Canvas. `abs_rect` is in **absolute** coordinates (it comes directly from `Layout::split()`'s output). The method clamps it to the parent canvas's bounds:

```cpp
Canvas Canvas::sub_canvas(Rect abs_rect) const {
    // Intersect abs_rect with area_ to get the actual visible region
    int x1 = std::max(abs_rect.x, area_.x);
    int y1 = std::max(abs_rect.y, area_.y);
    int x2 = std::min(abs_rect.right(),  area_.right());
    int y2 = std::min(abs_rect.bottom(), area_.bottom());
    Rect clamped = {x1, y1, std::max(0, x2-x1), std::max(0, y2-y1)};
    return Canvas(backend_, clamped);
}
```

This design means:
- Widgets never see absolute coordinates; they always draw at (0,0)-(w,h)
- Clipping is automatic and free — a widget that draws outside its canvas is silently ignored
- `Block::render()` calls `canvas.area().inner(1)` to get the inner absolute rect (shrunk by 1px border on each side), then passes it to `sub_canvas` to give the inner widget a properly clipped surface

### How `Container::render()` Uses This

```
Container::render(canvas):
    rects = layout_.split(canvas.area(), constraints_)
    for i, child in children_:
        child->last_rect_ = rects[i]           // store absolute rect for mouse hit-test
        child_canvas = canvas.sub_canvas(rects[i])
        child->render(child_canvas)
        child->dirty_ = false
```

`last_rect_` (in absolute terminal coordinates) is stored on every widget after each render. `App::find_focusable_at()` uses it for mouse hit-testing.

---

## 8. The Layout Algorithm

**Source:** `src/layout/layout.cpp` — `Layout::split()`

Input: an absolute `Rect area` and a `vector<Constraint>`. Output: a `vector<Rect>` of the same length, positioned within `area`.

### Pass 1 — Resolve Non-Fill Constraints

```
allocated = 0
for each constraint:
    Fixed(n):      sizes[i] = n;          allocated += n
    Percentage(p): sizes[i] = area*p/100; allocated += sizes[i]
    Min(n):        sizes[i] = n;          allocated += n
    Max/Fill:      accumulate fill_weight_total
```

`available = total - gap_total`. `remaining = available - allocated`.

### Pass 2 — Distribute Remaining Space to Fill/Max

```
for each Fill(w) constraint:
    sizes[i] = remaining * w / total_fill_weight

for each Max(cap) constraint:
    sizes[i] = min(remaining / total_fill_weight, cap)

# Integer rounding: give leftover cells to the last Fill widget
sizes[last_fill_idx] += (remaining - distributed)
```

### Pass 3 — Apply Justify and Compute Rects

When `fill` constraints absorb all space, `true_remaining == 0` and `justify` has no effect. Otherwise:

| Justify | Effect |
|---|---|
| `Start` | `start_offset = area.x` (or `.y`) |
| `Center` | `start_offset += true_remaining / 2` |
| `End` | `start_offset += true_remaining` |
| `SpaceBetween` | Distributes `true_remaining` as extra gaps between children |
| `SpaceAround` | Distributes `true_remaining` as slots around and between children |

Finally, rects are computed by walking from `start_offset`, advancing by `sizes[i] + gaps[i]`.

### Gap

`gap_` cells are subtracted from `available` upfront (`gap_ * (n-1)` total). The per-pair gap array `gaps[]` starts as `[gap_, gap_, ...]`. `SpaceBetween`/`SpaceAround` add to these values rather than replacing them.

---

## 9. Widget Lifecycle

**Source:** `src/core/app.cpp`, `include/Strata/core/widget.hpp`

```
App::run()
  mount_all(root_):
      DFS (pre-order): on_mount() called on every widget

  ... event loop ...

  unmount_all(root_):
      DFS (post-order): on_unmount() called on every widget
```

**`on_mount()`** — called once when the widget tree is first set up. Default implementation is empty. Override to set up timers, subscriptions, or one-time initialization.

**`on_unmount()`** — called once when the app exits. Default implementation is empty.

**`on_focus()` / `on_blur()`** — called by `FocusManager` when a widget gains or loses keyboard focus. Default implementation calls `mark_dirty()` to trigger a visual update (e.g., to draw focus highlight). Override to perform additional actions.

### `for_each_child(visitor)`

This is the traversal primitive. Any framework code that needs to walk the widget tree uses it instead of `dynamic_cast` or RTTI:

```cpp
// Default in Widget: does nothing (leaf widget)
virtual void for_each_child(const std::function<void(Widget&)>&) {}

// Container overrides:
void Container::for_each_child(const std::function<void(Widget&)>& v) {
    for (auto& child : children_) v(*child);
}

// Block overrides:
void Block::for_each_child(const std::function<void(Widget&)>& v) {
    if (inner_) v(*inner_);
}
```

`App::mount_all`, `App::unmount_all`, `FocusManager::collect`, and `App::find_focusable_at` all use this pattern.

### `friend class` Declarations

`Widget` has private members (`parent_`, `dirty_`, `focused_`, `last_rect_`) that need to be accessed by the framework but not by user code. Rather than making them `protected` (which would expose them to user-defined widget subclasses), `Widget` grants `friend` access to specific classes:

```cpp
class Widget {
    friend class App;
    friend class Container;
    friend class FocusManager;
    friend class Block;
    friend class ScrollView;
    // ...
};
```

User-defined widgets inherit from `Widget` but cannot access `parent_`, `dirty_`, or `focused_` directly. They use `mark_dirty()` (the protected method) to signal changes, and `is_focused()` (the public const method) to query focus state.

---

## 10. Event Routing

**Source:** `src/core/app.cpp` — `App::route_event()`

```cpp
void App::route_event(const Event& e) {
    // 1. Tab / Shift-Tab are global — never reach any widget
    if (is_key(e, '\t'))       { focus_->focus_next(); return; }
    if (is_key(e, KEY_BTAB))   { focus_->focus_prev(); return; }

    // 2. Mouse press: hit-test → focus → deliver
    if (const auto* me = as_mouse(e)) {
        if (me->kind == MouseEvent::Kind::Press) {
            Widget* target = find_focusable_at(root_.get(), me->x, me->y);
            if (target) {
                focus_->focus(target);
                target->handle_event(e);
                return;
            }
        }
    }

    // 3. All other events: focused widget → bubble up parent chain
    Widget* target = focus_->focused();
    while (target) {
        if (target->handle_event(e)) return;   // consumed
        target = target->parent();
    }

    // 4. Global fallback
    if (on_event) on_event(e);
}
```

**Bubbling** — if the focused widget's `handle_event` returns `false`, the event is delivered to `widget->parent()`, then its parent, and so on until someone returns `true` or the root is reached.

`Container::handle_event()` always returns `false` — containers are transparent to events. The default `Widget::handle_event()` also returns `false`. Only leaf widgets that actually process events return `true`.

**Mouse hit-testing** uses the `last_rect_` stored during the previous `render_frame()`. It traverses the entire widget tree and returns the **deepest** focusable widget whose `last_rect_` contains the click coordinates:

```cpp
Widget* App::find_focusable_at(Widget* w, int x, int y) {
    Widget* best = nullptr;
    if (w->last_rect_.contains(x, y) && w->is_focusable())
        best = w;
    w->for_each_child([&](Widget& child) {
        Widget* found = find_focusable_at(&child, x, y);
        if (found) best = found;   // deepest wins
    });
    return best;
}
```

---

## 11. FocusManager

**Source:** `src/core/focus_manager.hpp/cpp`

```cpp
class FocusManager {
    std::vector<Widget*> tab_order_;
    int current_idx_ = -1;
    // ...
};
```

**`rebuild(root)`** — called once at startup after `mount_all()`. Performs a DFS using `for_each_child()`, collecting every widget where `is_focusable()` returns `true`. Then does a **stable sort** by `tab_index`:

```
collect all focusable widgets (DFS order)
stable_sort by widget->tab_index
focus widget at index 0 (if any)
```

Stable sort preserves DFS order among widgets with the same `tab_index`. This means:
- Widgets with `tab_index = -1` come before `tab_index = 0`
- Among `tab_index = 0` widgets, order follows tree structure (top-to-bottom, left-to-right DFS)

**`focus_next()` / `focus_prev()`** — advance `current_idx_` with wrapping, then call `apply_focus()`.

**`apply_focus(idx)`:**
```
1. Call old_widget->on_blur()    → marks old widget dirty (triggers visual update)
2. Set old_widget->focused_ = false
3. Set new_widget->focused_ = true
4. current_idx_ = idx
5. Call new_widget->on_focus()   → marks new widget dirty
```

`on_focus()` and `on_blur()` call `mark_dirty()` by default, so focus changes automatically trigger a repaint.

**`focus(Widget* w)`** — finds `w` in `tab_order_` and calls `apply_focus()`. Used by mouse click routing.

---

## 12. Unicode & Wide-Character Handling

**Source:** `src/render/canvas.cpp`

### UTF-8 Decoding

`Canvas::draw_text()` manually decodes UTF-8 byte sequences into `char32_t` codepoints:

```
1-byte:  0xxxxxxx                          → 7-bit ASCII
2-byte:  110xxxxx 10xxxxxx                 → 11-bit
3-byte:  1110xxxx 10xxxxxx 10xxxxxx        → 16-bit  (CJK range)
4-byte:  11110xxx 10xxxxxx 10xxxxxx 10xxxxxx → 21-bit (emoji range)
```

### Display Width with `wcwidth()`

```cpp
#include <cwchar>
int w = wcwidth(codepoint);
```

`wcwidth()` returns:
- `2` for full-width characters (CJK ideographs, many emoji)
- `1` for normal characters
- `0` for combining marks (accent characters, zero-width joiners)
- `-1` for control characters

The canvas code:
1. Skips codepoints where `wcwidth() <= 0` (combining marks, controls)
2. For `wcwidth() == 2`, draws the character and advances x by 2, filling the second column with a space cell to prevent display corruption
3. Clamps to canvas width — a wide character that would straddle the right edge is skipped entirely

### ncurses `mvadd_wch`

`NcursesBackend::render_cell(x, y, cell)` uses `mvadd_wch(y, x, &wch)` (row-first, column-second — ncurses convention). It constructs a `cchar_t` with the `char32_t` and the computed ncurses attribute bitmask.

---

## 13. The DSL Layer — `ui.hpp`

**Source:** `include/Strata/ui.hpp`

The DSL is pure header-only and adds no compiled code. It sits on top of the existing widget API without modifying it.

### Type Erasure via `shared_ptr<Concept>`

`Node` erases the concrete descriptor type using a `shared_ptr` to an abstract interface:

```cpp
class Node {
    struct Concept {
        virtual std::unique_ptr<strata::Widget> build() const = 0;
        virtual strata::Constraint constraint() const = 0;
        virtual ~Concept() = default;
    };
    template<typename T>
    struct Model : Concept {
        T val;
        explicit Model(T v) : val(std::move(v)) {}
        std::unique_ptr<strata::Widget> build() const override { return val.build(); }
        strata::Constraint constraint() const override { return val.constraint(); }
    };
    std::shared_ptr<Concept> impl_;
public:
    template<typename T, enable_if_t<!is_same_v<decay_t<T>, Node>, int> = 0>
    Node(T t) : impl_(make_shared<Model<decay_t<T>>>(move(t))) {}
    // ...
};
```

**Why `shared_ptr` instead of `unique_ptr`?** `std::initializer_list<Node>` provides `const Node&` references to its elements. When those references are copied into `std::vector<Node>` (inside `Col`/`Row`/`ScrollView`), the copy constructor is called. `unique_ptr` is not copyable; `shared_ptr` is. The actual `Concept` object is never shared between nodes — each `Node` is independently constructed from its own descriptor — so this is safe despite using `shared_ptr`.

### Build Happens at `populate()` Time

Descriptors are lightweight value objects. The widget tree is materialized only when `build()` is called:

```
populate(app, {
    Col{                        ← Col descriptor created (holds vector<Node>)
        Button{"OK"},           ← Button descriptor created, wrapped in Node
        Label{"hello"},         ← Label descriptor created, wrapped in Node
    }.size(fixed(3))
})

→ populate() calls Node.build() on each top-level node
→ Col::build() calls make_unique<Container>(), then child.build() for each child
→ Button::build() calls make_unique<strata::Button>(), applies style, sets on_click
→ Container::add(child_widget, child_constraint) for each child
→ widgets are owned by the widget tree; descriptors go out of scope and are destroyed
```

### Method Chaining on Temporaries

All descriptor methods return `T&` (reference to self). Chained calls like:

```cpp
Button{"OK"}.style(s).focused_style(fs).click(fn).size(fixed(3))
```

work because the temporary `Button` object lives until the end of the full expression (the containing `initializer_list` construction). By the time `Node(Button&)` is called, the temporary is still alive, and `Node`'s constructor copies or moves the descriptor into `Model<Button>`.

### The `bind()` Pattern — Deferred Pointer Capture

Descriptors that support `bind(W*& ref)` store `mutable W** ref_`:

```cpp
// In descriptor:
mutable strata::Button** ref_ = nullptr;
Button& bind(strata::Button*& ref) { ref_ = &ref; return *this; }

// In build():
if (ref_) *ref_ = w.get();
```

`build()` is `const` because it is called through `const Concept&` in `Node::build()`. The `mutable` qualifier allows `ref_` to be dereferenced in a `const` function.

User code:
```cpp
strata::ProgressBar* pb = nullptr;          // declare before populate()
populate(app, {
    ProgressBar{}.bind(pb).size(fixed(1)),  // build() sets pb = widget.get()
    Button{"+"}.click([&]{ pb->set_value(...); }),  // lambda captures &pb
});
// After populate() returns: pb != nullptr
```

The lambda captures `pb` **by reference** (the variable, not its value). The lambda is stored inside the `strata::Button` widget. By the time any click event fires, `populate()` has completed and `pb` points to the real widget.

### Sentinel Check for `focused_*_style`

`strata::Block::set_focused_border_style()` has a side-effect: it sets the private `has_focused_styles_ = true`, which switches the block to use focused styles when the inner widget is focused. If it is never called, `has_focused_styles_` stays `false` and the block always uses the normal style.

The `Block` descriptor checks whether the user supplied a non-default style before calling the setter:

```cpp
static const strata::Style kDefault{};
if (focused_border_style_ != kDefault) w->set_focused_border_style(focused_border_style_);
if (focused_title_style_  != kDefault) w->set_focused_title_style(focused_title_style_);
```

This preserves the opt-in semantics: blocks without an explicit `.focused_border()` call render identically to the original imperative API where `set_focused_border_style` was never called.

---

## 14. Color & Attribute Mapping

**Source:** `src/render/ncurses_backend.cpp`

### NamedColor → ncurses color index

ncurses defines 8 basic colors (0–7). The standard bright colors (8–15) are achieved with `A_BOLD` on some terminals, or with the extended 256-color initialization on others.

```
NamedColor::Black   → ncurses color 0
NamedColor::Red     → ncurses color 1
...
NamedColor::White   → ncurses color 7
NamedColor::BrightBlack  → color 0 + A_BOLD (or color 8 if can_256_)
...
NamedColor::BrightWhite  → color 7 + A_BOLD (or color 15 if can_256_)
NamedColor::Default → -1 (ncurses "use terminal default")
```

### RgbColor → 256-color cube

When `COLORS >= 256` (terminal supports 256 colors):

```
index = 16 + 36*r6 + 6*g6 + b6
where r6 = round(r / 255.0 * 5)  (0–5 scale)
      g6 = round(g / 255.0 * 5)
      b6 = round(b / 255.0 * 5)
```

The xterm-256 cube starts at color index 16, with 6 levels per channel.

When `COLORS < 256`, the RGB color is quantized to the nearest of the 8 basic colors using Euclidean distance in RGB space.

### Color Pair Cache

ncurses requires a (fg, bg) pair to be registered before use with `init_pair(id, fg, bg)`. `NcursesBackend` caches these:

```cpp
std::map<PairKey, short> pair_cache_;
short next_pair_id_ = 1;

short alloc_pair(short fg, short bg) {
    PairKey key{fg, bg};
    auto it = pair_cache_.find(key);
    if (it != pair_cache_.end()) return it->second;
    short id = next_pair_id_++;
    init_pair(id, fg, bg);
    pair_cache_[key] = id;
    return id;
}
```

A color pair is allocated on first use and reused for all subsequent identical (fg, bg) combinations. The total number of unique pairs is bounded by `COLORS * COLORS`, which on a 256-color terminal is 65,536 — well within ncurses's typical `COLOR_PAIRS` limit of 65,536.

### Attribute Bitmask

`NcursesBackend::render_cell()` combines the color pair with the style booleans into an ncurses `attr_t`:

```
attr = COLOR_PAIR(pair_id)
if style.bold:      attr |= A_BOLD
if style.italic:    attr |= A_ITALIC
if style.underline: attr |= A_UNDERLINE
if style.dim:       attr |= A_DIM
if style.blink:     attr |= A_BLINK
if style.reverse:   attr |= A_REVERSE
```

This is written into a `cchar_t` and sent to the terminal via `mvadd_wch(y, x, &wch)`.
