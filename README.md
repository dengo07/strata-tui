# Strata TUI Framework — Documentation

Strata is a retained-mode C++17 TUI (terminal UI) framework built on ncurses. Widgets own their state, declare a constraint-based layout, and re-render automatically whenever state changes. The ncurses backend is hidden behind a pure-virtual `Backend` interface, making it straightforward to swap for testing or for future backend implementations.

---

## Table of Contents

1. [Getting Started](#1-getting-started)
2. [Tutorial](#2-tutorial)
3. [Core Concepts](#3-core-concepts)
4. [DSL Reference (ui.hpp)](#4-dsl-reference-uihpp)
5. [Widget Reference](#5-widget-reference)
6. [App API Reference](#6-app-api-reference)
7. [Adding a Custom Widget](#7-adding-a-custom-widget)
8. [Patterns & Recipes](#8-patterns--recipes)

---

## 1. Getting Started

### Dependencies

| Dependency | Package (Debian/Ubuntu) |
|---|---|
| ncursesw (wide-char ncurses) | `libncursesw-dev` |
| pkg-config | `pkg-config` |
| CMake ≥ 3.16 | `cmake` |
| C++17 compiler | `g++` or `clang++` |

```bash
sudo apt install libncursesw-dev pkg-config cmake g++
```

### Build

```bash
git clone <repo>
cd strata
mkdir build && cd build
cmake ..
make
./strata_test   # run the built-in demo
```

CMake discovers all source files via `GLOB_RECURSE`. After adding a new `.cpp` file, re-run `cmake ..` from the build directory.

### Install system-wide (like ncurses)

```bash
git clone https://github.com/dengo07/strata-tui
cd strata-tui
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
make -j$(nproc)
sudo make install
```

This installs:
- `libstrata.a` → `/usr/local/lib/`
- Headers → `/usr/local/include/Strata/`
- CMake package → `/usr/local/lib/cmake/Strata/`
- pkg-config file → `/usr/local/lib/pkgconfig/strata.pc`

### Linking your own project

**Option A — CMake (recommended)**

```cmake
cmake_minimum_required(VERSION 3.16)
project(MyApp LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

find_package(Strata REQUIRED)

add_executable(myapp main.cpp)
target_link_libraries(myapp PRIVATE Strata::strata)
```

**Option B — pkg-config**

```bash
g++ -std=c++17 main.cpp -o myapp $(pkg-config --cflags --libs strata)
```

**Option C — embed as subdirectory (no install needed)**

```cmake
add_subdirectory(path/to/strata-tui)
target_link_libraries(myapp PRIVATE strata)
```

### Two include modes

```cpp
// Low-level (full widget API, manual tree construction)
#include <Strata/strata.hpp>

// DSL layer (declarative, builder-pattern — recommended for most apps)
#include <Strata/ui.hpp>
using namespace strata::ui;
```

`ui.hpp` includes everything from `strata.hpp` plus the declarative descriptor types.

---

## 2. Tutorial

### 2a. Hello World

A centered greeting box — showcases `Block`, nested `Col`, and justify/cross-axis centering in one small example.

```cpp
#include <Strata/ui.hpp>
using namespace strata::ui;

int main() {
    App app;

    populate(app, {
        Col({
            Block(" Hello World ")
                .border(Style{}.with_fg(color::Cyan).with_bold())
                .inner(
                    Col({
                        Label("Hello, Strata!")
                            .style(Style{}.with_fg(color::BrightWhite).with_bold())
                            .size(fixed(1)),
                        Label("").size(fixed(1)),
                        Label("press  q  to quit")
                            .style(Style{}.with_fg(color::BrightBlack))
                            .size(fixed(1)),
                    }).justify(Layout::Justify::Center)
                      .cross_align(Layout::Align::Center)
                )
                .size(fixed(7))
                .cross(fixed(36)),
        }).justify(Layout::Justify::Center)
          .cross_align(Layout::Align::Center)
    });

    app.on_event = [&](const Event& e) {
        if (is_char(e, 'q')) app.quit();
    };

    app.run();
}
```

**What's happening:**
- `App` initialises the ncurses backend and creates a root `Container`.
- `populate()` adds each top-level `Node` to that root container.
- `app.run()` blocks in an event loop until `app.quit()` is called.
- The outer `Col` with `justify(Center)` + `cross_align(Center)` centers the box vertically and horizontally on screen.
- `Block.size(fixed(7)).cross(fixed(36))` pins the box to 7 rows × 36 columns regardless of terminal size.
- `on_event` is called for every event not consumed by a focused widget — use it for global hotkeys.

### 2b. Counter

Two bordered buttons that increment/decrement an integer, displayed in a styled panel — centered on screen.

```cpp
#include <Strata/ui.hpp>
using namespace strata::ui;

int main() {
    App app;
    int count = 0;
    strata::Label* lbl = nullptr;

    populate(app, {
        Block("Counter App")
            .border(Style{}.with_bg(color::Black))
            .inner(
                Col({
                    Block()
                        .border(Style{}.with_fg(color::Red).with_bg(color::Black))
                        .inner(
                            Col({
                                Label("0")
                                    .style(Style{}.with_fg(color::White).with_bold())
                                    .size(fixed(1))
                                    .cross(fixed(4))
                                    .bind(lbl)
                            }).justify(Layout::Justify::Center)
                              .cross_align(Layout::Align::Center)
                        )
                        .size(fixed(10))
                        .cross(fixed(50)),
                    Row({
                        Button("-")
                            .style(Style{}.with_bg(color::BrightRed).with_fg(color::White).with_bold())
                            .focused_style(Style{}.with_bg(color::BrightRed).with_fg(color::rgb(0,0,0)).with_bold())
                            .click([&]{ lbl->set_text(std::to_string(--count)); })
                            .size(fixed(10))
                            .cross(fixed(4)),
                        Button("+")
                            .style(Style{}.with_bg(color::BrightGreen).with_fg(color::Black).with_bold())
                            .focused_style(Style{}.with_bg(color::BrightGreen).with_fg(color::rgb(0,0,0)).with_bold())
                            .click([&]{ lbl->set_text(std::to_string(++count)); })
                            .size(fixed(10))
                            .cross(fixed(4)),
                    }).justify(Layout::Justify::Center)
                      .gap(1)
                      .size(fixed(4)),
                }).justify(Layout::Justify::Center)
                  .cross_align(Layout::Align::Center)
            )
    });

    app.on_event = [&](const Event& e) {
        if (is_char(e, 'q')) app.quit();
    };

    app.run();
}
```

**Key ideas:**
- `.bind(lbl)` stores the raw pointer to the created widget so you can call setters later.
- `.click(fn)` sets the `on_click` callback on the `Button`.
- `set_text()` calls `mark_dirty()` internally, scheduling a redraw.
- `Row.size(fixed(4))` fixes the button row height so `justify(Center)` on the outer `Col` has no fill child and can distribute vertical space evenly.
- Buttons use `size(fixed(10)).cross(fixed(4))` — width 10, height 4. At `body_h = 3` (unfocused) the bordered rendering tier activates automatically.
- `cross_align(Center)` on the outer `Col` keeps the display block (50 wide) horizontally centered regardless of terminal width.

### 2c. To-Do List

An `Input` for adding items, `Checkbox` slots for toggling completion, a `Select` filter, and a modal confirmation for deletion — all in a scrollable panel.

```cpp
#include <Strata/ui.hpp>
using namespace strata::ui;
#include <vector>
#include <string>
#include <algorithm>

static const int MAX_ITEMS = 64;

int main() {
    App app;

    struct Item { std::string text; bool done = false; };
    std::vector<Item> items;
    std::vector<int>  view;   // filtered indices into items
    int cursor = 0;           // last-interacted slot position in view
    int confirm_id = -1;

    strata::Label*  stats_lbl    = nullptr;
    strata::Input*  input_widget = nullptr;
    strata::Select* filter_sel   = nullptr;

    std::vector<strata::Checkbox*> slots(MAX_ITEMS, nullptr);
    std::vector<Node> slot_nodes;
    slot_nodes.reserve(MAX_ITEMS);
    for (int i = 0; i < MAX_ITEMS; ++i) {
        slot_nodes.push_back(
            Checkbox("")
                .size(fixed(1))
                .group("list")
                .style(Style{}.with_fg(color::White))
                .focused_style(Style{}.with_fg(color::Black).with_bg(color::Cyan))
                .bind(slots[i])
                .change([&, i](bool checked){
                    cursor = i;
                    if (i < (int)view.size())
                        items[view[i]].done = checked;
                    int done = (int)std::count_if(items.begin(), items.end(),
                                    [](const Item& it){ return it.done; });
                    if (stats_lbl)
                        stats_lbl->set_text(std::to_string(done) + "/" +
                                            std::to_string(items.size()) + " done");
                })
        );
    }

    auto refresh = [&]{
        int mode = filter_sel ? filter_sel->selected() : 0;
        view.clear();
        for (int i = 0; i < (int)items.size(); ++i) {
            if (mode == 0) view.push_back(i);
            else if (mode == 1 && !items[i].done) view.push_back(i);
            else if (mode == 2 &&  items[i].done) view.push_back(i);
        }
        for (int s = 0; s < MAX_ITEMS; ++s) {
            if (!slots[s]) continue;
            if (s < (int)view.size()) {
                slots[s]->set_label(" " + items[view[s]].text);
                slots[s]->set_checked(items[view[s]].done);
            } else {
                slots[s]->set_label("");
                slots[s]->set_checked(false);
            }
        }
        int done = (int)std::count_if(items.begin(), items.end(),
                        [](const Item& it){ return it.done; });
        if (stats_lbl)
            stats_lbl->set_text(std::to_string(done) + "/" +
                                std::to_string(items.size()) + " done");
    };

    populate(app, {
        Block(" To-Do List ")
            .border(Style{}.with_fg(color::Cyan).with_bold())
            .inner(
                Col({
                    Row({
                        Label("  Add: ").style(Style{}.with_fg(color::BrightBlack)).size(fixed(7)),
                        Input()
                            .placeholder("type and press Enter…")
                            .group("input")
                            .size(fill())
                            .bind(input_widget)
                            .submit([&](const std::string& val){
                                if (!val.empty() && (int)items.size() < MAX_ITEMS) {
                                    items.push_back({val, false});
                                    input_widget->set_value("");
                                    refresh();
                                }
                            }),
                    }).size(fixed(1)),
                    Row({
                        Label(" Filter: ").style(Style{}.with_fg(color::BrightBlack)).size(fixed(9)),
                        Select()
                            .items({"All", "Active", "Done"})
                            .group("filter")
                            .size(fill())
                            .bind(filter_sel)
                            .change([&](int, const std::string&){ refresh(); }),
                        Label("").style(Style{}.with_fg(color::BrightBlack))
                            .size(fixed(10)).bind(stats_lbl),
                    }).size(fixed(1)),
                    Label("").size(fixed(1)),
                    ScrollView(std::move(slot_nodes)).group("list"),
                    Label("  Tab: switch section  ·  d: delete item")
                        .style(Style{}.with_fg(color::BrightBlack))
                        .size(fixed(1)),
                })
            )
    });

    refresh();

    app.on_event = [&](const Event& e) {
        if (app.has_modal()) return;
        if (is_char(e, 'q')) app.quit();
        if (is_char(e, 'd') && cursor < (int)view.size()) {
            const std::string item_text = items[view[cursor]].text;
            confirm_id = app.open_modal(
                ModalDesc()
                    .title(" Delete Item ")
                    .size(44, 8)
                    .border(Style{}.with_fg(color::Red))
                    .on_close([&]{ app.close_modal(confirm_id); })
                    .inner(
                        Col({
                            Label("  Delete \"" + item_text + "\"?")
                                .style(Style{}.with_fg(color::White))
                                .size(fixed(1)),
                            Label("").size(fixed(1)),
                            Row({
                                Button("Yes")
                                    .style(Style{}.with_bg(color::Red).with_fg(color::White).with_bold())
                                    .focused_style(Style{}.with_bg(color::BrightRed).with_fg(color::White).with_bold())
                                    .click([&]{
                                        app.close_modal(confirm_id);
                                        if (cursor < (int)view.size()) {
                                            items.erase(items.begin() + view[cursor]);
                                            if (cursor > 0) --cursor;
                                            refresh();
                                        }
                                    })
                                    .size(fixed(10)).cross(fixed(4)),
                                Button("No")
                                    .style(Style{}.with_bg(color::BrightBlack).with_fg(color::White))
                                    .focused_style(Style{}.with_bg(color::White).with_fg(color::Black))
                                    .click([&]{ app.close_modal(confirm_id); })
                                    .size(fixed(10)).cross(fixed(4)),
                            }).gap(2).justify(Layout::Justify::Center).size(fixed(4)),
                        }).justify(Layout::Justify::Center)
                    )
                    .build_modal()
            );
        }
    };

    app.run();
}
```

**Key ideas:**
- **Checkbox slots** replace Labels — Space/Enter toggles the item done/undone; the `change` callback also records `cursor` (last-interacted slot position in the view). `.style()` / `.focused_style()` control the label text and row background for unfocused and focused states respectively.
- **`view[]` + filter**: `Select` drives a mode (All/Active/Done); `refresh()` rebuilds `view` and re-syncs all slots on every change.
- **Stats label** is a plain `Label*` updated by `refresh()` — no special widget needed, just a bound pointer and `set_text()`.
- **Modal confirmation** for delete: press `d` to open a two-button dialog; Tab cycles between Yes/No; Escape closes without deleting. See [Modal with confirmation](#modal-with-confirmation).
- **`app.has_modal()`** guard in `on_event` prevents global hotkeys from firing while the modal is open.

### 2d. Dashboard with Async Updates

Three live metrics (CPU / Memory / Network) with color-coded status labels, a `Spinner` for visual feedback, and fluctuating simulated values — all updated via `run_async`.

```cpp
#include <Strata/ui.hpp>
using namespace strata::ui;
#include <memory>
#include <array>
#include <cmath>

int main() {
    App app;
    int tick = 0;

    strata::ProgressBar* cpu_bar  = nullptr;
    strata::ProgressBar* mem_bar  = nullptr;
    strata::ProgressBar* net_bar  = nullptr;
    strata::Label*       cpu_st   = nullptr;
    strata::Label*       mem_st   = nullptr;
    strata::Label*       net_st   = nullptr;
    strata::Label*       info_lbl = nullptr;

    auto threshold_style = [](float v) -> Style {
        if (v < 0.6f) return Style{}.with_fg(color::BrightGreen);
        if (v < 0.8f) return Style{}.with_fg(color::BrightYellow);
        return Style{}.with_fg(color::BrightRed).with_bold();
    };
    auto threshold_text = [](float v) -> std::string {
        if (v < 0.6f) return "  OK ";
        if (v < 0.8f) return "WARN ";
        return "CRIT!";
    };

    populate(app, {
        Col({
            Block(" System Monitor ")
                .border(Style{}.with_fg(color::Blue).with_bold())
                .inner(
                    Col({
                        Row({
                            Label("CPU    ").style(Style{}.with_fg(color::BrightWhite).with_bold()).size(fixed(8)),
                            ProgressBar().size(fill()).bind(cpu_bar),
                            Label("  OK ").style(Style{}.with_fg(color::BrightGreen)).size(fixed(6)).bind(cpu_st),
                        }).size(fixed(1)),
                        Label("").size(fixed(1)),
                        Row({
                            Label("Memory ").style(Style{}.with_fg(color::BrightWhite).with_bold()).size(fixed(8)),
                            ProgressBar().size(fill()).bind(mem_bar),
                            Label("  OK ").style(Style{}.with_fg(color::BrightGreen)).size(fixed(6)).bind(mem_st),
                        }).size(fixed(1)),
                        Label("").size(fixed(1)),
                        Row({
                            Label("Network").style(Style{}.with_fg(color::BrightWhite).with_bold()).size(fixed(8)),
                            ProgressBar().size(fill()).bind(net_bar),
                            Label("  OK ").style(Style{}.with_fg(color::BrightGreen)).size(fixed(6)).bind(net_st),
                        }).size(fixed(1)),
                        Label("").size(fixed(1)),
                        Row({
                            Spinner("").auto_animate(true, 4).size(fixed(3)),
                            Label("Waiting for first sample…")
                                .style(Style{}.with_fg(color::BrightBlack))
                                .size(fill())
                                .bind(info_lbl),
                        }).size(fixed(1)),
                    })
                )
                .size(fixed(13))
                .cross(fixed(56)),
        }).justify(Layout::Justify::Center)
          .cross_align(Layout::Align::Center)
    });

    app.set_interval(1000, [&]{
        ++tick;
        auto vals = std::make_shared<std::array<float, 3>>();
        app.run_async(
            // Background thread — simulate a slow measurement; must NOT touch widgets
            [vals, tick]{
                (*vals)[0] = 0.30f + 0.55f * std::abs(std::sin(tick * 0.7f));
                (*vals)[1] = 0.45f + 0.40f * std::abs(std::sin(tick * 0.4f));
                (*vals)[2] = 0.05f + 0.70f * std::abs(std::sin(tick * 1.2f));
            },
            // Main thread — safe to call widget setters
            [&, vals]{
                float c = (*vals)[0], m = (*vals)[1], n = (*vals)[2];
                if (cpu_bar) cpu_bar->set_value(c);
                if (mem_bar) mem_bar->set_value(m);
                if (net_bar) net_bar->set_value(n);
                if (cpu_st) { cpu_st->set_text(threshold_text(c)); cpu_st->set_style(threshold_style(c)); }
                if (mem_st) { mem_st->set_text(threshold_text(m)); mem_st->set_style(threshold_style(m)); }
                if (net_st) { net_st->set_text(threshold_text(n)); net_st->set_style(threshold_style(n)); }
                if (info_lbl) info_lbl->set_text("  Updated " + std::to_string(tick) + "×");
            }
        );
    });

    app.on_event = [&](const Event& e) {
        if (is_char(e, 'q')) app.quit();
    };

    app.run();
}
```

**Thread safety rules:**
- `bg_fn` runs on a detached thread. Never touch any widget inside it.
- `on_done` is queued and runs on the main thread ≤ 16 ms after `bg_fn` returns.
- Pass data through a `shared_ptr<T>`: bg thread writes, on_done reads.

**Key ideas:**
- **Multiple bound pointers**: three `ProgressBar*` and three status `Label*` — all updated from the same `on_done` callback, which runs on the main thread.
- **`set_style()` for live color**: each status label's color changes every update to reflect the current threshold — `set_style()` calls `mark_dirty()` internally.
- **Fluctuating simulation**: values use `std::sin(tick * speed)` so bars move realistically. Replace with real `/proc/stat` reads for production.
- **Spinner with `auto_animate(true)`**: drives itself every N render calls — no extra timer needed for the animation.
- **`run_async` + `shared_ptr`**: the background "measurement" and the main-thread UI update are always separated; the `shared_ptr<array>` is the only safe handoff channel.

---

## 3. Core Concepts

### 3a. The Widget Tree

```
App
└── root Container  (Vertical layout, fills terminal)
    ├── Widget A    (Constraint::fill)
    ├── Widget B    (Constraint::fixed(3))
    └── Container C (Horizontal layout)
        ├── Widget D
        └── Widget E
```

- `App` owns the root `Container` and the `Backend`.
- Widgets are added to a `Container` (or `App`) via `add<W>(...)` or `populate()`.
- Each widget knows its `parent_`; `mark_dirty()` propagates up to the root, signalling `App::render_frame()` to redraw.
- **Lifecycle**: `on_mount()` fires after the widget is attached to the tree (before the first render). `on_unmount()` fires before removal.

### 3b. Retained-Mode Rendering

Strata is **retained-mode**: each widget stores its own state and renders itself into a `Canvas` on demand. You update state via setters (e.g. `set_text()`), which internally call `mark_dirty()`. The event loop redraws the whole frame only when the root is dirty.

The `NcursesBackend` is double-buffered: `draw_cell()` writes to a back buffer; `flush()` diffs the back buffer against the front buffer and only emits escape sequences for cells that changed. This keeps terminal output minimal even for large screens.

### 3c. Layout System

#### Constraint types

Each child carries **two** independent constraints: **main-axis** (`.size()`) and **cross-axis** (`.cross()`).

| | `Col` | `Row` |
|---|---|---|
| Main axis | height | width |
| Cross axis | width | height |

| Constructor | Meaning |
|---|---|
| `fixed(n)` | Exactly `n` cells |
| `min(n)` | At least `n` cells; grows into remaining space |
| `max(n)` | At most `n` cells; fills available space up to cap |
| `percentage(p)` | `p`% of the available space |
| `fill(w=1)` | Proportional fill; weight `w` divides remaining space |

#### Two-pass algorithm (main axis)

1. **Pass 1**: resolve `Fixed`, `Percentage`, `Min` constraints and sum the allocated space. `Min` widgets also participate in fill distribution from their floor.
2. **Pass 2**: distribute remaining space proportionally among `Fill`, `Max`, and `Min` widgets by weight.

#### Cross-axis sizing and alignment

By default every child stretches to fill the full cross-axis slot (`fill()`). Set `.cross(constraint)` to limit it:

```cpp
// Row: a column that is exactly 5 rows tall, centered vertically
Row{
    Col{ ... }.size(fill()).cross(fixed(5)),
}.cross_align(Layout::Align::Center)
```

| `.cross(constraint)` | Cross-axis behavior |
|---|---|
| `fill()` (default) | Stretch to fill the full slot |
| `fixed(n)` | Exactly `n` cells in the cross direction |
| `max(n)` | At most `n` cells |
| `percentage(p)` | `p`% of the cross-axis slot |

When a child's cross size is less than the full slot, position it with `cross_align` on the container:

| `cross_align(value)` | Position |
|---|---|
| `Align::Start` (default) | Top / left |
| `Align::Center` | Centered |
| `Align::End` | Bottom / right |

#### Layout direction

```cpp
Layout(Layout::Direction::Vertical)    // children stacked top-to-bottom
Layout(Layout::Direction::Horizontal)  // children placed left-to-right
```

`Col{...}` in the DSL is Vertical; `Row{...}` is Horizontal.

#### Justify (main-axis alignment)

When **no** `fill` constraints are present, the leftover main-axis space is distributed according to `Justify`:

| Value | Effect |
|---|---|
| `Start` (default) | All children packed at the start |
| `Center` | Children centred, equal margins |
| `End` | Children packed at the end |
| `SpaceBetween` | Equal gaps between children, none at edges |
| `SpaceAround` | Equal gaps around all children |

```cpp
Col({...}).justify(Layout::Justify::SpaceBetween)
```

#### Gap

```cpp
Col({...}).gap(1)   // 1-cell gap between every child
```

#### Canvas coordinates

- `Canvas::draw_*` uses **relative** coordinates — (0,0) is the top-left of the current canvas.
- `Canvas::sub_canvas(Rect)` takes an **absolute** `Rect` (as returned by `Layout::split()`).

### 3d. Event System

```cpp
using Event = std::variant<KeyEvent, MouseEvent, ResizeEvent>;
```

#### Helper functions

```cpp
is_key(e, 10)          // true if key == 10 (Enter)
is_char(e, 'q')        // true if key == 'q'
as_key(e)              // const KeyEvent* or nullptr
as_mouse(e)            // const MouseEvent* or nullptr
as_resize(e)           // const ResizeEvent* or nullptr
```

#### Key code reference

| Key | Code |
|---|---|
| Enter | 10 |
| Backspace | 263 |
| Delete | 330 |
| Arrow Down | 258 |
| Arrow Up | 259 |
| Arrow Left | 260 |
| Arrow Right | 261 |
| Home | 262 |
| Page Down | 338 |
| Page Up | 339 |
| Tab | 9 |
| Escape | 27 |

#### Event routing

1. Delivered first to the **focused widget** via `handle_event()`.
2. If the widget returns `false` (not consumed), the event **bubbles up** through `parent_` pointers.
3. Unhandled Tab/Shift-Tab → focus group navigation.
4. Unhandled ↓/j → `focus_next_local()`; ↑/k → `focus_prev_local()`.
5. Any remaining unhandled event reaches `App::on_event`.

When a modal is open, events are delivered only to modal widgets.

### 3e. Focus & Focus Groups

#### Tab order

Set `tab_index` on any focusable widget. `FocusManager` stable-sorts all focusable widgets by `tab_index` after a DFS collect; lower values receive focus earlier.

```cpp
Button("First").tab_index(0)
Button("Second").tab_index(1)
```

#### Focus groups

Group related widgets together so Tab jumps between groups rather than individual widgets:

```cpp
Input().group("header")
Button("OK").group("actions")
Button("Cancel").group("actions")
```

- **Tab** → move focus to the first widget in the **next** group.
- **Shift-Tab** → move focus to the first widget in the **previous** group.
- **↓ / j** (unhandled) → next focusable widget **within** the current group.
- **↑ / k** (unhandled) → previous focusable widget **within** the current group.
- Widgets with no group act as their own single-widget group for Tab purposes.
- If all widgets share the same group, Tab does nothing (no other group to jump to).

### 3f. Styling

```cpp
Style s = Style{}
    .with_fg(color::Green)
    .with_bg(color::Black)
    .with_bold()
    .with_italic()
    .with_underline()
    .with_dim()
    .with_blink()
    .with_reverse();
```

#### Named colors (`strata::color::`)

`Black`, `Red`, `Green`, `Yellow`, `Blue`, `Magenta`, `Cyan`, `White`,
`BrightBlack`, `BrightRed`, `BrightGreen`, `BrightYellow`, `BrightBlue`, `BrightMagenta`, `BrightCyan`, `BrightWhite`,
`Default` (terminal default)

#### RGB colors

```cpp
color::rgb(255, 128, 0)   // orange
```

On terminals with fewer than 256 colors, RGB is quantized to the nearest entry in the 6×6×6 color cube.

---

## 4. DSL Reference (ui.hpp)

Include and open the namespace:

```cpp
#include <Strata/ui.hpp>
using namespace strata::ui;
```

All descriptor types produce a `Node` that is consumed by `populate()` or nested inside `Col`/`Row`/`Block`/`ScrollView`.

### `populate()`

```cpp
void populate(App& app, std::initializer_list<Node> nodes);
```

Adds all top-level nodes to the App's root container.

### Constraint helpers

```cpp
fixed(int n)         // Constraint::fixed(n)
fill(int w = 1)      // Constraint::fill(w)
min(int n)           // Constraint::min(n)
max(int n)           // Constraint::max(n)
percentage(int p)    // Constraint::percentage(p)
```

### `Col` — Vertical container

```cpp
Col({ child, child, ... })
    .size(Constraint)                    // main-axis (height); default: fill()
    .cross(Constraint)                   // cross-axis (width); default: fill() = stretch
    .justify(Layout::Justify::Start)     // main-axis child alignment when no fill
    .cross_align(Layout::Align::Start)   // cross-axis child alignment when child < full
    .gap(int)                            // gap between children
```

### `Row` — Horizontal container

```cpp
Row({ child, child, ... })
    .size(Constraint)                    // main-axis (width); default: fill()
    .cross(Constraint)                   // cross-axis (height); default: fill() = stretch
    .justify(Layout::Justify::Start)
    .cross_align(Layout::Align::Start)   // Start | Center | End
    .gap(int)
```

Every leaf descriptor (`Label`, `Button`, `Input`, etc.) also accepts `.cross(Constraint)` to override its cross-axis size when placed inside a container.

### `Block` — Bordered box

```cpp
Block("Title")                           // optional title string
    .inner(Node)                         // single inner widget
    .size(Constraint)
    .border(Style)                       // border style (unfocused)
    .title_style(Style)                  // title style (unfocused)
    .focused_border(Style)               // border style when inner has focus
    .focused_title(Style)                // title style when inner has focus
    .styles(border, title, foc_border, foc_title)  // set all four at once
    .bind(strata::Block*& ref)
```

> **Note**: focused border highlighting works only when the direct inner widget is the focusable widget. If the inner widget is a Container with multiple focusable children, the border won't change on focus.

### `Label` — Static text

```cpp
Label("text")
    .style(Style)
    .wrap(bool)          // word-wrap (default false)
    .size(Constraint)
    .bind(strata::Label*& ref)
```

### `Button` — Clickable button

```cpp
Button("label")
    .style(Style)
    .focused_style(Style)
    .shadow(Style)                       // explicit shadow style (optional)
    .size(Constraint)
    .click(std::function<void()>)
    .tab_index(int)
    .group(std::string)
    .bind(strata::Button*& ref)
```

**Rendering tiers** (chosen automatically based on available space):

| Condition | Style |
|---|---|
| `body_h ≥ 3` and `width ≥ 4` | Rounded border (`╭─╮ │ ╰─╯`) with label centered inside |
| Otherwise | Solid filled rectangle with label centered |

`body_h` is `height − 1` when unfocused (shadow row reserved) and `height` when focused.
Use `fixed(4)` or taller for the bordered style when unfocused; `fixed(3)` is sufficient when focused (no shadow row).

The shadow row uses `▀` (half-block) characters — top half painted in the button's background color, bottom half in the shadow color — giving a seamless "raised button" effect. The shadow row disappears while the button has focus.

### `Checkbox` — Toggle checkbox

```cpp
Checkbox("label")
    .checked(bool)                       // initial state
    .size(Constraint)
    .change(std::function<void(bool)>)   // called with new state
    .tab_index(int)
    .group(std::string)
    .bind(strata::Checkbox*& ref)
```

### `Switch` — Toggle switch

```cpp
Switch("label")
    .on(bool)                            // initial state
    .size(Constraint)
    .change(std::function<void(bool)>)
    .tab_index(int)
    .group(std::string)
    .bind(strata::Switch*& ref)
```

### `Input` — Single-line text field

```cpp
Input()
    .placeholder(std::string)
    .value(std::string)                  // initial value
    .size(Constraint)
    .submit(std::function<void(const std::string&)>)   // on Enter
    .change(std::function<void(const std::string&)>)   // on each keystroke
    .tab_index(int)
    .group(std::string)
    .bind(strata::Input*& ref)
```

### `Select` — Cycling selector

```cpp
Select()
    .items(std::vector<std::string>)
    .selected(int)                       // initial index
    .size(Constraint)
    .change(std::function<void(int, const std::string&)>)
    .tab_index(int)
    .group(std::string)
    .bind(strata::Select*& ref)
```

### `RadioGroup` — Vertical radio list

```cpp
RadioGroup()
    .items(std::vector<std::string>)
    .selected(int)
    .size(Constraint)
    .change(std::function<void(int, const std::string&)>)
    .tab_index(int)
    .group(std::string)
    .bind(strata::RadioGroup*& ref)
```

### `ProgressBar` — Horizontal fill bar

```cpp
ProgressBar()
    .value(float)                        // 0.0–1.0
    .show_percent(bool)                  // default true
    .size(Constraint)
    .bind(strata::ProgressBar*& ref)
```

### `Spinner` — Animated braille spinner

```cpp
Spinner("label")
    .auto_animate(bool enable, int every = 6)   // auto-advance every N frames
    .size(Constraint)
    .bind(strata::Spinner*& ref)
```

### `ScrollView` — Scrollable container

```cpp
ScrollView({ child, child, ... })
    .size(Constraint)
    .tab_index(int)
    .group(std::string)
    .bind(strata::ScrollView*& ref)
```

### `ModalDesc` — Modal dialog descriptor

Modals are opened imperatively; the descriptor builds the `Modal` object:

```cpp
int modal_id = app.open_modal(
    ModalDesc()
        .title("Confirm")
        .size(40, 10)                    // width, height
        .border(Style)
        .title_style(Style)
        .overlay(Style)                  // dimming overlay style
        .on_close(std::function<void()>) // called on Escape
        .inner(Node)                     // inner content
        .bind(strata::Modal*& ref)
        .build_modal()
);

// Close later:
app.close_modal(modal_id);
```

---

## 5. Widget Reference

### Label

Read-only text display. Non-focusable.

```cpp
Label(std::string text = "", Style style = {})
```

| Method | Description |
|---|---|
| `set_text(string)` | Update displayed text; calls `mark_dirty()` |
| `set_style(Style)` | Update text style |
| `set_wrap(bool)` | Enable/disable word-wrap |
| `text()` | Returns current text |
| `style()` | Returns current style |

### Block

Bordered box wrapping one inner widget.

```cpp
Block()
```

| Method | Description |
|---|---|
| `set_title(string)` | Set title text in the top border |
| `set_border_style(Style)` | Unfocused border style |
| `set_title_style(Style)` | Unfocused title style |
| `set_focused_border_style(Style)` | Border style when inner widget has focus |
| `set_focused_title_style(Style)` | Title style when inner widget has focus |
| `set_inner<W>(args...)` | Construct inner widget in-place; returns `W*` |
| `set_inner(unique_ptr<Widget>)` | Take ownership of existing widget |

### Button

Clickable, focusable widget. Automatically selects a rendering tier based on available height and width (see [Button DSL](#button--clickable-button) for the tier thresholds).

```cpp
Button(std::string label = "Button")
```

| Public member | Type | Description |
|---|---|---|
| `on_click` | `function<void()>` | Called on Enter/Space when focused |

| Method | Description |
|---|---|
| `set_label(string)` | Update button text |
| `set_style(Style)` | Unfocused style |
| `set_focused_style(Style)` | Style when focused |
| `set_shadow_style(Style)` | Override the auto-derived shadow color; sets the `bg` of the `▀` shadow row |

| Key | Action |
|---|---|
| Enter / Space | Fire `on_click` |

### ProgressBar

Horizontal fill bar. Non-focusable.

```cpp
ProgressBar()
```

| Method | Description |
|---|---|
| `set_value(float v)` | Set fill ratio in `[0.0, 1.0]` |
| `set_value(int current, int total)` | Convenience: `current / total` |
| `set_show_percent(bool)` | Show/hide percentage text |
| `value()` | Returns current float value |

### Checkbox

Single toggle. Focusable.

```cpp
Checkbox(std::string label = "", bool checked = false)
```

| Public member | Type | Description |
|---|---|---|
| `on_change` | `function<void(bool)>` | Called with new state after toggle |

| Method | Description |
|---|---|
| `set_label(string)` | Update label text |
| `set_checked(bool)` | Set state programmatically |
| `is_checked()` | Returns current state |

| Key | Action |
|---|---|
| Space / Enter | Toggle |

### RadioGroup

Vertical list of radio buttons. Focusable.

```cpp
RadioGroup()
```

| Public member | Type | Description |
|---|---|---|
| `on_change` | `function<void(int, const string&)>` | Called with new index and item text |

| Method | Description |
|---|---|
| `add_item(string)` | Append an option |
| `set_items(vector<string>)` | Replace all options |
| `set_selected(int)` | Select by index |
| `selected()` | Returns current index |
| `selected_item()` | Returns current item text |

| Key | Action |
|---|---|
| j / ↓ | Move selection down |
| k / ↑ | Move selection up |
| Space / Enter | Confirm selection |

### TextBox

Read-only multiline text viewer. Focusable.

```cpp
TextBox()
```

| Method | Description |
|---|---|
| `set_text(string)` | Replace all content (splits on `\n`) |
| `set_wrap(bool)` | Enable word-wrap |
| `append(string)` | Append a line and scroll to bottom |
| `clear()` | Remove all content |

| Key | Action |
|---|---|
| j / ↓ | Scroll down 1 line |
| k / ↑ | Scroll up 1 line |

### Input

Single-line editable text field. Focusable.

```cpp
Input()
```

| Public member | Type | Description |
|---|---|---|
| `on_submit` | `function<void(const string&)>` | Called on Enter |
| `on_change` | `function<void(const string&)>` | Called on every character change |

| Method | Description |
|---|---|
| `set_placeholder(string)` | Dimmed hint shown when empty |
| `set_value(string)` | Set content programmatically |
| `value()` | Returns current text |

| Key | Action |
|---|---|
| Printable chars | Insert at cursor |
| ← / → | Move cursor |
| Home | Jump to start |
| Backspace | Delete before cursor |
| Delete | Delete at cursor |
| Ctrl+U | Clear entire field |
| Enter | Fire `on_submit` |

### Select

Single-row cycling selector. Focusable. Renders as `◀ Item ▶` when focused.

```cpp
Select()
```

| Public member | Type | Description |
|---|---|---|
| `on_change` | `function<void(int, const string&)>` | Called on each item change |

| Method | Description |
|---|---|
| `set_items(vector<string>)` | Replace item list |
| `set_selected(int)` | Select by index |
| `selected()` | Returns current index |
| `selected_item()` | Returns current item text |

| Key | Action |
|---|---|
| ← / h | Previous item (wraps) |
| → / l | Next item (wraps) |

### Switch

Toggle with an ON/OFF indicator. Focusable. Renders as `Label ─── [ ON ]` or `Label ─── [OFF ]`.

```cpp
Switch(std::string label = "", bool on = false)
```

| Public member | Type | Description |
|---|---|---|
| `on_change` | `function<void(bool)>` | Called with new state |

| Method | Description |
|---|---|
| `set_label(string)` | Update label text |
| `set_on(bool)` | Set state programmatically |
| `is_on()` | Returns current state |

| Key | Action |
|---|---|
| Space / Enter | Toggle |

### Spinner

Animated braille spinner. Non-focusable. Renders as `⠋ label`, cycling through 10 frames.

```cpp
Spinner(std::string label = "")
```

| Method | Description |
|---|---|
| `set_label(string)` | Update accompanying text |
| `tick()` | Advance one frame and mark dirty |
| `set_auto_animate(bool, int every = 6)` | Auto-advance every N render calls |

With `auto_animate` enabled the spinner drives itself; no timer needed.

### ScrollView

Vertically scrollable container. Focusable.

```cpp
ScrollView(Layout layout = Layout(Layout::Direction::Vertical))
```

| Method | Description |
|---|---|
| `add(unique_ptr<Widget>, Constraint)` | Add a widget |
| `add<W>(Constraint, args...)` | Construct in-place with constraint |
| `add<W>(args...)` | Construct in-place, constraint from `flex` |
| `scroll_to(int y)` | Set scroll offset absolutely |
| `scroll_by(int delta)` | Adjust scroll offset by delta |
| `scroll_y()` | Returns current scroll offset |

| Key | Action |
|---|---|
| j / ↓ | Scroll down 1 line |
| k / ↑ | Scroll up 1 line |
| PgDn | Scroll down (visible height − 1) |
| PgUp | Scroll up (visible height − 1) |

The `ScrollView` auto-scrolls to keep the currently focused descendant visible on every render pass.

### Modal

A centered dialog with a full-screen dimming overlay. Not part of the widget tree; managed by `App`.

```cpp
Modal()
```

| Method | Description |
|---|---|
| `set_title(string)` | Dialog title |
| `set_size(int w, int h)` | Preferred width and height |
| `set_border_style(Style)` | Dialog border style |
| `set_title_style(Style)` | Title text style |
| `set_overlay_style(Style)` | Background overlay style |
| `set_on_close(function<void()>)` | Called when Escape is pressed |
| `set_inner(unique_ptr<Widget>)` | Content widget |
| `set_inner<W>(args...)` | Construct content in-place |
| `inner()` | Returns raw pointer to inner widget |

Focus is trapped inside the modal while it is open. Press Escape to trigger `on_close_`.

---

## 6. App API Reference

```cpp
#include <Strata/strata.hpp>
// or
#include <Strata/ui.hpp>
```

### Constructor

```cpp
explicit App(std::unique_ptr<Backend> backend = nullptr);
```

Pass `nullptr` (default) to use the built-in `NcursesBackend`. Pass a custom `Backend` subclass for testing or alternative outputs.

### Adding widgets

```cpp
// Construct in-place, Constraint::fill() by default
template<typename W, typename... Args>
W* add(Args&&... args);

// Construct in-place with explicit constraint
template<typename W, typename... Args>
W* add(Constraint c, Args&&... args);

// Transfer ownership of an existing widget
Widget* add(std::unique_ptr<Widget> w, Constraint c = Constraint::fill());
```

### Run / quit

```cpp
void run();    // blocks until quit() is called
void quit();   // signals the event loop to exit cleanly
```

### Global event handler

```cpp
std::function<void(const Event&)> on_event;
```

Called after the focused widget's bubble-up chain if no widget consumed the event. Use for global hotkeys.

### Timer API

```cpp
// Call fn on the main thread every ms milliseconds. Returns timer ID.
int set_interval(int ms, std::function<void()> fn);

// Call fn on the main thread once after ms milliseconds. Returns timer ID.
int set_timeout(int ms, std::function<void()> fn);

// Cancel a timer by ID (safe to call with an already-fired or invalid ID).
void clear_timer(int id);
```

Timers fire on the main thread — widget setters are safe to call directly.

### Async API

```cpp
void run_async(std::function<void()> bg_fn,
               std::function<void()> on_done = {});
```

- `bg_fn` runs on a **detached background thread**. Never access widgets or Strata state from it.
- `on_done` is queued to the main thread and executes ≤ 16 ms after `bg_fn` returns.
- Pass data between the two using `shared_ptr<T>`: bg writes, on_done reads.
- The `App` and all captured widget pointers must remain alive until `on_done` executes.

### Alert / toast notifications

```cpp
// Show a toast in the bottom-right corner. Returns alert ID.
// duration_ms = 0 → persistent until dismiss_alert() is called.
int notify(std::string title,
           std::string message = "",
           AlertManager::Level level = AlertManager::Level::Info,
           int duration_ms = 4000);

// Remove an alert by ID.
void dismiss_alert(int id);
```

`AlertManager::Level` values: `Info`, `Success`, `Warning`, `Error`.

### Modal system

```cpp
// Open a modal overlay. Returns modal ID.
int  open_modal(std::unique_ptr<Modal> m);

// Close a modal by ID, restoring background focus.
void close_modal(int id);

// Returns true if at least one modal is currently open.
bool has_modal() const;
```

---

## 7. Adding a Custom Widget

### Step 1 — Create the header

`include/Strata/widgets/my_widget.hpp`:

```cpp
#pragma once
#include <Strata/core/widget.hpp>
#include <Strata/style/style.hpp>
#include <string>

namespace strata {

class MyWidget : public Widget {
    std::string text_;
    Style       style_;

public:
    explicit MyWidget(std::string text = "", Style style = {})
        : text_(std::move(text)), style_(style) {}

    MyWidget& set_text(std::string t) { text_ = std::move(t); mark_dirty(); return *this; }

    // Required — draw into the canvas using relative coordinates
    void render(Canvas& canvas) override {
        canvas.draw_text(0, 0, text_, style_);
    }

    // Optional — return true if this widget can receive Tab focus
    bool is_focusable() const override { return false; }

    // Optional — return true to consume the event (stop bubbling)
    bool handle_event(const Event& e) override { (void)e; return false; }

    // Required if this widget wraps children — iterate for DFS traversal
    // void for_each_child(const std::function<void(Widget&)>& v) override { v(*child_); }
};

} // namespace strata
```

### Step 2 — Create the implementation

`src/widgets/my_widget.cpp`:

```cpp
#include <Strata/widgets/my_widget.hpp>
// Additional implementation if needed
```

Even if the `.cpp` is empty it must exist so CMake's `GLOB_RECURSE` picks it up.

### Step 3 — Register in the master include

Add to `include/Strata/strata.hpp`:

```cpp
#include "widgets/my_widget.hpp"
```

### Step 4 — Re-run CMake

```bash
cd build && cmake .. && make
```

`GLOB_RECURSE` does not auto-detect new `.cpp` files — `cmake ..` is required after every addition.

### Canvas API quick reference

```cpp
canvas.draw_text(x, y, "text", style);   // relative coords
canvas.draw_cell(x, y, cell);
canvas.fill(U' ', style);                // flood-fill entire area
canvas.draw_border(style);               // single-line Unicode border
canvas.draw_title("Title", style);       // title into top border
canvas.width();                          // canvas width
canvas.height();                         // canvas height

// Create a sub-canvas for a child widget (takes ABSOLUTE rect)
Canvas child_canvas = canvas.sub_canvas(absolute_rect);
```

---

## 8. Patterns & Recipes

### Dynamic list (retained-mode)

Strata does not support adding/removing widgets at runtime. Use the pre-allocated slot pattern:

```cpp
static const int MAX = 100;
std::vector<strata::Label*> slots(MAX, nullptr);
std::vector<int> view;   // indices into your data model

// Build slots once
std::vector<Node> nodes;
for (int i = 0; i < MAX; ++i)
    nodes.push_back(Label("").size(fixed(1)).bind(slots[i]));

// Refresh after data changes
auto refresh = [&]{
    for (int i = 0; i < MAX; ++i) {
        if (!slots[i]) continue;
        if (i < (int)view.size())
            slots[i]->set_text(data[view[i]].name);
        else
            slots[i]->set_text("");
    }
};
```

Guard all per-slot callbacks with `if (s < view.size())` to avoid stale index access.

### Modal with confirmation

```cpp
int confirm_id = -1;

auto open_confirm = [&]{
    confirm_id = app.open_modal(
        ModalDesc()
            .title("Confirm Delete")
            .size(40, 8)
            .on_close([&]{ app.close_modal(confirm_id); })
            .inner(
                Col({
                    Label("Are you sure?").size(fixed(1)),
                    Row({
                        Button("Yes")
                            .click([&]{
                                do_delete();
                                app.close_modal(confirm_id);
                            }),
                        Button("No")
                            .click([&]{ app.close_modal(confirm_id); }),
                    }).size(fixed(4))   // fixed(4) → bordered style (body_h=3 unfocused)
                })
            )
            .build_modal()
    );
};
```

Tab cycles between "Yes" and "No" while the modal is open. Escape fires `on_close`.

### Background data fetch

```cpp
app.set_interval(5000, [&]{
    auto result = std::make_shared<std::string>();
    app.run_async(
        // bg thread
        [result]{ *result = fetch_from_network(); },
        // main thread
        [&, result]{ label->set_text(*result); }
    );
});
```

### CPU/resource polling with async measurement

Combine `set_interval` for scheduling with `run_async` for non-blocking measurement:

```cpp
app.set_interval(1000, [&]{
    auto value = std::make_shared<float>(0.f);
    app.run_async(
        [value]{ *value = read_cpu_usage(); },   // blocks in bg
        [&, value]{ pb->set_value(*value); }      // updates on main thread
    );
});
```

### Alert / notification on completion

```cpp
app.run_async(
    []{ do_long_work(); },
    [&]{ app.notify("Done", "Task completed successfully",
                    AlertManager::Level::Success, 3000); }
);
```

### Preventing double-handling with an open modal

Check `app.has_modal()` before acting on global events:

```cpp
app.on_event = [&](const Event& e) {
    if (app.has_modal()) return;   // let modal handle everything
    if (is_char(e, 'q')) app.quit();
    if (is_char(e, 'd')) open_confirm();
};
```

### Deferred close (dismiss after timeout)

```cpp
int alert_id = app.notify("Saving…", "", AlertManager::Level::Info, 0);
app.run_async(
    []{ save_file(); },
    [&, alert_id]{
        app.dismiss_alert(alert_id);
        app.notify("Saved", "", AlertManager::Level::Success, 2000);
    }
);
```
