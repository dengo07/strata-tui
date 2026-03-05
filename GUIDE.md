# Strata — User Guide

Strata is a retained-mode C++17 TUI framework built on ncurses. You describe a widget tree once, call `app.run()`, and the framework handles rendering, input routing, and focus cycling. Widgets own their state; you mutate them and the framework redraws automatically.

---

## Table of Contents

1. [Requirements & Build](#1-requirements--build)
2. [Quick Start](#2-quick-start)
3. [Core Concepts](#3-core-concepts)
4. [The Declarative DSL — `ui.hpp`](#4-the-declarative-dsl--uihpp)
5. [The Imperative API](#5-the-imperative-api)
6. [Layout](#6-layout)
7. [Style & Color](#7-style--color)
8. [Events](#8-events)
9. [Timers & Async](#9-timers--async)
10. [Focus Management](#10-focus-management)
11. [Widget Reference](#11-widget-reference)
12. [Adding a New Widget](#12-adding-a-new-widget)

---

## 1. Requirements & Build

**Dependencies**

| Package | Purpose |
|---|---|
| `libncursesw-dev` | Wide-char ncurses (required for Unicode box-drawing, CJK, emoji) |
| `libpkg-config` | CMake uses it to locate ncursesw/panelw |
| CMake ≥ 3.16 | Build system |
| C++17 compiler | `std::variant`, `std::optional`, structured bindings |

**Build**

```bash
mkdir build && cd build
cmake ..
make
./strata_test       # run the bundled showcase demo
```

**Clean rebuild**

```bash
make -C build clean && make -C build
```

> **Note:** CMake auto-discovers sources with `GLOB_RECURSE`. After adding new `.cpp` files you must re-run `cmake ..` once — `make` alone won't pick them up.

---

## 2. Quick Start

```cpp
#include <Strata/ui.hpp>

int main() {
    using namespace strata::ui;

    App app;

    populate(app, {
        Label{"Hello, Strata!"}.size(fixed(1)),
        Button{"Quit"}
            .click([&]{ app.quit(); })
            .size(fixed(3)),
    });

    app.run();
}
```

That's it. `populate()` builds the widget tree; `app.run()` starts the event loop. Press the button (Tab to focus, Enter/Space to activate) to quit.

---

## 3. Core Concepts

### The Widget Tree

The application owns a vertical root `Container`. Everything you add goes into that root. Containers can nest arbitrarily — `Col`/`Row` are just vertical/horizontal containers.

```
App
└── Container (root, vertical)
    ├── Label
    ├── Container (horizontal Row)
    │   ├── Block
    │   │   └── Container (vertical Col)
    │   │       ├── Button
    │   │       └── Button
    │   └── ScrollView
    │       ├── Label
    │       └── Label
    └── Label
```

### Retained Mode

Widgets own their state. You call setters like `button->set_label("New")` at any time (including from callbacks). The setter calls `mark_dirty()` internally, which propagates up the tree and triggers a redraw on the next frame. You never explicitly repaint.

### The Main Loop

`app.run()` runs at approximately 60 fps (16 ms event poll timeout). Each iteration:
1. Poll for one event (keyboard, mouse, or terminal resize)
2. Route the event through the widget tree
3. Redraw — but **only if the root is dirty** (dirty-flag optimization)

---

## 4. The Declarative DSL — `ui.hpp`

Include `<Strata/ui.hpp>` instead of `<Strata/strata.hpp>` and add `using namespace strata::ui;`. You get a tree-structured builder API where layout, style, and behavior are colocated.

### `populate()`

```cpp
populate(app, {
    node1,
    node2,
    ...
});
```

Builds every descriptor in the list and adds the resulting widgets to the app's root container. Call this once before `app.run()`.

### Container Descriptors

| Descriptor | Direction | Constructor |
|---|---|---|
| `Col{ node, node, ... }` | Vertical | `Col{initializer_list<Node>}` |
| `Row{ node, node, ... }` | Horizontal | `Row{initializer_list<Node>}` |
| `ScrollView{ node, ... }` | Vertical + scroll | `ScrollView{initializer_list<Node>}` |

All container descriptors support:

```cpp
Col{ ... }
    .size(fixed(7))                         // main-axis (height for Col)
    .cross(fixed(40))                       // cross-axis (width for Col)
    .justify(Layout::Justify::SpaceBetween) // justify children along main axis
    .cross_align(Layout::Align::Center)     // align children along cross axis
    .gap(1)                                 // gap between children (cells)
```

Every leaf descriptor also accepts `.cross(Constraint)` to limit its cross-axis size when placed inside a `Row` or `Col`.

`ScrollView` additionally:

```cpp
ScrollView{ ... }
    .tab_index(-1)   // override tab order (-1 = first)
    .bind(sv_ptr)    // capture raw pointer after build
```

### Block Descriptor

`Block` wraps a single inner node in a bordered box:

```cpp
Block{"My Title", Col{
    Label{"Content"},
}}
.border(Style{}.with_fg(color::Cyan))
.title_style(Style{}.with_fg(color::BrightCyan))
.focused_border(Style{}.with_fg(color::BrightWhite).with_bold())
.focused_title(Style{}.with_fg(color::BrightWhite).with_bold())
.size(fill(1))
```

Methods: `.border()`, `.title_style()`, `.focused_border()`, `.focused_title()`, `.styles(border, title, foc_border, foc_title)`, `.inner(node)`, `.size()`, `.bind(ptr)`.

The `focused_border` / `focused_title` styles are applied when the inner widget (or any of its descendants) has keyboard focus.

### Leaf Descriptors

Every leaf descriptor exposes `.size(Constraint)` and `.bind(W*& ptr)`. Additional methods are widget-specific:

```cpp
Label{"text"}
    .style(Style{}.with_fg(color::BrightWhite))
    .wrap(true)
    .size(fixed(1))

Button{"Click me"}
    .style(Style{}.with_fg(color::BrightCyan))
    .focused_style(Style{}.with_fg(color::Black).with_bg(color::BrightCyan).with_bold())
    .click([&]{ /* callback */ })
    .tab_index(0)
    .size(fill(1))

Checkbox{"Enable feature"}
    .checked(true)
    .change([](bool v){ /* callback */ })
    .size(fixed(1))

Switch{"Dark mode"}
    .on(false)
    .change([](bool v){ /* callback */ })
    .size(fixed(1))

Input{}
    .placeholder("type here…")
    .value("initial")
    .submit([](const std::string& s){ /* on Enter */ })
    .change([](const std::string& s){ /* on each keystroke */ })
    .size(fixed(1))

Select{}
    .items({"Option A", "Option B", "Option C"})
    .selected(0)
    .change([](int idx, const std::string& item){ })
    .size(fixed(1))

RadioGroup{}
    .items({"Small", "Medium", "Large"})
    .selected(1)
    .change([](int idx, const std::string& item){ })

ProgressBar{}
    .value(0.5f)         // 0.0–1.0
    .show_percent(true)
    .bind(pb)            // capture for callbacks
    .size(fixed(1))

Spinner{"Loading…"}
    .auto_animate(true)  // auto-advance frames
    .size(fill(1))
```

### The `bind()` Pattern

Use `bind()` when one widget's callback needs to read or mutate another widget:

```cpp
strata::ProgressBar* pb = nullptr;   // declared before populate()

populate(app, {
    ProgressBar{}.value(0.5f).bind(pb).size(fixed(1)),
    Button{"Reset"}.click([&]{ pb->set_value(0.0f); }).size(fixed(3)),
});
// After populate() returns, pb points to the ProgressBar widget.
// The lambda's [&] capture keeps a reference to the pb variable,
// which is valid and non-null by the time any button press fires.
```

`bind(ptr)` stores `&ptr` inside the descriptor. When `build()` runs during `populate()`, it sets `*ref_ = widget.get()`. Every callback fires after `populate()` completes, so `pb` is always set.

### Re-exports in `strata::ui`

`using namespace strata::ui` brings in:

- `App`, `Style`, `Constraint`, `Layout`, `Event`
- `is_char`, `is_key`, `as_key`, `as_mouse`
- `namespace color = strata::color`
- Free functions: `fixed(n)`, `fill(w=1)`, `min(n)`, `max(n)`, `percentage(p)`

---

## 5. The Imperative API

If you prefer direct control, `#include <Strata/strata.hpp>` and call `add<Widget>()` directly:

```cpp
#include <Strata/strata.hpp>
using namespace strata;

App app;

auto* lbl = app.add<Label>(Constraint::fixed(1), "Hello!");
lbl->set_style(Style{}.with_fg(color::BrightWhite));

auto* row = app.add<Container>(Constraint::fixed(3),
    Layout(Layout::Direction::Horizontal));

auto* btn = row->add<Button>(Constraint::fixed(10), "Click");
btn->set_style(Style{}.with_fg(color::BrightCyan));
btn->on_click = [&]{ app.quit(); };

app.run();
```

`add<W>(Constraint, args...)` constructs `W` in place, sets its `parent_`, and returns a raw pointer. The app owns the widget via `unique_ptr`.

**`Block` with imperative API:**

```cpp
auto* block = app.add<Block>(Constraint::fill(1));
block->set_title(" My Panel ");
block->set_border_style(Style{}.with_fg(color::Cyan));
block->set_focused_border_style(Style{}.with_fg(color::BrightWhite).with_bold());

auto* inner = block->set_inner<Container>(
    Layout(Layout::Direction::Vertical));
inner->add<Label>(Constraint::fixed(1), "Inside the block");
```

---

## 6. Layout

### Constraints

Each widget carries two independent constraints: **main-axis** (set via `.size()`) and **cross-axis** (set via `.cross()`).

| | `Col` (vertical) | `Row` (horizontal) |
|---|---|---|
| Main axis | height | width |
| Cross axis | width | height |

| Constraint | Meaning |
|---|---|
| `fixed(n)` | Exactly `n` cells |
| `fill(w=1)` | Proportional share of remaining space (weight `w`) |
| `min(n)` | At least `n` cells; grows into remaining space |
| `max(n)` | Up to `n` cells from the fill pool |
| `percentage(p)` | `p`% of available space (0–100) |

**Fill weights** — two `fill(1)` siblings each get 50 %; `fill(2)` and `fill(1)` split space 2:1.

### Cross-axis size and alignment

By default every child stretches to fill the full cross-axis of its slot. Override this with `.cross()`:

```cpp
// In a Row, limit a column's height to 5 rows and center it vertically
Row{
    Col{ ... }.size(fixed(30)).cross(fixed(5)),
    Col{ ... }.size(fill()),
}.cross_align(Layout::Align::Center)
```

| `.cross(constraint)` | Cross-axis behavior |
|---|---|
| `fill()` (default) | Stretch to fill the full cross-axis slot |
| `fixed(n)` | Exactly `n` cells in the cross direction |
| `max(n)` | At most `n` cells |
| `percentage(p)` | `p`% of the cross-axis slot |

When a child's cross size is less than the full slot, `cross_align` on the container positions it:

```cpp
Col{ ... }.cross_align(Layout::Align::Center)   // center children horizontally
Row{ ... }.cross_align(Layout::Align::End)       // align children to the bottom
```

| `.cross_align(value)` | Position |
|---|---|
| `Align::Start` (default) | Top / left |
| `Align::Center` | Centered |
| `Align::End` | Bottom / right |

### Justify

When children have `fixed`/`min` constraints (no `fill`), the remaining **main-axis** space can be distributed:

```cpp
Row{ ... }.justify(Layout::Justify::SpaceBetween)
```

| Value | Effect |
|---|---|
| `Start` (default) | All children at the start, remaining space at the end |
| `Center` | Children centered, equal margins on both sides |
| `End` | Children at the end |
| `SpaceBetween` | Equal gaps between children, no outer margin |
| `SpaceAround` | Equal gaps between and around children |

> `Justify` has no effect when any child uses `fill()` — fill constraints absorb all remaining space.

### Gap

```cpp
Col{ ... }.gap(1)   // 1-cell gap between every pair of children
```

The gap is subtracted from available space before constraints are resolved.

---

## 7. Style & Color

### `Style`

```cpp
Style s = Style{}
    .with_fg(color::BrightCyan)
    .with_bg(color::Blue)
    .with_bold()
    .with_italic()
    .with_underline()
    .with_dim()
    .with_blink()
    .with_reverse();
```

`Style` fields: `fg`, `bg`, `bold`, `italic`, `underline`, `dim`, `blink`, `reverse`. All default to "terminal default" / `false`.

### Named Colors

```
color::Black       color::BrightBlack
color::Red         color::BrightRed
color::Green       color::BrightGreen
color::Yellow      color::BrightYellow
color::Blue        color::BrightBlue
color::Magenta     color::BrightMagenta
color::Cyan        color::BrightCyan
color::White       color::BrightWhite
color::Default     (terminal default foreground/background)
```

### RGB Colors

```cpp
Style{}.with_fg(color::rgb(255, 128, 0))
```

On terminals with 256-color support (`COLORS >= 256`), RGB values are mapped to the nearest xterm-256 6×6×6 color cube. On 8-color terminals, they quantize to the nearest basic color.

---

## 8. Events

### `Event` Type

```cpp
using Event = std::variant<KeyEvent, MouseEvent, ResizeEvent>;
```

**`KeyEvent`** — any keypress, including special keys:

```cpp
struct KeyEvent {
    int  key;    // ASCII value, or ncurses KEY_* constant
    bool ctrl;
    bool alt;
    bool shift;
};
```

Common key codes (no `#include <ncurses.h>` needed):

| Key | Code |
|---|---|
| Enter | `10` |
| Backspace | `263` |
| Delete | `330` |
| Arrow Up | `259` |
| Arrow Down | `258` |
| Arrow Left | `260` |
| Arrow Right | `261` |
| Home | `262` |
| Page Down | `338` |
| Page Up | `339` |
| Tab | `'\t'` (9) |
| Shift+Tab | handled by App internally |

**`MouseEvent`**:

```cpp
struct MouseEvent {
    int    x, y;
    Button button;   // Left, Right, Middle, ScrollUp, ScrollDown, None
    Kind   kind;     // Press, Release, Move
};
```

**`ResizeEvent`** — fired when the terminal window is resized; the framework handles it automatically (marks the whole tree dirty).

### Helper Functions

```cpp
is_char(e, 'q')       // true if KeyEvent with key == 'q'
is_key(e, 258)        // true if KeyEvent with key == 258 (↓)
as_key(e)             // const KeyEvent* or nullptr
as_mouse(e)           // const MouseEvent* or nullptr
as_resize(e)          // const ResizeEvent* or nullptr
```

### Global Event Handler

```cpp
app.on_event = [&](const Event& e) -> bool {
    if (is_char(e, 'q')) { app.quit(); return true; }
    return false;  // not consumed
};
```

`on_event` is called only if the focused widget and all its ancestors returned `false` from `handle_event`. Return `true` to mark the event consumed, `false` to ignore it (the framework ignores unhandled events regardless).

### Event Routing Order

1. **Tab / Shift-Tab** — intercepted by `App` before any widget; cycles focus
2. **Mouse Press** — hit-tests to find the deepest focusable widget under the cursor, focuses it, sends the event
3. **All other events** — delivered to the focused widget; if `handle_event` returns `false`, the event bubbles up through `parent_` pointers
4. **Fallback** — `app.on_event` if no widget consumed it

---

## 9. Timers & Async

### `set_interval` / `set_timeout`

Callbacks that run on the **main thread** at a fixed schedule. The event loop adjusts its poll timeout so timers fire on time without busy-waiting.

```cpp
// Fires every 1000 ms — good for cheap reads (< a few ms)
int id = app.set_interval(1000, [&]{
    pb_cpu->set_value(read_cpu_usage());   // fast /proc read
    pb_ram->set_value(read_ram_usage());
});

// Fires once after 500 ms
app.set_timeout(500, [&]{
    lbl_status->set_text("Ready.");
});

// Cancel a timer by ID
app.clear_timer(id);
```

> **Rule of thumb:** use `set_interval` only for work that completes in a millisecond or two. Anything slower belongs in `run_async`.

### `run_async`

Runs `bg_fn` on a **background thread**; queues `on_done` to run on the **main thread** once it completes (≤ 16 ms later).

```cpp
// Share results between threads via shared_ptr
auto result = std::make_shared<std::vector<std::string>>();

app.set_interval(5000, [&, result]{
    app.run_async(
        // Background thread — must NOT touch widgets
        [result]{ *result = scan_directory("/some/path"); },
        // Main thread — safe to call any widget setter
        [&, result]{
            for (auto& s : *result)
                sv->add<strata::Label>(Constraint::fixed(1), s, Style{});
        }
    );
});
```

**Rules:**
- `bg_fn` runs on a detached thread — never touch widgets or call `mark_dirty()` from it
- `on_done` runs on the main thread — full widget access is safe
- Both `App` and all captured widget pointers must outlive `on_done`'s execution

---

## 10. Focus Management


### Tab Order

Every widget has a public `int tab_index = 0` field. FocusManager collects all focusable widgets via DFS then **stable-sorts** them by `tab_index`. Lower values come first; widgets with the same `tab_index` keep their DFS (tree) order.

```cpp
// DSL
Button{"First"}.tab_index(-1)   // will be focused before tab_index=0 widgets
Button{"Last"} .tab_index(99)

// Imperative
btn->tab_index = -1;
scroll_view->tab_index = -1;    // keyboard-scroll views often go first
```

**Tab** cycles to the next focusable widget; **Shift-Tab** cycles backward. Focus wraps around.

### Which Widgets Are Focusable

| Widget | Focusable |
|---|---|
| Button | Yes |
| Checkbox | Yes |
| Switch | Yes |
| Input | Yes |
| Select | Yes |
| RadioGroup | Yes |
| ScrollView | Yes |
| Label | No |
| ProgressBar | No |
| Spinner | No |
| Container | No |
| Block | No (delegates to inner) |

---

## 11. Widget Reference

### `Label`

Non-interactive text display. Supports optional word-wrap.

```cpp
Label{"Hello world"}
    .style(Style{}.with_fg(color::BrightWhite).with_bold())
    .wrap(true)
    .size(fill(1))
```

Imperative: `label->set_text("…")`, `label->set_style(…)`, `label->set_wrap(bool)`.

---

### `Button`

Focusable. Renders as `[ Label ]`. Space or Enter fires `on_click`.

```cpp
Button{"OK"}
    .style(Style{}.with_fg(color::BrightGreen))
    .focused_style(Style{}.with_fg(color::Black).with_bg(color::BrightGreen).with_bold())
    .click([&]{ /* ... */ })
    .size(fill(1))
```

Imperative: `btn->set_label("…")`, `btn->set_style(…)`, `btn->set_focused_style(…)`, `btn->on_click = [&]{};`.

---

### `Checkbox`

Focusable. Renders as `[x] Label` / `[ ] Label`. Space or Enter toggles.

```cpp
Checkbox{"Enable notifications"}
    .checked(true)
    .change([](bool checked){ /* ... */ })
    .size(fixed(1))
```

Imperative: `cb->set_checked(bool)`, `cb->is_checked()`, `cb->on_change = [](bool){};`.

---

### `Switch`

Focusable. Renders as `Label ─── [ ON ]` or `Label ─── [OFF ]`. Space or Enter toggles.

```cpp
Switch{"Wi-Fi"}
    .on(true)
    .change([](bool on){ /* ... */ })
    .size(fixed(1))
```

Imperative: `sw->set_on(bool)`, `sw->is_on()`, `sw->on_change = [](bool){};`.

---

### `Input`

Focusable. Single-line editable text field.

**Keys:** arrows move cursor, Home/End jump, Backspace/Delete edit, Ctrl+U clears, Enter fires `on_submit`.

```cpp
Input{}
    .placeholder("search…")
    .value("initial text")
    .submit([](const std::string& s){ /* on Enter */ })
    .change([](const std::string& s){ /* every keystroke */ })
    .size(fixed(1))
```

Imperative: `inp->set_placeholder("…")`, `inp->set_value("…")`, `inp->value()`, `inp->on_submit`, `inp->on_change`.

---

### `Select`

Focusable. Single-row cycling selector: `◀ Item ▶` (focused) / `  Item  ` (unfocused).

**Keys:** ←/h/k go backward; →/l/j go forward (wraps). ↑/↓ are reserved for group focus navigation.

```cpp
Select{}
    .items({"Light", "Dark", "Solarized"})
    .selected(0)
    .change([](int idx, const std::string& item){ })
    .size(fixed(1))
```

Imperative: `sel->set_items({…})`, `sel->set_selected(int)`, `sel->selected()`, `sel->selected_item()`, `sel->on_change`.

---

### `RadioGroup`

Focusable. Vertical list: `(●) Selected` / `(○) Other`. Auto-scrolls when list is taller than the available area.

**Keys:** Down/j and Up/k navigate; Space/Enter selects.

```cpp
RadioGroup{}
    .items({"Small", "Medium", "Large"})
    .selected(1)
    .change([](int idx, const std::string& item){ })
    .size(fill(1))
```

Imperative: `rg->add_item("…")`, `rg->set_items({…})`, `rg->set_selected(int)`, `rg->selected()`, `rg->on_change`.

---

### `ProgressBar`

Non-focusable. Horizontal bar: `████░░░░ 60%`.

```cpp
ProgressBar{}
    .value(0.5f)          // 0.0–1.0
    .show_percent(true)
    .bind(pb)
    .size(fixed(1))

// Later, from a callback:
pb->set_value(std::clamp(pb->value() + 0.1f, 0.0f, 1.0f));
```

Imperative: `pb->set_value(float)`, `pb->set_value(int current, int total)`, `pb->set_show_percent(bool)`, `pb->value()`.

---

### `Spinner`

Non-focusable. Animated braille spinner with optional label.

```cpp
Spinner{"Loading…"}
    .auto_animate(true)          // auto-advance every 6 render calls (~10 fps)
    .auto_animate(true, 3)       // faster: every 3 render calls
    .size(fill(1))
```

Imperative: `sp->set_label("…")`, `sp->tick()` (advance one frame), `sp->set_auto_animate(bool, int)`.

With `auto_animate` enabled, the spinner sets an internal flag that keeps the render loop running continuously — even without user input.

---

### `ScrollView`

Focusable. Vertical scrolling container. Same `add<W>()` interface as `Container`.

**Keys:** j/↓ scroll down 1 line; k/↑ scroll up 1 line; PgDn scroll down `height-1`; PgUp scroll up `height-1`.

Auto-scrolls to keep the focused descendant visible during render.

A scrollbar (`░`/`█`) is drawn automatically in the rightmost column whenever the content is taller than the visible area. It disappears when all content fits. The thumb brightens when the ScrollView has focus.

```cpp
ScrollView{
    Label{"Item 1"}.size(fixed(1)),
    Label{"Item 2"}.size(fixed(1)),
    // ...
}.tab_index(-1)
 .size(fill(1))
```

Imperative: `sv->add<W>(Constraint, args...)`, `sv->scroll_to(int y)`, `sv->scroll_by(int delta)`, `sv->scroll_y()`.

---

### `Block`

Non-focusable (transparent). Bordered box wrapping exactly one inner widget. Focused styles apply when the inner widget or any of its descendants has focus.

```cpp
Block{"Title", inner_node}
    .border(Style{}.with_fg(color::Cyan))
    .title_style(Style{}.with_fg(color::BrightCyan))
    .focused_border(Style{}.with_fg(color::BrightWhite).with_bold())
    .focused_title(Style{}.with_fg(color::BrightWhite).with_bold())
    .size(fill(1))
```

> **Known limitation:** `Block`'s focus detection uses `inner_->is_focused() || inner_->has_focused_descendant()`. This works correctly for all cases — including `Container` inners with multiple focusable children.

---

## 12. Adding a New Widget

1. Create `include/Strata/widgets/my_widget.hpp` — inherit from `strata::Widget`
2. Implement:
   - `void render(Canvas& canvas) override` — draw at relative coords
   - `bool handle_event(const Event& e) override` — return `true` to consume
   - `bool is_focusable() const override` — return `true` if Tab-navigable
   - `void for_each_child(const std::function<void(Widget&)>&) override` — if it wraps child widgets
3. Create `src/widgets/my_widget.cpp` with the implementation
4. Include it in `include/Strata/strata.hpp`
5. Re-run `cmake ..` once (GLOB_RECURSE won't pick up new files automatically)

**Calling `mark_dirty()`:** Call it from any setter that changes visual state. It propagates up through `parent_` pointers and causes the entire tree to re-render on the next frame.

**Continuous animation:** Set `Widget::s_needs_rerender_ = true` at the end of `render()` to keep the loop running frame-by-frame (used by `Spinner::render()`).

```cpp
// Minimal widget example
class MyWidget : public strata::Widget {
    std::string text_;
public:
    MyWidget& set_text(std::string t) { text_ = std::move(t); mark_dirty(); return *this; }
    void render(strata::Canvas& canvas) override {
        canvas.draw_text(0, 0, text_, {});
    }
};
```
