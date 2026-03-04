#include "focus_manager.hpp"
#include "../../include/Strata/core/widget.hpp"
#include <algorithm>

namespace strata {

void FocusManager::collect(Widget* w) {
    if (w->is_focusable()) {
        tab_order_.push_back(w);
    }
    w->for_each_child([this](Widget& child) {
        collect(&child);
    });
}

void FocusManager::rebuild(Widget* root) {
    // Clear current focus state before rebuild
    if (current_idx_ >= 0 && current_idx_ < static_cast<int>(tab_order_.size())) {
        tab_order_[current_idx_]->focused_ = false;
    }
    tab_order_.clear();
    current_idx_ = -1;

    if (root) collect(root);

    // Sort by tab_index (stable preserves DFS order among equal values)
    std::stable_sort(tab_order_.begin(), tab_order_.end(),
        [](Widget* a, Widget* b) { return a->tab_index < b->tab_index; });

    // Focus first widget if any exist
    if (!tab_order_.empty()) {
        apply_focus(0);
    }
}

void FocusManager::apply_focus(int idx) {
    // Blur previous
    if (current_idx_ >= 0 && current_idx_ < static_cast<int>(tab_order_.size())) {
        tab_order_[current_idx_]->focused_ = false;
        tab_order_[current_idx_]->on_blur();
    }
    current_idx_ = idx;
    if (current_idx_ >= 0 && current_idx_ < static_cast<int>(tab_order_.size())) {
        tab_order_[current_idx_]->focused_ = true;
        tab_order_[current_idx_]->on_focus();
    }
}

void FocusManager::focus_next() {
    if (tab_order_.empty()) return;
    int n = static_cast<int>(tab_order_.size());
    apply_focus((current_idx_ + 1) % n);
}

void FocusManager::focus_prev() {
    if (tab_order_.empty()) return;
    int n = static_cast<int>(tab_order_.size());
    apply_focus((current_idx_ - 1 + n) % n);
}


void FocusManager::focus_next_group() {
    if (tab_order_.empty() || current_idx_ < 0) return;
    int n = static_cast<int>(tab_order_.size());
    std::string current_group = tab_order_[current_idx_]->focus_group();

    // If current widget has no group, behave like focus_next()
    if (current_group.empty()) { focus_next(); return; }

    int next_idx = (current_idx_ + 1) % n;
    while (next_idx != current_idx_) {
        if (tab_order_[next_idx]->focus_group() != current_group) {
            apply_focus(next_idx);
            return;
        }
        next_idx = (next_idx + 1) % n;
    }
}

void FocusManager::focus_prev_group() {
    if (tab_order_.empty() || current_idx_ < 0) return;
    int n = static_cast<int>(tab_order_.size());
    std::string current_group = tab_order_[current_idx_]->focus_group();

    // If current widget has no group, behave like focus_prev()
    if (current_group.empty()) { focus_prev(); return; }

    int prev_idx = (current_idx_ - 1 + n) % n;
    while (prev_idx != current_idx_) {
        if (tab_order_[prev_idx]->focus_group() != current_group) {
            std::string target_group = tab_order_[prev_idx]->focus_group();
            if (!target_group.empty()) {
                while (prev_idx > 0 && tab_order_[prev_idx - 1]->focus_group() == target_group) {
                    prev_idx--;
                }
            }
            apply_focus(prev_idx);
            return;
        }
        prev_idx = (prev_idx - 1 + n) % n;
    }
}

void FocusManager::focus_next_local() {
    if (tab_order_.empty() || current_idx_ < 0) return;
    int n = static_cast<int>(tab_order_.size());
    std::string current_group = tab_order_[current_idx_]->focus_group();

    if (current_group.empty()) { focus_next(); return; }

    int next_idx = (current_idx_ + 1) % n;
    if (tab_order_[next_idx]->focus_group() == current_group) {
        apply_focus(next_idx);
    } else {
        // At end of group — wrap to the group's first element
        int start_idx = current_idx_;
        while (start_idx > 0 && tab_order_[start_idx - 1]->focus_group() == current_group) {
            start_idx--;
        }
        apply_focus(start_idx);
    }
}

void FocusManager::focus_prev_local() {
    if (tab_order_.empty() || current_idx_ < 0) return;
    int n = static_cast<int>(tab_order_.size());
    std::string current_group = tab_order_[current_idx_]->focus_group();

    if (current_group.empty()) { focus_prev(); return; }

    int prev_idx = (current_idx_ - 1 + n) % n;
    if (prev_idx < current_idx_ && tab_order_[prev_idx]->focus_group() == current_group) {
        apply_focus(prev_idx);
    } else {
        // At start of group — wrap to the group's last element
        int end_idx = current_idx_;
        while (end_idx < n - 1 && tab_order_[end_idx + 1]->focus_group() == current_group) {
            end_idx++;
        }
        apply_focus(end_idx);
    }
}

void FocusManager::focus(Widget* w) {
    for (int i = 0; i < static_cast<int>(tab_order_.size()); ++i) {
        if (tab_order_[i] == w) {
            apply_focus(i);
            return;
        }
    }
}

Widget* FocusManager::focused() const {
    if (current_idx_ < 0 || current_idx_ >= static_cast<int>(tab_order_.size()))
        return nullptr;
    return tab_order_[current_idx_];
}

void FocusManager::push_scope(Widget* root) {
    // Save current state
    scope_stack_.push_back({tab_order_, current_idx_});

    // Blur currently focused widget
    if (current_idx_ >= 0 && current_idx_ < static_cast<int>(tab_order_.size())) {
        tab_order_[current_idx_]->focused_ = false;
        tab_order_[current_idx_]->on_blur();
    }

    // Rebuild from the new root (modal inner widget)
    tab_order_.clear();
    current_idx_ = -1;
    if (root) collect(root);

    std::stable_sort(tab_order_.begin(), tab_order_.end(),
        [](Widget* a, Widget* b) { return a->tab_index < b->tab_index; });

    if (!tab_order_.empty()) apply_focus(0);
}

void FocusManager::pop_scope() {
    if (scope_stack_.empty()) return;

    // Blur modal's focused widget
    if (current_idx_ >= 0 && current_idx_ < static_cast<int>(tab_order_.size())) {
        tab_order_[current_idx_]->focused_ = false;
        tab_order_[current_idx_]->on_blur();
    }

    // Restore saved state
    auto saved = std::move(scope_stack_.back());
    scope_stack_.pop_back();
    tab_order_   = std::move(saved.tab_order);
    current_idx_ = saved.idx;

    // Re-apply focus to the restored widget
    if (current_idx_ >= 0 && current_idx_ < static_cast<int>(tab_order_.size())) {
        tab_order_[current_idx_]->focused_ = true;
        tab_order_[current_idx_]->on_focus();
    }
}

} // namespace strata
