#include <Strata/ui.hpp>
using namespace strata::ui;
#include <vector>
#include <string>
#include <algorithm>

// ─────────────────────────────────────────────────────────────────────────────
// Kanban Board — dynamic widget showcase
//
// Demonstrates Container::add() / remove() / clear() at runtime:
//   • Adding a card  → sv->add<Checkbox>() mounts the widget and rebuilds focus
//   • Moving a card  → clear() + re-add to destination column
//   • Deleting       → erase from data + rebuild
//   • Clearing a col → sv->clear() unmounts all children at once
//
// Controls:
//   Tab / Shift-Tab  — jump between columns and the input row
//   ↑ / ↓           — navigate cards within a column
//   m               — move focused card to the next column (wraps)
//   d               — delete focused card
//   c               — clear the entire column of the focused card
//   q               — quit
// ─────────────────────────────────────────────────────────────────────────────

int main() {
    App app;

    struct Card {
        std::string text;
        int         col    = 0;   // 0=Backlog  1=In Progress  2=Done
        strata::Checkbox* widget = nullptr;
    };
    std::vector<Card> cards;

    // One ScrollView and one count label per column — populated via bind()
    strata::ScrollView* cols[3]   = {};
    strata::Label*      counts[3] = {};
    strata::Input*      inp       = nullptr;
    strata::Select*     col_sel   = nullptr;

    static const char* const kNames[3]  = { "Backlog", "In Progress", "Done" };
    static const char* const kGroups[3] = { "backlog", "progress",    "done" };

    // Per-column card styles
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

    // rebuild() — clear all three ScrollViews and re-add cards that belong there.
    // Each call to sv->add<Checkbox>() triggers on_mount() + focus rebuild.
    auto rebuild = [&]{
        for (int c = 0; c < 3; ++c) {
            if (cols[c]) cols[c]->clear();   // on_unmount() on every child
        }
        int cnt[3] = {};
        for (int i = 0; i < (int)cards.size(); ++i) {
            int c = cards[i].col;
            if (!cols[c]) { cards[i].widget = nullptr; continue; }
            ++cnt[c];
            auto* cb = cols[c]->add<strata::Checkbox>(   // on_mount() called here
                strata::Constraint::fixed(1), " " + cards[i].text);
            cb->set_style(card_style(c));
            cb->set_focused_style(card_focused_style(c));
            cb->set_focus_group(kGroups[c]);
            cards[i].widget = cb;
        }
        for (int c = 0; c < 3; ++c)
            if (counts[c])
                counts[c]->set_text(std::to_string(cnt[c]) +
                                    (cnt[c] == 1 ? " card" : " cards"));
    };

    populate(app, {
        Col({
            // ── Title bar ────────────────────────────────────────────────────
            Label("  Kanban  ─  m: move right  d: delete  c: clear column  q: quit")
                .style(Style{}.with_fg(color::BrightBlack))
                .size(fixed(1)),
            Label("").size(fixed(1)),

            // ── Add row ──────────────────────────────────────────────────────
            Row({
                Label(" Add to: ").style(Style{}.with_fg(color::BrightBlack)).size(fixed(9)).cross(fixed(1)),
                Select()
                    .items({"Backlog", "In Progress", "Done"})
                    .group("input")
                    .size(fixed(16))
                    .cross(fixed(1))
                    .bind(col_sel),
                Label("  ").size(fixed(2)).cross(fixed(1)),
                Block().inner(
                    Input()
                    .placeholder("card title, then Enter…")
                    .group("input")
                    .bind(inp)
                    .submit([&](const std::string& val){
                        if (!val.empty()) {
                            int c = col_sel ? col_sel->selected() : 0;
                            cards.push_back({val, c, nullptr});
                            inp->set_value("");
                            rebuild();
                        }
                    })
                ).border(Style{}.with_fg(color::White)).size(fill()),
            }).size(fixed(3)).cross_align(Layout::Align::Center),
            Label("").size(fixed(1)),

            // ── Three columns ────────────────────────────────────────────────
            Row({
                Block(" Backlog ")
                    .focused_title(Style{}.with_bg(color::Blue))
                    .border(Style{}.with_fg(color::Blue).with_bold())
                    .inner(Col({
                        Label("0 cards")
                            .style(Style{}.with_fg(color::BrightBlack))
                            .size(fixed(1))
                            .bind(counts[0]),
                        Label("").size(fixed(1)),
                        // ScrollView starts empty; cards are added via rebuild().
                        // tab_index(99) keeps the ScrollView after its cards in
                        // the focus order so Tab lands on a card, not the container.
                        ScrollView({}).group("backlog").tab_index(99).bind(cols[0]),
                    })),

                Block(" In Progress ")
                    .focused_title(Style{}.with_bg(color::Yellow))
                    .border(Style{}.with_fg(color::Yellow).with_bold())
                    .inner(Col({
                        Label("0 cards")
                            .style(Style{}.with_fg(color::BrightBlack))
                            .size(fixed(1))
                            .bind(counts[1]),
                        Label("").size(fixed(1)),
                        ScrollView({}).group("progress").tab_index(99).bind(cols[1]),
                    })),

                Block(" Done ")
                    .focused_title(Style{}.with_bg(color::Green))
                    .border(Style{}.with_fg(color::Green).with_bold())
                    .inner(Col({
                        Label("0 cards")
                            .style(Style{}.with_fg(color::BrightBlack))
                            .size(fixed(1))
                            .bind(counts[2]),
                        Label("").size(fixed(1)),
                        ScrollView({}).group("done").tab_index(99).bind(cols[2]),
                    })),
            }),
        })
    });

    app.on_event = [&](const Event& e) {
        if (is_char(e, 'q')) { app.quit(); return; }

        // Find which card is currently focused
        int focused = -1;
        for (int i = 0; i < (int)cards.size(); ++i)
            if (cards[i].widget && cards[i].widget->is_focused()) { focused = i; break; }

        if (focused < 0) return;

        // Move focused card to the next column
        if (is_char(e, 'm')) {
            int from = cards[focused].col;
            int to   = (from + 1) % 3;
            cards[focused].col = to;
            app.notify("Moved to " + std::string(kNames[to]), cards[focused].text,
                       strata::AlertManager::Level::Info, 1500);
            rebuild();
        }

        // Delete focused card
        if (is_char(e, 'd')) {
            app.notify("Deleted", cards[focused].text,
                       strata::AlertManager::Level::Warning, 1500);
            cards.erase(cards.begin() + focused);
            rebuild();
        }

        // Clear the entire column of the focused card
        if (is_char(e, 'c')) {
            int col = cards[focused].col;
            app.notify("Cleared " + std::string(kNames[col]), "",
                       strata::AlertManager::Level::Warning, 1500);
            cards.erase(std::remove_if(cards.begin(), cards.end(),
                [col](const Card& card){ return card.col == col; }), cards.end());
            rebuild();
        }
    };

    app.run();
}
