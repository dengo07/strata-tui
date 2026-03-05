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
    int del_slot   = -1;      // slot index being deleted (set when modal opens)
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
                .focused_style(Style{}.with_bg(color::White))
                .group("list")
                .bind(slots[i])
                .change([&, i](bool checked){
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
        // Find which slot is currently focused
        int focused_slot = -1;
        for (int i = 0; i < MAX_ITEMS; ++i)
            if (slots[i] && slots[i]->is_focused()) { focused_slot = i; break; }

        if (is_char(e, 'd') && focused_slot >= 0 && focused_slot < (int)view.size()) {
            del_slot = focused_slot;
            const std::string item_text = items[view[del_slot]].text;
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
                                        if (del_slot < (int)view.size()) {
                                            items.erase(items.begin() + view[del_slot]);
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