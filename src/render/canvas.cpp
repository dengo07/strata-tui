#include "../../include/Strata/render/canvas.hpp"
#include <algorithm>
#include <cwchar>

namespace strata {

// Unicode box-drawing characters
static const char32_t BOX_TOP_LEFT     = U'\u250C'; // ┌
static const char32_t BOX_TOP_RIGHT    = U'\u2510'; // ┐
static const char32_t BOX_BOTTOM_LEFT  = U'\u2514'; // └
static const char32_t BOX_BOTTOM_RIGHT = U'\u2518'; // ┘
static const char32_t BOX_HORIZONTAL   = U'\u2500'; // ─
static const char32_t BOX_VERTICAL     = U'\u2502'; // │

Canvas::Canvas(Backend& backend, Rect area)
    : backend_(backend), area_(area) {}

void Canvas::draw_cell(int x, int y, const Cell& cell) {
    int abs_x = area_.x + x;
    int abs_y = area_.y + y;
    if (abs_x < area_.x || abs_x >= area_.right()) return;
    if (abs_y < area_.y || abs_y >= area_.bottom()) return;
    backend_.draw_cell(abs_x, abs_y, cell);
}

void Canvas::draw_text(int x, int y, std::string_view text, const Style& style) {
    int abs_y = area_.y + y;
    if (abs_y < area_.y || abs_y >= area_.bottom()) return;

    int col = 0;
    // Iterate over bytes — treat as UTF-8 and extract codepoints
    // For simplicity, handle ASCII (single-byte) and multi-byte sequences
    size_t i = 0;
    while (i < text.size()) {
        int abs_x = area_.x + x + col;

        // Parse UTF-8 codepoint
        char32_t cp = 0;
        unsigned char byte = static_cast<unsigned char>(text[i]);
        size_t char_bytes = 1;

        if (byte < 0x80) {
            cp = byte;
        } else if ((byte & 0xE0) == 0xC0 && i + 1 < text.size()) {
            cp = (byte & 0x1F);
            cp = (cp << 6) | (static_cast<unsigned char>(text[i+1]) & 0x3F);
            char_bytes = 2;
        } else if ((byte & 0xF0) == 0xE0 && i + 2 < text.size()) {
            cp = (byte & 0x0F);
            cp = (cp << 6) | (static_cast<unsigned char>(text[i+1]) & 0x3F);
            cp = (cp << 6) | (static_cast<unsigned char>(text[i+2]) & 0x3F);
            char_bytes = 3;
        } else if ((byte & 0xF8) == 0xF0 && i + 3 < text.size()) {
            cp = (byte & 0x07);
            cp = (cp << 6) | (static_cast<unsigned char>(text[i+1]) & 0x3F);
            cp = (cp << 6) | (static_cast<unsigned char>(text[i+2]) & 0x3F);
            cp = (cp << 6) | (static_cast<unsigned char>(text[i+3]) & 0x3F);
            char_bytes = 4;
        }
        i += char_bytes;

        // Determine display column width (wcwidth returns -1 for non-printable, 2 for wide chars)
        int cw = wcwidth(static_cast<wchar_t>(cp));
        if (cw <= 0) continue; // skip control chars and combining marks

        // Clip horizontally
        if (abs_x >= area_.right()) break;
        if (abs_x >= area_.x) {
            backend_.draw_cell(abs_x, abs_y, Cell{cp, style});
            // For 2-wide chars, fill the second column with a space so the next char starts correctly
            if (cw == 2 && abs_x + 1 < area_.right()) {
                backend_.draw_cell(abs_x + 1, abs_y, Cell{U' ', style});
            }
        }
        col += cw;
    }
}

void Canvas::fill(char32_t ch, const Style& style) {
    Cell cell{ch, style};
    for (int row = 0; row < area_.height; ++row) {
        for (int col = 0; col < area_.width; ++col) {
            backend_.draw_cell(area_.x + col, area_.y + row, cell);
        }
    }
}

void Canvas::draw_border(const Style& style) {
    if (area_.width < 2 || area_.height < 2) return;

    int right  = area_.x + area_.width  - 1;
    int bottom = area_.y + area_.height - 1;

    // Corners
    backend_.draw_cell(area_.x, area_.y, {BOX_TOP_LEFT,     style});
    backend_.draw_cell(right,   area_.y, {BOX_TOP_RIGHT,    style});
    backend_.draw_cell(area_.x, bottom,  {BOX_BOTTOM_LEFT,  style});
    backend_.draw_cell(right,   bottom,  {BOX_BOTTOM_RIGHT, style});

    // Top and bottom edges
    for (int col = area_.x + 1; col < right; ++col) {
        backend_.draw_cell(col, area_.y, {BOX_HORIZONTAL, style});
        backend_.draw_cell(col, bottom,  {BOX_HORIZONTAL, style});
    }

    // Left and right edges
    for (int row = area_.y + 1; row < bottom; ++row) {
        backend_.draw_cell(area_.x, row, {BOX_VERTICAL, style});
        backend_.draw_cell(right,   row, {BOX_VERTICAL, style});
    }
}

void Canvas::draw_title(std::string_view title, const Style& style) {
    if (area_.width < 4 || title.empty()) return;
    // Place title at top border: "─ Title ─"
    // Position: column 2 (after corner + 1 space)
    int max_title_width = area_.width - 4; // 2 border cells + 2 spaces
    if (max_title_width <= 0) return;

    // Draw " Title " starting at x=2 on the top border row
    int start_x = area_.x + 2;
    int abs_y   = area_.y;

    backend_.draw_cell(start_x - 1, abs_y, {U' ', style});
    int col = 0;
    size_t i = 0;
    while (i < title.size() && col < max_title_width) {
        unsigned char byte = static_cast<unsigned char>(title[i]);
        char32_t cp = 0;
        size_t char_bytes = 1;

        if (byte < 0x80) {
            cp = byte;
        } else if ((byte & 0xE0) == 0xC0 && i + 1 < title.size()) {
            cp = (byte & 0x1F);
            cp = (cp << 6) | (static_cast<unsigned char>(title[i+1]) & 0x3F);
            char_bytes = 2;
        } else if ((byte & 0xF0) == 0xE0 && i + 2 < title.size()) {
            cp = (byte & 0x0F);
            cp = (cp << 6) | (static_cast<unsigned char>(title[i+1]) & 0x3F);
            cp = (cp << 6) | (static_cast<unsigned char>(title[i+2]) & 0x3F);
            char_bytes = 3;
        } else if ((byte & 0xF8) == 0xF0 && i + 3 < title.size()) {
            cp = (byte & 0x07);
            cp = (cp << 6) | (static_cast<unsigned char>(title[i+1]) & 0x3F);
            cp = (cp << 6) | (static_cast<unsigned char>(title[i+2]) & 0x3F);
            cp = (cp << 6) | (static_cast<unsigned char>(title[i+3]) & 0x3F);
            char_bytes = 4;
        }
        i += char_bytes;

        int cw = wcwidth(static_cast<wchar_t>(cp));
        if (cw <= 0) continue; // skip non-printable / combining marks

        if (start_x + col >= area_.right()) break;
        backend_.draw_cell(start_x + col, abs_y, {cp, style});
        if (cw == 2 && start_x + col + 1 < area_.right()) {
            backend_.draw_cell(start_x + col + 1, abs_y, {U' ', style});
        }
        col += cw;
    }
    backend_.draw_cell(start_x + col, abs_y, {U' ', style});
}

Canvas Canvas::sub_canvas(Rect abs_rect) const {
    // Clamp abs_rect to this canvas's bounds
    int clamped_x = std::max(abs_rect.x, area_.x);
    int clamped_y = std::max(abs_rect.y, area_.y);
    int clamped_r = std::min(abs_rect.right(),  area_.right());
    int clamped_b = std::min(abs_rect.bottom(), area_.bottom());

    int w = std::max(0, clamped_r - clamped_x);
    int h = std::max(0, clamped_b - clamped_y);

    return Canvas(backend_, {clamped_x, clamped_y, w, h});
}

} // namespace strata
