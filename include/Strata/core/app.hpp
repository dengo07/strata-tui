#pragma once
#include "widget.hpp"
#include "container.hpp"
#include "alert_manager.hpp"
#include "../render/backend.hpp"
#include "../layout/constraint.hpp"
#include "../widgets/modal.hpp"
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace strata {

class FocusManager;

class App {
    // ── Event loop internals ─────────────────────────────────────────────────
    std::unique_ptr<Backend>      backend_;
    std::unique_ptr<Container>    root_;
    std::unique_ptr<FocusManager> focus_;
    bool running_ = false;

    // ── Alert / toast overlay ────────────────────────────────────────────────
    AlertManager alert_manager_;

    // ── Modal stack ──────────────────────────────────────────────────────────
    std::vector<std::unique_ptr<Modal>> modals_;
    std::vector<int>                    modal_ids_;
    int                                 next_modal_id_ = 1;

    void render_frame();
    void route_event(const Event& e);
    void mount_all(Widget* w);
    void unmount_all(Widget* w);
    Widget* find_focusable_at(Widget* w, int x, int y);

    // ── Timer system ─────────────────────────────────────────────────────────
    struct Timer {
        int id;
        std::chrono::steady_clock::time_point next;
        int interval_ms; // >0 = repeat every N ms; 0 = one-shot (removed after firing)
        std::function<void()> fn;
    };
    std::vector<Timer> timers_;
    int next_timer_id_ = 1;

    // Returns how many ms until the next timer fires (capped at 16 ms).
    int ms_until_next_timer() const;

    // Fires all timers whose deadline has passed; reschedules repeating ones.
    void fire_timers();

    // ── Async task queue ─────────────────────────────────────────────────────
    std::vector<std::function<void()>> pending_callbacks_;
    std::mutex                          pending_mutex_;
    std::atomic<bool>                   has_pending_{false};

    // Drains callbacks posted by background threads and runs them on this thread.
    void drain_pending();

public:
    // Pass a custom backend, or leave null to use NcursesBackend (default)
    explicit App(std::unique_ptr<Backend> backend = nullptr);
    ~App();

    // Construct widget in-place with Constraint::fill() by default
    template<typename W, typename... Args>
    W* add(Args&&... args) {
        return root_->add<W>(std::forward<Args>(args)...);
    }

    template<typename W, typename... Args>
    W* add(Constraint c, Args&&... args) {
        return root_->add<W>(c, std::forward<Args>(args)...);
    }

    Widget* add(std::unique_ptr<Widget> w, Constraint c = Constraint::fill());

    // Blocks until quit() is called
    void run();

    // Signal the event loop to exit cleanly
    void quit();

    Backend& backend() { return *backend_; }

    // ── Timer API ────────────────────────────────────────────────────────────

    // Calls fn on the main thread every `ms` milliseconds.
    // Returns a timer ID that can be passed to clear_timer().
    int set_interval(int ms, std::function<void()> fn);

    // Calls fn on the main thread once after `ms` milliseconds.
    // Returns a timer ID (cancel with clear_timer() before it fires if needed).
    int set_timeout(int ms, std::function<void()> fn);

    // Cancels a timer by ID (safe to call with an already-fired / invalid ID).
    void clear_timer(int id);

    // ── Async API ────────────────────────────────────────────────────────────

    // Runs bg_fn on a background thread. When it completes, on_done is queued
    // to run on the main thread (during the next loop iteration, ≤ 16 ms later).
    //
    // bg_fn must not touch any Strata widget — it runs on a separate thread.
    // on_done runs on the main thread and may freely call widget setters.
    //
    // NOTE: the App (and all captured widget pointers) must remain alive until
    // on_done executes. For long-running tasks, guard with a shared_ptr sentinel.
    void run_async(std::function<void()> bg_fn, std::function<void()> on_done = {});

    // ── Alert / toast notifications ───────────────────────────────────────────

    // Shows a toast notification in the bottom-right corner.
    // duration_ms = 0 means persistent (call dismiss_alert() to remove manually).
    // Returns an alert ID that can be passed to dismiss_alert().
    int notify(std::string title, std::string message = "",
               AlertManager::Level level = AlertManager::Level::Info,
               int duration_ms = 4000);

    // Dismisses a toast notification by ID.
    void dismiss_alert(int id);

    // ── Modal system ─────────────────────────────────────────────────────────

    // Opens a modal overlay. Focus is trapped inside the modal until it is closed.
    // Returns a modal ID that can be passed to close_modal().
    int  open_modal(std::unique_ptr<Modal> m);

    // Closes a modal by ID, restoring background focus.
    void close_modal(int id);

    // Returns true if at least one modal is currently open.
    bool has_modal() const;

    // ── Global event handler ─────────────────────────────────────────────────

    // Called after the focused widget's bubble-up chain if no widget consumed
    // the event.
    std::function<void(const Event&)> on_event;
};

} // namespace strata
