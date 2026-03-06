#include <Strata/ui.hpp>
using namespace strata::ui;
#include <string>
#include <algorithm>

// ─────────────────────────────────────────────────────────────────────────────
// Reactive Kanban — Flutter/SwiftUI-style reactive showcase
//
// State is declared once as Reactive<vector<Card>>.
// Mutate it anywhere and the UI updates automatically — no rebuild() needed.
//
// Controls:
//   Tab / Shift-Tab  — jump between columns and the input row
//   ↑ / ↓           — navigate cards within a column
//   m               — move the focused card to the next column (wraps)
//   d               — delete the focused card
//   c               — clear all cards in the focused card's column
//   q               — quit
// ─────────────────────────────────────────────────────────────────────────────

int main() {
    App app;

    struct Card {
        int         id;
        std::string text;
        int         col = 0;   // 0=Backlog  1=In Progress  2=Done
    };

    // ── Reactive state ────────────────────────────────────────────────────────
    auto cards = make_state(std::vector<Card>{});
    int  next_id     = 0;
    int  selected_id = -1;   // id of last focused card (updated by on_focused)

    // ── Constants ─────────────────────────────────────────────────────────────
    static const char* const kNames[3]  = { "Backlog", "In Progress", "Done" };
    static const char* const kGroups[3] = { "backlog", "progress",    "done" };

    auto card_style = [](int c) -> Style {
        if (c == 0) return Style{}.with_fg(color::BrightBlue);
        if (c == 1) return Style{}.with_fg(color::BrightYellow);
        return Style{}.with_fg(color::BrightGreen);
    };
    auto card_focused_style = [](int c) -> Style {
        if (c == 0) return Style{}.with_fg(color::Black).with_bg(color::BrightBlue).with_bold();
        if (c == 1) return Style{}.with_fg(color::Black).with_bg(color::BrightYellow).with_bold();
        return Style{}.with_fg(color::Black).with_bg(color::BrightGreen).with_bold();
    };

    // ── Widgets that need runtime access ─────────────────────────────────────
    strata::Input*  inp     = nullptr;
    strata::Select* col_sel = nullptr;

    // ── UI declaration ────────────────────────────────────────────────────────
    populate(app, {
        Col({
            // Title bar
            Label("  Kanban  ─  m: move right  d: delete  c: clear column  q: quit")
                .style(Style{}.with_fg(color::BrightBlack))
                .size(fixed(1)),
            Label("").size(fixed(1)),

            // Add-card row
            Row({
                Label(" Add to: ").style(Style{}.with_fg(color::BrightBlack))
                    .size(fixed(9)).cross(fixed(1)),
                Select()
                    .items({"Backlog", "In Progress", "Done"})
                    .group("input")
                    .size(fixed(16)).cross(fixed(1))
                    .bind(col_sel),
                Label("  ").size(fixed(2)).cross(fixed(1)),
                Block().inner(
                    Input()
                        .placeholder("card title, then Enter…")
                        .group("input")
                        .bind(inp)
                        .submit([&](const std::string& val) {
                            if (val.empty()) return;
                            int c  = col_sel ? col_sel->selected() : 0;
                            int id = next_id++;
                            // One mutate() → all three ForEach columns rebuild
                            cards.mutate([&](auto& v){ v.push_back({id, val, c}); });
                            inp->set_value("");
                        })
                ).border(Style{}.with_fg(color::White)).size(fill()),
            }).size(fixed(3)).cross_align(Layout::Align::Center),
            Label("").size(fixed(1)),

            // Three columns, each driven by a ForEach
            Row({
                Block(" Backlog ")
                    .focused_title(Style{}.with_bg(color::Blue))
                    .border(Style{}.with_fg(color::Blue).with_bold())
                    .inner(
                        ForEach(cards, [&](const Card& card, int /*i*/) -> std::optional<Node> {
                            if (card.col != 0) return std::nullopt;
                            int id = card.id;
                            return Checkbox(card.text)
                                .group(kGroups[0])
                                .style(card_style(0))
                                .focused_style(card_focused_style(0))
                                .focused([&selected_id, id]{ selected_id = id; })
                                .size(fixed(1));
                        })
                    ),

                Block(" In Progress ")
                    .focused_title(Style{}.with_bg(color::Yellow))
                    .border(Style{}.with_fg(color::Yellow).with_bold())
                    .inner(
                        ForEach(cards, [&](const Card& card, int /*i*/) -> std::optional<Node> {
                            if (card.col != 1) return std::nullopt;
                            int id = card.id;
                            return Checkbox(card.text)
                                .group(kGroups[1])
                                .style(card_style(1))
                                .focused_style(card_focused_style(1))
                                .focused([&selected_id, id]{ selected_id = id; })
                                .size(fixed(1));
                        })
                    ),

                Block(" Done ")
                    .focused_title(Style{}.with_bg(color::Green))
                    .border(Style{}.with_fg(color::Green).with_bold())
                    .inner(
                        ForEach(cards, [&](const Card& card, int /*i*/) -> std::optional<Node> {
                            if (card.col != 2) return std::nullopt;
                            int id = card.id;
                            return Checkbox(card.text)
                                .group(kGroups[2])
                                .style(card_style(2))
                                .focused_style(card_focused_style(2))
                                .focused([&selected_id, id]{ selected_id = id; })
                                .size(fixed(1));
                        })
                    ),
            }),
        })
    });

    // ── Global key handlers ───────────────────────────────────────────────────
    app.on_event = [&](const Event& e) {
        if (is_char(e, 'q')) { app.quit(); return; }
        if (selected_id < 0) return;

        if (is_char(e, 'm')) {
            // Move focused card to the next column
            int to = -1;
            cards.mutate([&](auto& v) {
                for (auto& c : v) {
                    if (c.id == selected_id) {
                        to = (c.col + 1) % 3;
                        c.col = to;
                        break;
                    }
                }
            });
            if (to >= 0)
                app.notify("Moved to " + std::string(kNames[to]), "",
                           strata::AlertManager::Level::Info, 1500);
        }

        if (is_char(e, 'd')) {
            std::string txt;
            for (const auto& c : cards.get())
                if (c.id == selected_id) { txt = c.text; break; }
            app.notify("Deleted", txt, strata::AlertManager::Level::Warning, 1500);
            cards.mutate([&](auto& v) {
                v.erase(std::remove_if(v.begin(), v.end(),
                    [&](const Card& c){ return c.id == selected_id; }), v.end());
            });
            selected_id = -1;
        }

        if (is_char(e, 'c')) {
            int col = -1;
            for (const auto& c : cards.get())
                if (c.id == selected_id) { col = c.col; break; }
            if (col >= 0) {
                app.notify("Cleared " + std::string(kNames[col]), "",
                           strata::AlertManager::Level::Warning, 1500);
                cards.mutate([col](auto& v) {
                    v.erase(std::remove_if(v.begin(), v.end(),
                        [col](const Card& c){ return c.col == col; }), v.end());
                });
                selected_id = -1;
            }
        }
    };

    app.run();
}
