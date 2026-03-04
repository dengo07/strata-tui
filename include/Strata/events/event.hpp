#pragma once
#include <variant>
#include <optional>

namespace strata {

struct KeyEvent {
    int  key       = 0;
    bool ctrl      = false;
    bool alt       = false;
    bool shift     = false;
};

struct MouseEvent {
    enum class Button { Left, Right, Middle, ScrollUp, ScrollDown, None };
    enum class Kind   { Press, Release, Move };

    int    x      = 0;
    int    y      = 0;
    Button button = Button::None;
    Kind   kind   = Kind::Move;
};

struct ResizeEvent {
    int width  = 0;
    int height = 0;
};

using Event = std::variant<KeyEvent, MouseEvent, ResizeEvent>;

// Helper free functions — avoids boilerplate std::visit at every call site

inline bool is_key(const Event& e, int key) {
    if (const auto* k = std::get_if<KeyEvent>(&e))
        return k->key == key;
    return false;
}

// Cast through unsigned char to prevent sign-extension for chars >= 128
inline bool is_char(const Event& e, char c) {
    return is_key(e, static_cast<int>(static_cast<unsigned char>(c)));
}

inline const KeyEvent* as_key(const Event& e) {
    return std::get_if<KeyEvent>(&e);
}

inline const MouseEvent* as_mouse(const Event& e) {
    return std::get_if<MouseEvent>(&e);
}

inline const ResizeEvent* as_resize(const Event& e) {
    return std::get_if<ResizeEvent>(&e);
}

} // namespace strata
