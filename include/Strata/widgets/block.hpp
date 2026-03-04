#pragma once
#include "../core/widget.hpp"
#include "../style/style.hpp"
#include <string>
#include <memory>
#include <utility>

namespace strata {

// Block renders a bordered box with an optional title, wrapping one inner widget.
// The inner widget receives a canvas shrunk by 1 cell on each side (the border).
// When the inner widget is focused, focused_border_style_ / focused_title_style_
// are used instead — set them to give a visible focus indicator on the border.
class Block : public Widget {
    std::string title_;
    Style border_style_;
    Style title_style_;
    Style focused_border_style_;
    Style focused_title_style_;
    bool has_focused_styles_ = false;
    std::unique_ptr<Widget> inner_;

public:
    Block() = default;

    Block& set_title(std::string title);
    Block& set_border_style(Style s);
    Block& set_title_style(Style s);
    // Styles applied to the border/title when the inner widget is focused
    Block& set_focused_border_style(Style s);
    Block& set_focused_title_style(Style s);

    // Construct the inner widget in-place
    template<typename W, typename... Args>
    W* set_inner(Args&&... args) {
        auto w = std::make_unique<W>(std::forward<Args>(args)...);
        W* raw = w.get();
        w->parent_ = this;
        inner_ = std::move(w);
        mark_dirty();
        return raw;
    }

    // Take ownership of an existing widget
    Widget* set_inner(std::unique_ptr<Widget> w);

    void render(Canvas& canvas) override;
    bool handle_event(const Event& e) override;
    void for_each_child(const std::function<void(Widget&)>& visitor) override;
};

} // namespace strata
