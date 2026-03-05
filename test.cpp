#include <Strata/ui.hpp>
using namespace strata::ui;
#include <memory>
#include <thread>
#include <chrono>

int main() {
    App app;
    strata::ProgressBar* pb    = nullptr;
    strata::Label*       lbl   = nullptr;

    populate(app, {
        Col({
            Block("System Monitor")
                .border(Style{}.with_fg(color::Blue).with_bold())
                .inner(
                    Col({
                        Label("CPU Usage")
                            .style(Style{}.with_fg(color::BrightWhite).with_bold())
                            .size(fixed(1)),
                        ProgressBar().size(fixed(1)).bind(pb),
                        Label("").size(fixed(1)),
                        Label("Waiting for first sample…")
                            .style(Style{}.with_fg(color::BrightBlack))
                            .size(fixed(1))
                            .bind(lbl),
                    })
                )
                .size(fixed(8))
                .cross(fixed(48)),
        }).justify(Layout::Justify::Center)
          .cross_align(Layout::Align::Center)
    });

    app.set_interval(1000, [&]{
        auto result = std::make_shared<float>(0.0f);
        app.run_async(
            // Background thread — must NOT touch widgets
            [result]{
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                *result = 0.42f;  // replace with real measurement
            },
            // Main thread — safe to call widget setters
            [&, result]{
                if (pb)  pb->set_value(*result);
                if (lbl) lbl->set_text("Last reading: " +
                    std::to_string((int)(*result * 100)) + "%");
            }
        );
    });

    app.on_event = [&](const Event& e) {
        if (is_char(e, 'q')) app.quit();
    };

    app.run();
}