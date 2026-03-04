#pragma once
#include "../render/canvas.hpp"
#include "../style/style.hpp"
#include <string>
#include <vector>

namespace strata {

class AlertManager {
public:
    enum class Level { Info, Success, Warning, Error };

    // Adds a new alert and returns its ID.
    int add(std::string title, std::string message, Level level);

    // Dismisses an alert by ID.
    void dismiss(int id);

    // Called by App::render_frame() after the root widget render pass.
    void render(Canvas& canvas);

    bool is_dirty() const { return dirty_; }
    void clear_dirty() { dirty_ = false; }

private:
    struct Alert {
        int id;
        std::string title;
        std::string message;
        Level level;
    };

    std::vector<Alert> alerts_;
    bool dirty_ = false;
    int next_id_ = 0;

    static constexpr int kWidth = 36;
    static constexpr int kMaxMsgLines = 2;

    Style border_style_for(Level l) const;
    Style title_style_for(Level l) const;
    std::string level_label(Level l) const;
    static std::vector<std::string> wrap(const std::string& text, int width);
};

} // namespace strata
