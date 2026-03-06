#include <Strata/ui.hpp>
using namespace strata::ui;
#include <vector>
#include <string>
#include <algorithm>

int main() {
    App app;

    // widget pointer stored per item so we can detect focus and call remove()
    struct Item { std::string text; bool done = false; strata::Checkbox* widget = nullptr; };
    std::vector<Item> items;

    strata::Label*      stats_lbl    = nullptr;
    strata::Input*      input_widget = nullptr;
    strata::Select*     filter_sel   = nullptr;
    strata::ScrollView* sv           = nullptr;
    int del_idx    = -1;
    int confirm_id = -1;

    auto update_stats = [&]{
        if (!stats_lbl) return;
        int done = (int)std::count_if(items.begin(), items.end(),
                        [](const Item& it){ return it.done; });
        stats_lbl->set_text(std::to_string(done) + "/" +
                            std::to_string(items.size()) + " done");
    };

    // rebuild_list() clears the ScrollView and re-adds one Checkbox per visible item.
    // Called on add, delete, and filter change.
    std::function<void()> rebuild_list;
    rebuild_list = [&]{
        if (!sv) return;
        int mode = filter_sel ? filter_sel->selected() : 0;
        sv->clear();  // unmounts all existing checkboxes
        for (int i = 0; i < (int)items.size(); ++i) {
            items[i].widget = nullptr;
            if (mode == 1 && items[i].done)  continue;  // Active: hide done
            if (mode == 2 && !items[i].done) continue;  // Done:   hide active
            auto* cb = sv->add<strata::Checkbox>(
                strata::Constraint::fixed(1), " " + items[i].text, items[i].done);
            cb->set_focus_group("list");
            cb->set_style(Style{}.with_fg(color::White));
            cb->set_focused_style(Style{}.with_fg(color::Black).with_bg(color::Cyan));
            cb->on_change = [&, i](bool checked){
                items[i].done = checked;
                update_stats();
            };
            items[i].widget = cb;
        }
        update_stats();
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
                                if (!val.empty()) {
                                    items.push_back({val, false, nullptr});
                                    input_widget->set_value("");
                                    rebuild_list();
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
                            .change([&](int, const std::string&){ rebuild_list(); }),
                        Label("").style(Style{}.with_fg(color::BrightBlack))
                            .size(fixed(10)).bind(stats_lbl),
                    }).size(fixed(1)),
                    Label("").size(fixed(1)),
                    ScrollView({}).group("list").bind(sv),  // starts empty
                    Label("  Tab: switch section  ·  d: delete item")
                        .style(Style{}.with_fg(color::BrightBlack))
                        .size(fixed(1)),
                })
            )
    });

    app.on_event = [&](const Event& e) {
        if (app.has_modal()) return;
        if (is_char(e, 'q')) app.quit();
        if (is_char(e, 'd')) {
            del_idx = -1;
            for (int i = 0; i < (int)items.size(); ++i)
                if (items[i].widget && items[i].widget->is_focused()) { del_idx = i; break; }

            if (del_idx >= 0) {
                confirm_id = app.open_modal(
                    ModalDesc()
                        .title(" Delete Item ")
                        .size(44, 8)
                        .border(Style{}.with_fg(color::Red))
                        .on_close([&]{ app.close_modal(confirm_id); })
                        .inner(
                            Col({
                                Label("  Delete \"" + items[del_idx].text + "\"?")
                                    .style(Style{}.with_fg(color::White))
                                    .size(fixed(1)),
                                Label("").size(fixed(1)),
                                Row({
                                    Button("Yes")
                                        .style(Style{}.with_bg(color::Red).with_fg(color::White).with_bold())
                                        .focused_style(Style{}.with_bg(color::BrightRed).with_fg(color::White).with_bold())
                                        .click([&]{
                                            app.close_modal(confirm_id);
                                            if (del_idx >= 0 && del_idx < (int)items.size()) {
                                                items.erase(items.begin() + del_idx);
                                                rebuild_list();
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
        }
    };

    app.run();
}