#pragma once
#include <cstdint>
#include <variant>

namespace strata {

struct NamedColor {
    enum Value : uint8_t {
        Black   = 0,
        Red     = 1,
        Green   = 2,
        Yellow  = 3,
        Blue    = 4,
        Magenta = 5,
        Cyan    = 6,
        White   = 7,
        BrightBlack   = 8,
        BrightRed     = 9,
        BrightGreen   = 10,
        BrightYellow  = 11,
        BrightBlue    = 12,
        BrightMagenta = 13,
        BrightCyan    = 14,
        BrightWhite   = 15,
        Default = 255  // terminal default (rendered as -1 in ncurses)
    };
    Value value = Default;

    constexpr bool is_bright() const { return value >= 8 && value <= 15; }
    constexpr bool is_default() const { return value == Default; }
    // Base color index (0-7) for ncurses, ignoring bright modifier
    constexpr uint8_t base_index() const { return value % 8; }

    constexpr bool operator==(const NamedColor& o) const { return value == o.value; }
    constexpr bool operator!=(const NamedColor& o) const { return value != o.value; }
};

struct RgbColor {
    uint8_t r = 0, g = 0, b = 0;

    constexpr bool operator==(const RgbColor& o) const { return r == o.r && g == o.g && b == o.b; }
    constexpr bool operator!=(const RgbColor& o) const { return !(*this == o); }
};

using Color = std::variant<NamedColor, RgbColor>;

namespace color {
    inline constexpr Color Black          = NamedColor{NamedColor::Black};
    inline constexpr Color Red            = NamedColor{NamedColor::Red};
    inline constexpr Color Green          = NamedColor{NamedColor::Green};
    inline constexpr Color Yellow         = NamedColor{NamedColor::Yellow};
    inline constexpr Color Blue           = NamedColor{NamedColor::Blue};
    inline constexpr Color Magenta        = NamedColor{NamedColor::Magenta};
    inline constexpr Color Cyan           = NamedColor{NamedColor::Cyan};
    inline constexpr Color White          = NamedColor{NamedColor::White};
    inline constexpr Color BrightBlack    = NamedColor{NamedColor::BrightBlack};
    inline constexpr Color BrightRed      = NamedColor{NamedColor::BrightRed};
    inline constexpr Color BrightGreen    = NamedColor{NamedColor::BrightGreen};
    inline constexpr Color BrightYellow   = NamedColor{NamedColor::BrightYellow};
    inline constexpr Color BrightBlue     = NamedColor{NamedColor::BrightBlue};
    inline constexpr Color BrightMagenta  = NamedColor{NamedColor::BrightMagenta};
    inline constexpr Color BrightCyan     = NamedColor{NamedColor::BrightCyan};
    inline constexpr Color BrightWhite    = NamedColor{NamedColor::BrightWhite};
    inline constexpr Color Default        = NamedColor{NamedColor::Default};

    inline Color rgb(uint8_t r, uint8_t g, uint8_t b) { return RgbColor{r, g, b}; }
} // namespace color

} // namespace strata
