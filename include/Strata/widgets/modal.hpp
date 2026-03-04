#pragma once
#include "../core/widget.hpp"
#include "../style/style.hpp"
#include <functional>
#include <memory>
#include <string>
#include <utility>

namespace strata {

// Modal renders a full-screen dimming overlay with a centered dialog box.
// It is NOT part of the widget tree — it is managed by ModalManager and
// rendered in a separate pass after root_->render().
//
// The inner widget receives a canvas shrunk by 1 cell on each side (the border).
// Focus is trapped inside the modal via FocusManager::push_scope().
// Pressing Escape calls on_close_, which should call App::close_modal(id).
class Modal : public Widget {
    std::string title_;
    int preferred_w_ = 60;
    int preferred_h_ = 20;
    Style border_style_;
    Style title_style_;
    Style overlay_style_;
    std::unique_ptr<Widget> inner_;
    std::function<void()> on_close_;

public:
    Modal() = default;

    Modal& set_title(std::string title) {
        title_ = std::move(title);
        return *this;
    }

    Modal& set_size(int w, int h) {
        preferred_w_ = w;
        preferred_h_ = h;
        return *this;
    }

    Modal& set_border_style(Style s) {
        border_style_ = s;
        return *this;
    }

    Modal& set_title_style(Style s) {
        title_style_ = s;
        return *this;
    }

    Modal& set_overlay_style(Style s) {
        overlay_style_ = s;
        return *this;
    }

    Modal& set_on_close(std::function<void()> fn) {
        on_close_ = std::move(fn);
        return *this;
    }

    // Take ownership of an existing widget as the inner content
    Widget* set_inner(std::unique_ptr<Widget> w);

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

    Widget* inner() const { return inner_.get(); }

    void render(Canvas& canvas) override;
    bool handle_event(const Event& e) override;
    void for_each_child(const std::function<void(Widget&)>& visitor) override;
    bool is_focusable() const override { return false; }
};

} // namespace strata
