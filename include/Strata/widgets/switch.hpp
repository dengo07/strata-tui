#pragma once
#include "../core/widget.hpp"
#include "../style/style.hpp"
#include <string>
#include <functional>

namespace strata {

// Toggle switch: "Label ─── [ ON ]" / "Label ─── [OFF ]"
// Space/Enter toggles; fires on_change(is_on) after each toggle.
class Switch : public Widget {
    std::string label_;
    bool        on_ = false;

public:
    std::function<void(bool)> on_change;

    explicit Switch(std::string label = "", bool on = false);

    Switch& set_label(std::string label);
    Switch& set_on(bool on);

    bool is_on() const { return on_; }

    bool is_focusable() const override { return true; }
    void render(Canvas& canvas) override;
    bool handle_event(const Event& e) override;
};

} // namespace strata
