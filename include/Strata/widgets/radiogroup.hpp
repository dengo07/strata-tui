#pragma once
#include "../core/widget.hpp"
#include "../style/style.hpp"
#include <string>
#include <vector>
#include <functional>

namespace strata {

// Vertical radio button list: (●) Selected / (○) Other
// j/k or ↑↓ navigate; Space/Enter selects item.
// Fires on_change(index, item_text) on selection change.
class RadioGroup : public Widget {
    std::vector<std::string> items_;
    int selected_ = 0;
    int scroll_   = 0;

public:
    std::function<void(int, const std::string&)> on_change;

    RadioGroup& add_item(std::string item);
    RadioGroup& set_items(std::vector<std::string> items);
    RadioGroup& set_selected(int idx);

    int                selected()      const { return selected_; }
    const std::string& selected_item() const;

    bool is_focusable() const override { return true; }
    void render(Canvas& canvas) override;
    bool handle_event(const Event& e) override;
};

} // namespace strata
