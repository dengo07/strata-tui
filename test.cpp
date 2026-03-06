#include <Strata/ui.hpp>
using namespace strata::ui;
#include <memory>
#include <array>
#include <fstream>
#include <sstream>
#include <algorithm>

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

    // Delta state for CPU and network (only accessed from the bg thread)
    struct ProcState {
        long long cpu_idle  = 0;
        long long cpu_total = 0;
        long long net_bytes = 0;
    };
    auto state = std::make_shared<ProcState>();

    struct Vals { float cpu = 0, mem = 0, net = 0; std::string info; };

    app.set_interval(1000, [&, state]{
        ++tick;
        auto vals = std::make_shared<Vals>();
        app.run_async(
            // Background thread — reads /proc files; must NOT touch widgets
            [vals, state]{
                // CPU: /proc/stat first line
                {
                    std::ifstream f("/proc/stat");
                    std::string line;
                    std::getline(f, line); // "cpu  user nice system idle iowait irq softirq steal ..."
                    std::istringstream ss(line.substr(5));
                    long long u, n, s, idle, iowait, irq, softirq, steal;
                    ss >> u >> n >> s >> idle >> iowait >> irq >> softirq >> steal;
                    long long total    = u + n + s + idle + iowait + irq + softirq + steal;
                    long long idle_all = idle + iowait;
                    if (state->cpu_total > 0) {
                        long long dt = total    - state->cpu_total;
                        long long di = idle_all - state->cpu_idle;
                        vals->cpu = dt > 0 ? std::max(0.0f, (float)(dt - di) / dt) : 0.0f;
                    }
                    state->cpu_total = total;
                    state->cpu_idle  = idle_all;
                }
                // Memory: /proc/meminfo
                {
                    std::ifstream f("/proc/meminfo");
                    std::string line;
                    long long mem_total = 0, mem_avail = 0;
                    while (std::getline(f, line)) {
                        if (line.compare(0, 9, "MemTotal:") == 0)
                            std::istringstream(line.substr(9)) >> mem_total;
                        else if (line.compare(0, 13, "MemAvailable:") == 0)
                            std::istringstream(line.substr(13)) >> mem_avail;
                        if (mem_total && mem_avail) break;
                    }
                    vals->mem = mem_total > 0
                        ? std::min(1.0f, (float)(mem_total - mem_avail) / mem_total)
                        : 0.0f;
                }
                // Network: /proc/net/dev, summed across interfaces, capped at 10 MB/s
                {
                    std::ifstream f("/proc/net/dev");
                    std::string line;
                    std::getline(f, line); // header 1
                    std::getline(f, line); // header 2
                    long long bytes = 0;
                    while (std::getline(f, line)) {
                        std::istringstream ns(line);
                        std::string iface;
                        ns >> iface;
                        if (iface == "lo:") continue;
                        long long rx; ns >> rx;
                        long long dummy;
                        for (int j = 0; j < 7; ++j) ns >> dummy;
                        long long tx; ns >> tx;
                        bytes += rx + tx;
                    }
                    if (state->net_bytes > 0) {
                        long long db = std::max(0LL, bytes - state->net_bytes);
                        vals->net = std::min(1.0f, (float)db / (10.0f * 1024 * 1024));
                    }
                    state->net_bytes = bytes;
                }
                vals->info = "  CPU " + std::to_string((int)(vals->cpu * 100)) + "%"
                           + "  Mem " + std::to_string((int)(vals->mem * 100)) + "%"
                           + "  Net " + std::to_string((int)(vals->net * 10)) + " MB/s";
            },
            // Main thread — safe to call widget setters
            [&, vals]{
                if (cpu_bar) cpu_bar->set_value(vals->cpu);
                if (mem_bar) mem_bar->set_value(vals->mem);
                if (net_bar) net_bar->set_value(vals->net);
                if (cpu_st) { cpu_st->set_text(threshold_text(vals->cpu)); cpu_st->set_style(threshold_style(vals->cpu)); }
                if (mem_st) { mem_st->set_text(threshold_text(vals->mem)); mem_st->set_style(threshold_style(vals->mem)); }
                if (net_st) { net_st->set_text(threshold_text(vals->net)); net_st->set_style(threshold_style(vals->net)); }
                if (info_lbl) info_lbl->set_text(vals->info);
            }
        );
    });

    app.on_event = [&](const Event& e) {
        if (is_char(e, 'q')) app.quit();
    };

    app.run();
}