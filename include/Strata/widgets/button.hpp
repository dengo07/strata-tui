#pragma once
#include "../core/widget.hpp"
#include "../style/style.hpp"
#include <string>
#include <functional>

namespace strata {

class Button : public Widget {
    std::string label_;
    Style style_;
    Style focused_style_;
    Style shadow_style_;
    bool  has_shadow_style_ = false;

public:
    std::function<void()> on_click;

    explicit Button(std::string label = "Button");

    Button& set_label(std::string label);
    Button& set_style(Style style);
    Button& set_focused_style(Style style);
    Button& set_shadow_style(Style style);

    const std::string& label() const { return label_; }

    bool is_focusable() const override { return true; }
    void render(Canvas& canvas) override;
    bool handle_event(const Event& e) override;
};

} // namespace strata
