#pragma once
#include "../core/widget.hpp"
#include "../style/style.hpp"
#include <string>
#include <vector>

namespace strata {

// Read-only multiline text viewer.
// j/k or ↑↓ scroll. append() adds a line and scrolls to the bottom.
// Optional word-wrap reflows text to canvas width.
class TextBox : public Widget {
    std::vector<std::string> lines_;
    int  scroll_ = 0;
    bool wrap_   = false;

public:
    // Replace all content (splits on '\n')
    TextBox& set_text(std::string text);
    TextBox& set_wrap(bool wrap);
    // Append a line and scroll to bottom
    TextBox& append(std::string line);
    TextBox& clear();

    bool is_focusable() const override { return true; }
    void render(Canvas& canvas) override;
    bool handle_event(const Event& e) override;

private:
    std::vector<std::string> display_lines(int width) const;
};

} // namespace strata
