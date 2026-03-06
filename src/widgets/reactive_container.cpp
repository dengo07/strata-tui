#include "../../include/Strata/widgets/reactive_container.hpp"

namespace strata {

ReactiveContainer::ReactiveContainer(Layout layout) : layout_(layout) {}

void ReactiveContainer::add(std::unique_ptr<Widget> w, Constraint c) {
    w->parent_ = this;
    children_.push_back(std::move(w));
    constraints_.push_back(c);
}

void ReactiveContainer::set_rebuild(std::function<void()> fn) {
    rebuild_fn_ = std::move(fn);
}
void ReactiveContainer::set_subscribe(std::function<int(std::function<void()>)> fn) {
    subscribe_fn_ = std::move(fn);
}
void ReactiveContainer::set_unsubscribe(std::function<void(int)> fn) {
    unsubscribe_fn_ = std::move(fn);
}

void ReactiveContainer::on_mount() {
    // Populate initial children. App::mount_all() will then DFS-mount them
    // via for_each_child() after on_mount() returns.
    if (rebuild_fn_) rebuild_fn_();
    // Subscribe so future reactive changes trigger refresh().
    if (subscribe_fn_)
        listener_id_ = subscribe_fn_([this]() { refresh(); });
}

void ReactiveContainer::on_unmount() {
    if (unsubscribe_fn_ && listener_id_ >= 0) {
        unsubscribe_fn_(listener_id_);
        listener_id_ = -1;
    }
}

void ReactiveContainer::refresh() {
    // 1. Unmount old children (no focus rebuild yet).
    if (Widget::s_on_subtree_removed_)
        for (auto& child : children_) Widget::s_on_subtree_removed_(child.get());
    children_.clear();
    constraints_.clear();

    // 2. Rebuild: rebuild_fn_ calls add() to populate new children.
    if (rebuild_fn_) rebuild_fn_();

    // 3. Mount new children (mount-only, no per-child focus rebuild).
    if (mounted_ && Widget::s_on_mount_subtree_)
        for (auto& child : children_) Widget::s_on_mount_subtree_(child.get());

    // 4. Rebuild focus once on the clean tree.
    if (mounted_ && Widget::s_on_focus_rebuild_) Widget::s_on_focus_rebuild_();

    mark_dirty();
}

void ReactiveContainer::render(Canvas& canvas) {
    dirty_ = false;
    if (children_.empty()) return;

    auto rects = layout_.split(canvas.area(), constraints_);
    for (std::size_t i = 0; i < children_.size(); ++i) {
        if (i >= rects.size() || rects[i].empty()) continue;
        children_[i]->last_rect_ = rects[i];
        Canvas child_canvas = canvas.sub_canvas(rects[i]);
        children_[i]->render(child_canvas);
        children_[i]->dirty_ = false;
    }
}

void ReactiveContainer::for_each_child(const std::function<void(Widget&)>& visitor) {
    for (auto& child : children_) visitor(*child);
}

} // namespace strata
