#pragma once
#include "../core/widget.hpp"
#include "../style/style.hpp"
#include <string>

namespace strata {

class Label : public Widget {
    std::string text_;
    Style style_;
    bool wrap_ = false;

public:
    explicit Label(std::string text = "", Style style = {});

    Label& set_text(std::string text);
    Label& set_style(Style style);
    Label& set_wrap(bool wrap);

    const std::string& text()  const { return text_; }
    const Style&       style() const { return style_; }

    void render(Canvas& canvas) override;
};

} // namespace strata
