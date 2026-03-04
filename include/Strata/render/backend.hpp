#pragma once
#include "../style/cell.hpp"
#include "../events/event.hpp"
#include <optional>

namespace strata {

class Backend {
public:
    virtual void init()     = 0;
    virtual void shutdown() = 0;

    virtual int width()  const = 0;
    virtual int height() const = 0;

    virtual void draw_cell(int x, int y, const Cell& cell) = 0;
    virtual void clear()  = 0;
    virtual void flush()  = 0;

    // Returns nullopt on timeout (no event within timeout_ms milliseconds)
    virtual std::optional<Event> poll_event(int timeout_ms = 16) = 0;

    virtual ~Backend() = default;

protected:
    // Terminal state is a singleton resource — prevent copies
    Backend() = default;
    Backend(const Backend&) = delete;
    Backend& operator=(const Backend&) = delete;
    Backend(Backend&&) = delete;
    Backend& operator=(Backend&&) = delete;
};

} // namespace strata
