#include "../../include/Strata/core/app.hpp"
#include "focus_manager.hpp"
#include "../render/ncurses_backend.hpp"
#include <algorithm>
#include <ncurses.h>  // for KEY_BTAB
#include <thread>

namespace strata {

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

App::App(std::unique_ptr<Backend> backend) {
    if (backend) {
        backend_ = std::move(backend);
    } else {
        backend_ = std::make_unique<NcursesBackend>();
    }
    root_  = std::make_unique<Container>(Layout(Layout::Direction::Vertical));
    focus_ = std::make_unique<FocusManager>();
}

App::~App() = default;

// ---------------------------------------------------------------------------
// Public interface
// ---------------------------------------------------------------------------

Widget* App::add(std::unique_ptr<Widget> w, Constraint main, Constraint cross) {
    return root_->add(std::move(w), main, cross);
}

void App::quit() {
    running_ = false;
}

// ---------------------------------------------------------------------------
// DFS mount / unmount
// ---------------------------------------------------------------------------

void App::mount_all(Widget* w) {
    w->on_mount();
    w->for_each_child([this](Widget& child) {
        mount_all(&child);
    });
}

void App::unmount_all(Widget* w) {
    w->for_each_child([this](Widget& child) {
        unmount_all(&child);
    });
    w->on_unmount();
}

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------

void App::render_frame() {
    bool modal_dirty = std::any_of(modals_.begin(), modals_.end(),
        [](const auto& m){ return m->is_dirty(); });
    bool any_dirty = root_->is_dirty() || alert_manager_.is_dirty() || modal_dirty;
    if (!any_dirty) return;

    backend_->clear();
    Rect screen{0, 0, backend_->width(), backend_->height()};
    root_->last_rect_ = screen;
    Canvas canvas(*backend_, screen);
    root_->render(canvas);
    // Render modals in stack order (bottom to top)
    for (auto& m : modals_) {
        m->render(canvas);
        m->dirty_ = false;
    }
    alert_manager_.render(canvas);   // overlay pass — draws on top of everything
    alert_manager_.clear_dirty();
    backend_->flush();

    // Re-mark root dirty if any widget requested continuous rendering (e.g. Spinner auto-animate)
    if (Widget::s_needs_rerender_) {
        Widget::s_needs_rerender_ = false;
        root_->mark_dirty();
    }
}

// ---------------------------------------------------------------------------
// Alert / toast notifications
// ---------------------------------------------------------------------------

int App::notify(std::string title, std::string message,
                AlertManager::Level level, int duration_ms) {
    int id = alert_manager_.add(std::move(title), std::move(message), level);
    if (duration_ms > 0) {
        set_timeout(duration_ms, [this, id]() { dismiss_alert(id); });
    }
    return id;
}

void App::dismiss_alert(int id) {
    alert_manager_.dismiss(id);
}

// ---------------------------------------------------------------------------
// Modal system
// ---------------------------------------------------------------------------

int App::open_modal(std::unique_ptr<Modal> m) {
    int id = next_modal_id_++;
    m->parent_ = nullptr; // not part of the widget tree
    if (m->inner()) mount_all(m->inner());
    focus_->push_scope(m->inner());
    root_->mark_dirty(); // force a frame to render the new modal
    modals_.push_back(std::move(m));
    modal_ids_.push_back(id);
    return id;
}

void App::close_modal(int id) {
    for (std::size_t i = 0; i < modal_ids_.size(); ++i) {
        if (modal_ids_[i] == id) {
            if (modals_[i]->inner()) unmount_all(modals_[i]->inner());
            modals_.erase(modals_.begin() + i);
            modal_ids_.erase(modal_ids_.begin() + i);
            focus_->pop_scope();
            root_->mark_dirty();
            return;
        }
    }
}

bool App::has_modal() const {
    return !modals_.empty();
}

// ---------------------------------------------------------------------------
// Mouse hit-test
// ---------------------------------------------------------------------------

Widget* App::find_focusable_at(Widget* w, int x, int y) {
    Widget* best = nullptr;
    if (w->last_rect_.contains(x, y) && w->is_focusable())
        best = w;
    w->for_each_child([&](Widget& child) {
        Widget* found = find_focusable_at(&child, x, y);
        if (found) best = found; // deepest focusable wins
    });
    return best;
}

// ---------------------------------------------------------------------------
// Event routing (YENİ GRUP MANTIĞI BURADA)
// ---------------------------------------------------------------------------

void App::route_event(const Event& e) {
    // 0. If a modal is open, route only to the modal (focus is already trapped inside)
    if (has_modal()) {
        Modal* top = modals_.back().get();
        // Tab / Shift-Tab cycle through the modal's focusable widgets
        if (is_key(e, '\t'))    { focus_->focus_next(); return; }
        if (is_key(e, KEY_BTAB)) { focus_->focus_prev(); return; }
        Widget* target = focus_->focused();
        while (target) {
            if (target->handle_event(e)) return;
            target = target->parent();
        }
        // Give the modal itself a chance (handles Escape)
        top->handle_event(e);
        return;
    }

    // 1. Tab / Shift-Tab GRUPLAR ARASI atlar
    if (is_key(e, '\t')) {
        focus_->focus_next_group();
        return;
    }
    if (is_key(e, KEY_BTAB)) {
        focus_->focus_prev_group();
        return;
    }

    // 2. Mouse press: focus the deepest focusable widget under the cursor
    if (const auto* me = as_mouse(e)) {
        if (me->kind == MouseEvent::Kind::Press) {
            Widget* target = find_focusable_at(root_.get(), me->x, me->y);
            if (target) {
                focus_->focus(target);
                target->handle_event(e);
                return;
            }
        }
    }

    // 3. Route to focused widget first, then bubble up through parents
    Widget* target = focus_->focused();
    while (target != nullptr) {
        if (target->handle_event(e)) return; // consumed
        target = target->parent();
    }

    // 4. KİMSE YAKALAMADIYSA: Ok tuşlarıyla GRUP İÇİ gezinme
    if (is_key(e, 258) || is_char(e, 'j')) { focus_->focus_next_local(); return; }
    if (is_key(e, 259) || is_char(e, 'k')) { focus_->focus_prev_local(); return; }

    // 5. Fallback: global handler registered by user code
    if (on_event) on_event(e);
}

// ---------------------------------------------------------------------------
// Timer system
// ---------------------------------------------------------------------------

int App::set_interval(int ms, std::function<void()> fn) {
    int id = next_timer_id_++;
    timers_.push_back({
        id,
        std::chrono::steady_clock::now() + std::chrono::milliseconds(ms),
        ms,
        std::move(fn)
    });
    return id;
}

int App::set_timeout(int ms, std::function<void()> fn) {
    int id = next_timer_id_++;
    timers_.push_back({
        id,
        std::chrono::steady_clock::now() + std::chrono::milliseconds(ms),
        0, // one-shot
        std::move(fn)
    });
    return id;
}

void App::clear_timer(int id) {
    timers_.erase(
        std::remove_if(timers_.begin(), timers_.end(),
            [id](const Timer& t){ return t.id == id; }),
        timers_.end());
}

int App::ms_until_next_timer() const {
    if (timers_.empty()) return 16;
    auto now = std::chrono::steady_clock::now();
    int min_ms = 16;
    for (const auto& t : timers_) {
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t.next - now).count();
        if (ms < min_ms) min_ms = static_cast<int>(ms);
    }
    return std::max(0, min_ms);
}

void App::fire_timers() {
    auto now = std::chrono::steady_clock::now();
    // Iterate by index — fn() may call set_interval/clear_timer, modifying timers_
    for (std::size_t i = 0; i < timers_.size(); ) {
        if (timers_[i].next <= now) {
            timers_[i].fn();
            if (timers_[i].interval_ms > 0) {
                // Reschedule: advance by interval (catches up if we ran late)
                timers_[i].next += std::chrono::milliseconds(timers_[i].interval_ms);
                ++i;
            } else {
                // One-shot: remove by swap-and-pop (order doesn't matter here)
                timers_[i] = std::move(timers_.back());
                timers_.pop_back();
                // Don't increment i — the swapped element needs checking too
            }
        } else {
            ++i;
        }
    }
}

// ---------------------------------------------------------------------------
// Async task queue
// ---------------------------------------------------------------------------

void App::run_async(std::function<void()> bg_fn, std::function<void()> on_done) {
    std::thread([this,
                 bg   = std::move(bg_fn),
                 done = std::move(on_done)]() mutable {
        bg();
        if (done) {
            std::lock_guard<std::mutex> lock(pending_mutex_);
            pending_callbacks_.push_back(std::move(done));
            has_pending_.store(true, std::memory_order_release);
        }
    }).detach();
}

void App::drain_pending() {
    if (!has_pending_.load(std::memory_order_acquire)) return;
    std::vector<std::function<void()>> callbacks;
    {
        std::lock_guard<std::mutex> lock(pending_mutex_);
        callbacks.swap(pending_callbacks_);
        has_pending_.store(false, std::memory_order_release);
    }
    for (auto& fn : callbacks) fn();
}

// ---------------------------------------------------------------------------
// Main loop
// ---------------------------------------------------------------------------

void App::run() {
    backend_->init();

    mount_all(root_.get());
    focus_->rebuild(root_.get());

    root_->mark_dirty();
    running_ = true;
    render_frame();

    while (running_) {
        // Poll with a timeout no longer than needed for the next timer tick.
        // This lets timers fire on time without busy-waiting.
        int poll_ms = ms_until_next_timer();
        auto event = backend_->poll_event(poll_ms);

        if (event) {
            if (const auto* re = as_resize(*event)) {
                (void)re;
                root_->mark_dirty();
            } else {
                route_event(*event);
            }
        }

        // Fire any timers that have come due since the last iteration.
        fire_timers();

        // Run any callbacks posted by background threads.
        drain_pending();

        render_frame();
    }

    unmount_all(root_.get());
    backend_->shutdown();
}

} // namespace strata