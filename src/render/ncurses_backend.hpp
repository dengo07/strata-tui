#pragma once
#include "../../include/Strata/render/backend.hpp"
#include "../../include/Strata/style/cell.hpp"
#include <map>
#include <utility>
#include <vector>

namespace strata {

class NcursesBackend : public Backend {
public:
    NcursesBackend() = default;
    ~NcursesBackend() override;

    void init()     override;
    void shutdown() override;

    int width()  const override;
    int height() const override;

    void draw_cell(int x, int y, const Cell& cell) override;
    void clear()  override;
    void flush()  override;

    std::optional<Event> poll_event(int timeout_ms = 16) override;

private:
    bool initialized_ = false;
    bool can_256_      = false;

    // Double-buffer: back_buf_ is written during draw_cell(); flush() diffs vs front_buf_
    std::vector<Cell> front_buf_;
    std::vector<Cell> back_buf_;
    void resize_bufs(int w, int h);
    void render_cell(int x, int y, const Cell& cell);  // direct ncurses draw

    // Color pair cache: maps (fg_idx, bg_idx) → ncurses pair id
    struct PairKey {
        short fg, bg;
        bool operator<(const PairKey& o) const {
            if (fg != o.fg) return fg < o.fg;
            return bg < o.bg;
        }
    };
    std::map<PairKey, short> pair_cache_;
    short next_pair_id_ = 1;

    short resolve_color(const Color& color) const;
    short alloc_pair(short fg, short bg);
    MouseEvent translate_mouse(void* mevent_ptr);
};

} // namespace strata
