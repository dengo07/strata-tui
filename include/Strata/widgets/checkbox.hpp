#pragma once
#include "../core/widget.hpp"
#include "../style/style.hpp"
#include <string>
#include <functional>

namespace strata {

// Single checkbox: "[x] Label" or "[ ] Label"
// Space/Enter toggles. Fires on_change(checked) after each toggle.
class Checkbox : public Widget {
    std::string label_;
    bool        checked_ = false;

public:
    std::function<void(bool)> on_change;

    explicit Checkbox(std::string label = "", bool checked = false);

    Checkbox& set_label(std::string label);
    Checkbox& set_checked(bool checked);

    bool is_checked() const { return checked_; }

    bool is_focusable() const override { return true; }
    void render(Canvas& canvas) override;
    bool handle_event(const Event& e) override;
};

} // namespace strata
