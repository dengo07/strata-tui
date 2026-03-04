#pragma once
#include "../core/widget.hpp"
#include "../style/style.hpp"
#include <string>
#include <functional>

namespace strata {

// Single-line editable text field.
// Left/Right move cursor; Home jumps to start;
// Backspace/Delete edit; Ctrl+U clears; Enter fires on_submit.
// on_change fires on every keystroke that modifies the value.
class Input : public Widget {
    std::string value_;
    std::string placeholder_;
    int         cursor_ = 0;

public:
    std::function<void(const std::string&)> on_submit;
    std::function<void(const std::string&)> on_change;

    Input& set_placeholder(std::string placeholder);
    Input& set_value(std::string value);

    const std::string& value() const { return value_; }

    bool is_focusable() const override { return true; }
    void render(Canvas& canvas) override;
    bool handle_event(const Event& e) override;
};

} // namespace strata
