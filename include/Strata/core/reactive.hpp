#pragma once
#include <functional>
#include <vector>
#include <algorithm>

namespace strata {

// ── Reactive<T> ──────────────────────────────────────────────────────────────
// Lightweight observable value. Wrap any type T; call set() or mutate() to
// update the value and automatically notify all registered listeners.
//
// Usage:
//   auto count = strata::make_state(0);
//   count.on_change([](){ redraw(); });
//   count.set(count.get() + 1);          // fires all listeners
//   count.mutate([](int& v){ v += 1; }); // same, via in-place mutation
//
template<typename T>
class Reactive {
public:
    explicit Reactive(T init = T{}) : value_(std::move(init)) {}

    // Read the current value.
    const T& get() const { return value_; }
    const T& operator*()  const { return value_; }
    const T* operator->() const { return &value_; }

    // Replace the value and fire all listeners.
    void set(T v) { value_ = std::move(v); fire(); }

    // Mutate the value in-place via a callable, then fire all listeners.
    // fn signature: void(T&)
    template<typename Fn>
    void mutate(Fn&& fn) { fn(value_); fire(); }

    // Register a change listener. Returns an integer id for later removal.
    int on_change(std::function<void()> cb) {
        int id = next_id_++;
        listeners_.push_back({id, std::move(cb)});
        return id;
    }

    // Unregister a listener by id.
    void off(int id) {
        listeners_.erase(
            std::remove_if(listeners_.begin(), listeners_.end(),
                [id](const Listener& l){ return l.id == id; }),
            listeners_.end());
    }

private:
    struct Listener { int id; std::function<void()> fn; };
    T                    value_;
    std::vector<Listener> listeners_;
    int                  next_id_ = 0;

    void fire() {
        // Copy list in case a listener mutates listeners_ (e.g. off())
        auto copy = listeners_;
        for (auto& l : copy) l.fn();
    }
};

// Factory: strata::make_state<int>(42) or strata::make_state(std::vector<T>{})
template<typename T>
Reactive<T> make_state(T init = T{}) { return Reactive<T>(std::move(init)); }

} // namespace strata
