#include "ncurses_backend.hpp"
#include <ncurses.h>
#include <panel.h>
#include <locale.h>
#include <cstring>
#include <algorithm>
#include <climits>

namespace strata {

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

NcursesBackend::~NcursesBackend() {
    if (initialized_) shutdown();
}

void NcursesBackend::init() {
    setlocale(LC_ALL, "");   // Must be before initscr() for Unicode box-drawing chars

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    set_escdelay(25);        // Reduce ESC ambiguity delay (default is 1000ms)
    curs_set(0);             // Hide cursor

    if (has_colors()) {
        start_color();
        use_default_colors(); // Allows -1 as "terminal default" color
        can_256_ = (COLORS >= 256);
    }

    // Enable mouse events
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, nullptr);

    initialized_ = true;

    // Initialize double buffers; front_buf_ filled with sentinel so first frame draws all
    resize_bufs(width(), height());
}

void NcursesBackend::shutdown() {
    if (!initialized_) return;
    mousemask(0, nullptr);
    endwin();
    initialized_ = false;
}

// ---------------------------------------------------------------------------
// Terminal dimensions
// ---------------------------------------------------------------------------

int NcursesBackend::width()  const { return getmaxx(stdscr); }
int NcursesBackend::height() const { return getmaxy(stdscr); }

// ---------------------------------------------------------------------------
// Color management
// ---------------------------------------------------------------------------

short NcursesBackend::resolve_color(const Color& color) const {
    return std::visit([this](const auto& c) -> short {
        using T = std::decay_t<decltype(c)>;
        if constexpr (std::is_same_v<T, NamedColor>) {
            if (c.is_default()) return -1; // terminal default
            return static_cast<short>(c.base_index());
        } else {
            // RgbColor
            if (can_256_) {
                // Map to xterm-256 6x6x6 color cube: index = 16 + 36*r6 + 6*g6 + b6
                // Per-channel: (val < 48) ? 0 : min(5, (val-55)/40 + 1)
                auto cube_idx = [](uint8_t val) -> short {
                    if (val < 48) return 0;
                    return static_cast<short>(std::min(5, (static_cast<int>(val) - 55) / 40 + 1));
                };
                short r6 = cube_idx(c.r);
                short g6 = cube_idx(c.g);
                short b6 = cube_idx(c.b);
                return static_cast<short>(16 + 36 * r6 + 6 * g6 + b6);
            } else {
                // Quantize to nearest of 8 basic colors
                static const struct { uint8_t r, g, b; } basics[8] = {
                    {0,   0,   0  }, // Black
                    {170, 0,   0  }, // Red
                    {0,   170, 0  }, // Green
                    {170, 170, 0  }, // Yellow
                    {0,   0,   170}, // Blue
                    {170, 0,   170}, // Magenta
                    {0,   170, 170}, // Cyan
                    {170, 170, 170}, // White
                };
                int best = 0, best_dist = INT_MAX;
                for (int i = 0; i < 8; ++i) {
                    int dr = c.r - basics[i].r;
                    int dg = c.g - basics[i].g;
                    int db = c.b - basics[i].b;
                    int dist = dr*dr + dg*dg + db*db;
                    if (dist < best_dist) { best_dist = dist; best = i; }
                }
                return static_cast<short>(best);
            }
        }
    }, color);
}

short NcursesBackend::alloc_pair(short fg, short bg) {
    PairKey key{fg, bg};
    auto it = pair_cache_.find(key);
    if (it != pair_cache_.end()) return it->second;

    if (next_pair_id_ >= COLOR_PAIRS) {
        // Degraded: return pair 0 (terminal default)
        return 0;
    }
    short id = next_pair_id_++;
    init_pair(id, fg, bg);
    pair_cache_[key] = id;
    return id;
}

// ---------------------------------------------------------------------------
// Double-buffer management
// ---------------------------------------------------------------------------

void NcursesBackend::resize_bufs(int w, int h) {
    int sz = w * h;
    back_buf_.assign(sz, Cell{U' ', Style{}});
    // Sentinel value (U'\0') ensures first frame redraws all cells
    front_buf_.assign(sz, Cell{U'\0', Style{}});
}

// ---------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------

void NcursesBackend::render_cell(int x, int y, const Cell& cell) {
    // Direct ncurses draw — called only by flush() for changed cells
    short fg = resolve_color(cell.style.fg);
    short bg = resolve_color(cell.style.bg);
    short pair_id = (has_colors()) ? alloc_pair(fg, bg) : 0;

    attr_t attrs = COLOR_PAIR(pair_id);
    if (cell.style.bold)      attrs |= A_BOLD;
    if (cell.style.italic)    attrs |= A_ITALIC;
    if (cell.style.underline) attrs |= A_UNDERLINE;
    if (cell.style.dim)       attrs |= A_DIM;
    if (cell.style.blink)     attrs |= A_BLINK;
    if (cell.style.reverse)   attrs |= A_REVERSE;

    // Handle bright colors via A_BOLD
    if (const auto* nc = std::get_if<NamedColor>(&cell.style.fg)) {
        if (nc->is_bright()) attrs |= A_BOLD;
    }

    wchar_t wch[2] = { static_cast<wchar_t>(cell.ch), L'\0' };
    cchar_t cc;
    setcchar(&cc, wch, attrs, pair_id, nullptr);
    mvadd_wch(y, x, &cc);  // ncurses: row first, then col
}

void NcursesBackend::draw_cell(int x, int y, const Cell& cell) {
    int w = width(), h = height();
    if (x < 0 || y < 0 || x >= w || y >= h) return;
    int idx = y * w + x;
    if (idx < static_cast<int>(back_buf_.size()))
        back_buf_[idx] = cell;
}

void NcursesBackend::clear() {
    // Reset back_buf_ to blank; no erase() call needed (double-buffered)
    back_buf_.assign(back_buf_.size(), Cell{U' ', Style{}});
}

void NcursesBackend::flush() {
    int w = width(), h = height();
    int sz = w * h;

    // Resize buffers if terminal dimensions changed
    if (sz != static_cast<int>(back_buf_.size())) {
        resize_bufs(w, h);
        return; // next frame will draw everything
    }

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int idx = y * w + x;
            if (back_buf_[idx] != front_buf_[idx]) {
                render_cell(x, y, back_buf_[idx]);
            }
        }
    }
    front_buf_ = back_buf_;
    refresh();
}

// ---------------------------------------------------------------------------
// Mouse event translation
// ---------------------------------------------------------------------------

MouseEvent NcursesBackend::translate_mouse(void* mevent_ptr) {
    MEVENT* me = static_cast<MEVENT*>(mevent_ptr);
    MouseEvent evt;
    evt.x = me->x;
    evt.y = me->y;

    if (me->bstate & BUTTON1_PRESSED)        { evt.button = MouseEvent::Button::Left;   evt.kind = MouseEvent::Kind::Press; }
    else if (me->bstate & BUTTON1_RELEASED)  { evt.button = MouseEvent::Button::Left;   evt.kind = MouseEvent::Kind::Release; }
    else if (me->bstate & BUTTON2_PRESSED)   { evt.button = MouseEvent::Button::Middle; evt.kind = MouseEvent::Kind::Press; }
    else if (me->bstate & BUTTON2_RELEASED)  { evt.button = MouseEvent::Button::Middle; evt.kind = MouseEvent::Kind::Release; }
    else if (me->bstate & BUTTON3_PRESSED)   { evt.button = MouseEvent::Button::Right;  evt.kind = MouseEvent::Kind::Press; }
    else if (me->bstate & BUTTON3_RELEASED)  { evt.button = MouseEvent::Button::Right;  evt.kind = MouseEvent::Kind::Release; }
    else if (me->bstate & BUTTON4_PRESSED)   { evt.button = MouseEvent::Button::ScrollUp;   evt.kind = MouseEvent::Kind::Press; }
    else if (me->bstate & BUTTON5_PRESSED)   { evt.button = MouseEvent::Button::ScrollDown; evt.kind = MouseEvent::Kind::Press; }
    else                                     { evt.kind = MouseEvent::Kind::Move; }

    return evt;
}

// ---------------------------------------------------------------------------
// Event polling
// ---------------------------------------------------------------------------

std::optional<Event> NcursesBackend::poll_event(int timeout_ms) {
    timeout(timeout_ms);
    int ch = getch();

    if (ch == ERR) return std::nullopt; // timeout

    if (ch == KEY_RESIZE) {
        // ncurses fires KEY_RESIZE on SIGWINCH
        int rows, cols;
        getmaxyx(stdscr, rows, cols);
        // Resize buffers; sentinel in front_buf_ forces full redraw on next frame
        resize_bufs(cols, rows);
        return ResizeEvent{cols, rows};
    }

    if (ch == KEY_MOUSE) {
        MEVENT me;
        if (getmouse(&me) == OK) {
            return translate_mouse(&me);
        }
        return std::nullopt;
    }

    // Alt prefix: ESC followed by another key (within set_escdelay ms)
    if (ch == 27) {
        timeout(0); // non-blocking peek
        int next = getch();
        if (next != ERR) {
            KeyEvent ke;
            ke.alt = true;
            ke.key = next;
            return ke;
        }
        // Bare ESC key
        return KeyEvent{27, false, false, false};
    }

    // Ctrl key detection: ASCII 1–26 map to Ctrl+A through Ctrl+Z
    KeyEvent ke;
    ke.key  = ch;
    ke.ctrl = (ch >= 1 && ch <= 26 && ch != '\t' && ch != '\n' && ch != '\r');
    return ke;
}

} // namespace strata
