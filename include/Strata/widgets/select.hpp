#pragma once
#include "../core/widget.hpp"
#include "../style/style.hpp"
#include <string>
#include <vector>
#include <functional>

namespace strata {

// Single-row cycling selector: "◀ Item ▶" (focused) or "  Item  " (unfocused).
// Left/Right/Up/Down or h/j/k/l cycle through items (wraps around).
// Fires on_change(index, item_text) on each change.
class Select : public Widget {
    std::vector<std::string> items_;
    int selected_ = 0;

public:
    std::function<void(int, const std::string&)> on_change;

    Select& set_items(std::vector<std::string> items);
    Select& set_selected(int idx);

    int                selected()      const { return selected_; }
    const std::string& selected_item() const;

    bool is_focusable() const override { return true; }
    void render(Canvas& canvas) override;
    bool handle_event(const Event& e) override;
};

} // namespace strata
