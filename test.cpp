#include <Strata/ui.hpp>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <memory>

using namespace strata::ui;

// ── Data ─────────────────────────────────────────────────────────────────────
struct SystemData {
    float cpu = 0.0f;
    float ram = 0.0f;
    std::string time_str;
    std::vector<std::string> procs;
};

// ── Helpers ───────────────────────────────────────────────────────────────────
static float read_cpu(long& p_idle, long& p_total) {
    std::ifstream f("/proc/stat");
    std::string tag; long u, n, s, id, iow, irq, sirq;
    if (!(f >> tag >> u >> n >> s >> id >> iow >> irq >> sirq)) return 0.f;
    long total = u + n + s + id + iow + irq + sirq;
    long d_id = id - p_idle; long d_tot = total - p_total;
    p_idle = id; p_total = total;
    return (d_tot == 0) ? 0.f : 1.f - (static_cast<float>(d_id) / d_tot);
}

static float read_ram() {
    std::ifstream f("/proc/meminfo");
    long tot = 0, avail = 0; std::string k; long v; std::string u;
    while (f >> k >> v >> u) {
        if (k == "MemTotal:") tot = v;
        else if (k == "MemAvailable:") { avail = v; break; }
    }
    return (tot == 0) ? 0.f : 1.0f - (static_cast<float>(avail) / tot);
}

// ─────────────────────────────────────────────────────────────────────────────

int main() {
    App app;

    // ── Widget pointers ───────────────────────────────────────────────────────
    strata::ProgressBar* pb_cpu   = nullptr;
    strata::ProgressBar* pb_ram   = nullptr;
    strata::Label*       lbl_clock = nullptr;

    const int MAX_PROCS = 50;
    strata::Label* proc_labels[MAX_PROCS] = {nullptr};

    // ── Modal ID storage ──────────────────────────────────────────────────────
    int quit_modal_id = -1;
    int help_modal_id = -1;

    // ── UI layout ─────────────────────────────────────────────────────────────
    populate(app, {
        Label{"  STRATA MONITOR  |  q: Quit  |  ?: Help  |  n: Notify"}
            .style(Style{}.with_bg(color::Blue).with_fg(color::BrightWhite).with_bold())
            .size(fixed(1)),

        Row{
            Block{" Stats ",
                Col{
                    Label{" CPU Usage"}.style(Style{}.with_fg(color::BrightBlack)).size(fixed(1)),
                    ProgressBar{}.bind(pb_cpu).size(fixed(1)),
                    Label{" RAM Usage"}.style(Style{}.with_fg(color::BrightBlack)).size(fixed(1)),
                    ProgressBar{}.bind(pb_ram).size(fixed(1)),
                    Label{""}.size(fill(1)),
                    Label{" Time:"}.bind(lbl_clock).size(fixed(1))
                }.gap(1)
            }.size(percentage(30)),

            Block{" Process Explorer ",
                ScrollView{
                    Col{
                        Label{"Scanning..."}.size(fixed(1)),
                        Label{""}.bind(proc_labels[0]).size(fixed(1)),
                        Label{""}.bind(proc_labels[1]).size(fixed(1)),
                        Label{""}.bind(proc_labels[2]).size(fixed(1)),
                        Label{""}.bind(proc_labels[3]).size(fixed(1)),
                        Label{""}.bind(proc_labels[4]).size(fixed(1)),
                        Label{""}.bind(proc_labels[5]).size(fixed(1)),
                        Label{""}.bind(proc_labels[6]).size(fixed(1)),
                        Label{""}.bind(proc_labels[7]).size(fixed(1)),
                        Label{""}.bind(proc_labels[8]).size(fixed(1)),
                        Label{""}.bind(proc_labels[9]).size(fixed(1)),
                        Label{""}.bind(proc_labels[10]).size(fixed(1)),
                        Label{""}.bind(proc_labels[11]).size(fixed(1)),
                        Label{""}.bind(proc_labels[12]).size(fixed(1)),
                        Label{""}.bind(proc_labels[13]).size(fixed(1)),
                        Label{""}.bind(proc_labels[14]).size(fixed(1))
                    }.gap(0)
                }.group("procs")
            }.size(percentage(70))
        }.size(fill(1))
    });

    // ── Modal builders ────────────────────────────────────────────────────────

    // Quit confirmation modal — two buttons, Tab cycles between them, Escape cancels
    auto open_quit_modal = [&]() {
        if (app.has_modal()) return;
        auto m = ModalDesc{}
            .title(" Confirm Quit ")
            .size(42, 13)
            .border(Style{}.with_fg(color::Yellow).with_bold())
            .overlay(Style{}.with_bg(color::Black).with_fg(color::BrightBlack))
            .on_close([&]{ app.close_modal(quit_modal_id); })
            .inner(
                Col{
                    Label{"  Are you sure you want to quit?"}
                        .style(Style{}.with_bold())
                        .size(fixed(1)),
                    Label{""}.size(fixed(1)),
                    Label{"  Tab to switch  ·  Enter to confirm  ·  Esc to cancel"}
                        .style(Style{}.with_fg(color::BrightBlack))
                        .size(fixed(1)),
                    Label{""}.size(fill()),
                    Row{
                        Button{"[ Quit ]"}
                            .style(Style{}.with_fg(color::Black).with_bg(color::Red).with_bold())
                            .focused_style(Style{}.with_fg(color::BrightWhite).with_bg(color::BrightRed).with_bold())
                            .shadow(Style{}.with_bg(color::rgb(0,0,0)))
                            .click([&]{ app.quit(); })
                            .tab_index(0),
                        Button{"[ Cancel ]"}
                            .style(Style{}.with_fg(color::Black).with_bg(color::Green).with_bold())
                            .focused_style(Style{}.with_fg(color::BrightWhite).with_bg(color::BrightGreen).with_bold())
                            .shadow(Style{}.with_bg(color::rgb(0,0,0)))
                            .click([&]{ app.close_modal(quit_modal_id); })
                            .tab_index(1),
                    }.size(fixed(4))
                }
            )
            .build_modal();
        quit_modal_id = app.open_modal(std::move(m));
    };

    // Help / keybindings modal — informational, no buttons, Escape closes
    auto open_help_modal = [&]() {
        if (app.has_modal()) return;
        auto m = ModalDesc{}
            .title(" Keybindings ")
            .size(44, 14)
            .border(Style{}.with_fg(color::Cyan).with_bold())
            .overlay(Style{}.with_bg(color::Black).with_fg(color::BrightBlack))
            .on_close([&]{ app.close_modal(help_modal_id); })
            .inner(
                Col{
                    Label{"  Navigation"}.style(Style{}.with_bold().with_fg(color::Cyan)).size(fixed(1)),
                    Label{"  j / ↓        Scroll down / focus next"}.size(fixed(1)),
                    Label{"  k / ↑        Scroll up / focus prev"}.size(fixed(1)),
                    Label{"  Tab          Move between panels"}.size(fixed(1)),
                    Label{""}.size(fixed(1)),
                    Label{"  Actions"}.style(Style{}.with_bold().with_fg(color::Cyan)).size(fixed(1)),
                    Label{"  n            Fire a test notification"}.size(fixed(1)),
                    Label{"  ?            Show this help modal"}.size(fixed(1)),
                    Label{"  q            Quit (with confirmation)"}.size(fixed(1)),
                    Label{""}.size(fill()),
                    Label{"  Press Escape to close"}
                        .style(Style{}.with_fg(color::BrightBlack))
                        .size(fixed(1)),
                }
            )
            .build_modal();
        help_modal_id = app.open_modal(std::move(m));
    };

    // ── Timers ────────────────────────────────────────────────────────────────
    auto data    = std::make_shared<SystemData>();
    auto cpu_st  = std::make_shared<std::pair<long,long>>(0, 0);
    bool cpu_warned = false;

    // CPU/RAM + clock: every second
    app.set_interval(1000, [&, data, cpu_st]{
        app.run_async(
            [data, cpu_st]{
                data->cpu = read_cpu(cpu_st->first, cpu_st->second);
                data->ram = read_ram();
                time_t t = time(nullptr); char b[9];
                strftime(b, 9, "%H:%M:%S", localtime(&t));
                data->time_str = b;
            },
            [&, data]{
                if (pb_cpu) pb_cpu->set_value(data->cpu);
                if (pb_ram) pb_ram->set_value(data->ram);
                if (lbl_clock) lbl_clock->set_text(" Time: " + data->time_str);

                // Warn once when CPU spikes above 80 %
                if (data->cpu > 0.80f && !cpu_warned) {
                    cpu_warned = true;
                    app.notify("CPU Warning",
                               "Usage above 80% — consider closing background tasks.",
                               AlertLevel::Warning, 6000);
                } else if (data->cpu <= 0.80f) {
                    cpu_warned = false; // reset so we can warn again
                }
            }
        );
    });

    // Process list: every 5 seconds
    app.set_interval(5000, [&, data]{
        app.run_async(
            [data]{
                namespace fs = std::filesystem;
                data->procs.clear();
                int c = 0;
                for (auto& e : fs::directory_iterator("/proc")) {
                    std::string n = e.path().filename().string();
                    if (!std::all_of(n.begin(), n.end(), ::isdigit)) continue;
                    std::ifstream f(e.path().string() + "/comm");
                    std::string cmd; std::getline(f, cmd);
                    data->procs.push_back(n + "  " + cmd);
                    if (++c >= MAX_PROCS) break;
                }
            },
            [&, data]{
                for (int i = 0; i < MAX_PROCS; ++i) {
                    if (!proc_labels[i]) continue;
                    proc_labels[i]->set_text(
                        i < (int)data->procs.size() ? " " + data->procs[i] : "");
                }
                app.notify("Process List", "Refreshed " +
                           std::to_string(data->procs.size()) + " processes.",
                           AlertLevel::Info, 2500);
            }
        );
    });

    // ── Global key handler ────────────────────────────────────────────────────
    app.on_event = [&](const Event& e) {
        if (is_char(e, 'q')) { open_quit_modal(); }
        if (is_char(e, '?')) { open_help_modal(); }
        if (is_char(e, 'n')) {
            app.notify("Manual Notification",
                       "Triggered by pressing 'n'.",
                       AlertLevel::Success, 3000);
        }
    };

    app.run();
    return 0;
}
