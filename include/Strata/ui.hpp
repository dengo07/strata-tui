#pragma once
// strata::ui — declarative DSL layer for Strata.
//
// Usage:
//   #include <Strata/ui.hpp>
//   using namespace strata::ui;
//   App app;
//   populate(app, { Row{ ... }, Col{ ... }, ... });
//
// All descriptor types are header-only and build their widget trees lazily
// when populate() (or build()) is called.

#include <Strata/strata.hpp>
#include <functional>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace strata::ui {

// ── Re-exports ────────────────────────────────────────────────────────────────
using strata::App;
using strata::Style;
using strata::Constraint;
using strata::Layout;
using strata::Event;
using strata::is_char;
using strata::is_key;
using strata::as_key;
using strata::as_mouse;
namespace color = strata::color;

// AlertManager::Level convenience alias for DSL users
using AlertLevel = strata::AlertManager::Level;

// ── Constraint helpers ────────────────────────────────────────────────────────
inline strata::Constraint fixed(int n)      { return strata::Constraint::fixed(n); }
inline strata::Constraint fill(int w = 1)   { return strata::Constraint::fill(w); }
inline strata::Constraint min(int n)        { return strata::Constraint::min(n); }
inline strata::Constraint max(int n)        { return strata::Constraint::max(n); }
inline strata::Constraint percentage(int p) { return strata::Constraint::percentage(p); }

// ── Node ──────────────────────────────────────────────────────────────────────
// Type-erased wrapper for any descriptor. Copyable via shared_ptr<Concept>.
// Required because std::initializer_list<Node> provides const Node& which
// requires copyability.
class Node {
    struct Concept {
        virtual std::unique_ptr<strata::Widget> build() const = 0;
        virtual strata::Constraint constraint() const = 0;
        virtual strata::Constraint cross_constraint() const = 0;
        virtual ~Concept() = default;
    };
    template<typename T>
    struct Model : Concept {
        T val;
        explicit Model(T v) : val(std::move(v)) {}
        std::unique_ptr<strata::Widget> build() const override { return val.build(); }
        strata::Constraint constraint() const override { return val.constraint(); }
        strata::Constraint cross_constraint() const override { return val.cross_constraint(); }
    };
    std::shared_ptr<Concept> impl_;
public:
    template<typename T,
             std::enable_if_t<!std::is_same_v<std::decay_t<T>, Node>, int> = 0>
    Node(T t) : impl_(std::make_shared<Model<std::decay_t<T>>>(std::move(t))) {}

    std::unique_ptr<strata::Widget> build() const { return impl_->build(); }
    strata::Constraint constraint() const { return impl_->constraint(); }
    strata::Constraint cross_constraint() const { return impl_->cross_constraint(); }
};

// ── populate ──────────────────────────────────────────────────────────────────
// Add all top-level nodes to the App's root container.
inline void populate(strata::App& app, std::initializer_list<Node> nodes) {
    for (const Node& n : nodes)
        app.add(n.build(), n.constraint(), n.cross_constraint());
}

// ── Col ───────────────────────────────────────────────────────────────────────
// Vertical Container descriptor.
class Col {
    std::vector<Node>       children_;
    strata::Constraint      size_        = strata::Constraint::fill();
    strata::Constraint      cross_       = strata::Constraint::fill();
    strata::Layout::Justify justify_     = strata::Layout::Justify::Start;
    strata::Layout::Align   cross_align_ = strata::Layout::Align::Start;
    int                     gap_         = 0;
public:
    explicit Col(std::initializer_list<Node> children) : children_(children) {}

    Col& size(strata::Constraint c)              { size_        = c; return *this; }
    Col& cross(strata::Constraint c)             { cross_       = c; return *this; }
    Col& justify(strata::Layout::Justify j)      { justify_     = j; return *this; }
    Col& cross_align(strata::Layout::Align a)    { cross_align_ = a; return *this; }
    Col& gap(int g)                              { gap_         = g; return *this; }

    std::unique_ptr<strata::Widget> build() const {
        auto c = std::make_unique<strata::Container>(
            strata::Layout(strata::Layout::Direction::Vertical)
                .set_justify(justify_)
                .set_gap(gap_));
        c->set_cross_align(cross_align_);
        for (const Node& n : children_)
            c->add(n.build(), n.constraint(), n.cross_constraint());
        return c;
    }
    strata::Constraint constraint() const       { return size_; }
    strata::Constraint cross_constraint() const { return cross_; }
};

// ── Row ───────────────────────────────────────────────────────────────────────
// Horizontal Container descriptor.
class Row {
    std::vector<Node>       children_;
    strata::Constraint      size_        = strata::Constraint::fill();
    strata::Constraint      cross_       = strata::Constraint::fill();
    strata::Layout::Justify justify_     = strata::Layout::Justify::Start;
    strata::Layout::Align   cross_align_ = strata::Layout::Align::Start;
    int                     gap_         = 0;
public:
    explicit Row(std::initializer_list<Node> children) : children_(children) {}

    Row& size(strata::Constraint c)              { size_        = c; return *this; }
    Row& cross(strata::Constraint c)             { cross_       = c; return *this; }
    Row& justify(strata::Layout::Justify j)      { justify_     = j; return *this; }
    Row& cross_align(strata::Layout::Align a)    { cross_align_ = a; return *this; }
    Row& gap(int g)                              { gap_         = g; return *this; }

    std::unique_ptr<strata::Widget> build() const {
        auto c = std::make_unique<strata::Container>(
            strata::Layout(strata::Layout::Direction::Horizontal)
                .set_justify(justify_)
                .set_gap(gap_));
        c->set_cross_align(cross_align_);
        for (const Node& n : children_)
            c->add(n.build(), n.constraint(), n.cross_constraint());
        return c;
    }
    strata::Constraint constraint() const       { return size_; }
    strata::Constraint cross_constraint() const { return cross_; }
};

// ── Block ─────────────────────────────────────────────────────────────────────
// Bordered-box descriptor wrapping one inner Node.
class Block {
    std::string             title_;
    std::optional<Node>     inner_;
    strata::Style           border_style_;
    strata::Style           title_style_;
    strata::Style           focused_border_style_;
    strata::Style           focused_title_style_;
    strata::Constraint      size_  = strata::Constraint::fill();
    strata::Constraint      cross_ = strata::Constraint::fill();
    mutable strata::Block** ref_   = nullptr;
public:
    explicit Block(std::string title = "", std::optional<Node> inner = std::nullopt)
        : title_(std::move(title)), inner_(std::move(inner)) {}

    Block& size(strata::Constraint c)       { size_                 = c; return *this; }
    Block& cross(strata::Constraint c)      { cross_                = c; return *this; }
    Block& border(strata::Style s)          { border_style_         = s; return *this; }
    Block& title_style(strata::Style s)     { title_style_          = s; return *this; }
    Block& focused_border(strata::Style s)  { focused_border_style_ = s; return *this; }
    Block& focused_title(strata::Style s)   { focused_title_style_  = s; return *this; }
    Block& inner(Node n)                    { inner_ = std::move(n); return *this; }

    Block& styles(strata::Style border_s, strata::Style title_s,
                  strata::Style foc_border, strata::Style foc_title) {
        border_style_         = border_s;
        title_style_          = title_s;
        focused_border_style_ = foc_border;
        focused_title_style_  = foc_title;
        return *this;
    }

    Block& bind(strata::Block*& ref) { ref_ = &ref; return *this; }

    std::unique_ptr<strata::Widget> build() const {
        static const strata::Style kDefault{};
        auto w = std::make_unique<strata::Block>();
        w->set_title(title_);
        if (border_style_         != kDefault) w->set_border_style(border_style_);
        if (title_style_          != kDefault) w->set_title_style(title_style_);
        // Only call set_focused_*_style when non-default — they set has_focused_styles_
        if (focused_border_style_ != kDefault) w->set_focused_border_style(focused_border_style_);
        if (focused_title_style_  != kDefault) w->set_focused_title_style(focused_title_style_);
        if (inner_) w->set_inner(inner_->build());
        if (ref_) *ref_ = w.get();
        return w;
    }
    strata::Constraint constraint() const       { return size_; }
    strata::Constraint cross_constraint() const { return cross_; }
};

// ── Label ─────────────────────────────────────────────────────────────────────
class Label {
    std::string        text_;
    strata::Style      style_;
    bool               wrap_  = false;
    strata::Constraint size_  = strata::Constraint::fill();
    strata::Constraint cross_ = strata::Constraint::fill();
    mutable strata::Label** ref_ = nullptr;
public:
    explicit Label(std::string text = "") : text_(std::move(text)) {}

    Label& style(strata::Style s)      { style_ = s;    return *this; }
    Label& wrap(bool w)                { wrap_  = w;    return *this; }
    Label& size(strata::Constraint c)  { size_  = c;    return *this; }
    Label& cross(strata::Constraint c) { cross_ = c;    return *this; }
    Label& bind(strata::Label*& ref)   { ref_   = &ref; return *this; }

    std::unique_ptr<strata::Widget> build() const {
        auto w = std::make_unique<strata::Label>(text_, style_);
        w->set_wrap(wrap_);
        if (ref_) *ref_ = w.get();
        return w;
    }
    strata::Constraint constraint() const       { return size_; }
    strata::Constraint cross_constraint() const { return cross_; }
};

// ── Button ────────────────────────────────────────────────────────────────────
class Button {
    std::string              label_;
    strata::Style            style_;
    strata::Style            focused_style_;
    strata::Constraint       size_      = strata::Constraint::fill();
    strata::Constraint       cross_     = strata::Constraint::fill();
    std::function<void()>    on_click_;
    int                      tab_index_ = 0;
    std::string              focus_group_;
    mutable strata::Button** ref_       = nullptr;
public:
    explicit Button(std::string label) : label_(std::move(label)) {}

    Button& style(strata::Style s)          { style_         = s;            return *this; }
    Button& focused_style(strata::Style s)  { focused_style_ = s;            return *this; }
    Button& size(strata::Constraint c)      { size_          = c;            return *this; }
    Button& cross(strata::Constraint c)     { cross_         = c;            return *this; }
    Button& click(std::function<void()> f)  { on_click_      = std::move(f); return *this; }
    Button& tab_index(int i)                { tab_index_     = i;            return *this; }
    Button& group(std::string g)            { focus_group_   = std::move(g); return *this; }
    Button& bind(strata::Button*& ref)      { ref_           = &ref;         return *this; }

    std::unique_ptr<strata::Widget> build() const {
        static const strata::Style kDefault{};
        auto w = std::make_unique<strata::Button>(label_);
        if (style_         != kDefault) w->set_style(style_);
        if (focused_style_ != kDefault) w->set_focused_style(focused_style_);
        if (on_click_) w->on_click = on_click_;
        w->tab_index = tab_index_;
        if (!focus_group_.empty()) w->set_focus_group(focus_group_);
        if (ref_) *ref_ = w.get();
        return w;
    }
    strata::Constraint constraint() const       { return size_; }
    strata::Constraint cross_constraint() const { return cross_; }
};

// ── Checkbox ──────────────────────────────────────────────────────────────────
class Checkbox {
    std::string                  label_;
    bool                         checked_   = false;
    strata::Constraint           size_      = strata::Constraint::fill();
    strata::Constraint           cross_     = strata::Constraint::fill();
    std::function<void(bool)>    on_change_;
    int                          tab_index_ = 0;
    std::string                  focus_group_;
    mutable strata::Checkbox**   ref_       = nullptr;
public:
    explicit Checkbox(std::string label) : label_(std::move(label)) {}

    Checkbox& checked(bool c)                      { checked_   = c;            return *this; }
    Checkbox& size(strata::Constraint c)           { size_      = c;            return *this; }
    Checkbox& cross(strata::Constraint c)          { cross_     = c;            return *this; }
    Checkbox& change(std::function<void(bool)> f)  { on_change_ = std::move(f); return *this; }
    Checkbox& tab_index(int i)                     { tab_index_ = i;            return *this; }
    Checkbox& group(std::string g)                 { focus_group_ = std::move(g); return *this; }
    Checkbox& bind(strata::Checkbox*& ref)         { ref_       = &ref;         return *this; }

    std::unique_ptr<strata::Widget> build() const {
        auto w = std::make_unique<strata::Checkbox>(label_, checked_);
        if (on_change_) w->on_change = on_change_;
        w->tab_index = tab_index_;
        if (!focus_group_.empty()) w->set_focus_group(focus_group_);
        if (ref_) *ref_ = w.get();
        return w;
    }
    strata::Constraint constraint() const       { return size_; }
    strata::Constraint cross_constraint() const { return cross_; }
};

// ── Switch ────────────────────────────────────────────────────────────────────
class Switch {
    std::string                  label_;
    bool                         on_        = false;
    strata::Constraint           size_      = strata::Constraint::fill();
    strata::Constraint           cross_     = strata::Constraint::fill();
    std::function<void(bool)>    on_change_;
    int                          tab_index_ = 0;
    std::string                  focus_group_;
    mutable strata::Switch**     ref_       = nullptr;
public:
    explicit Switch(std::string label) : label_(std::move(label)) {}

    Switch& on(bool v)                             { on_        = v;            return *this; }
    Switch& size(strata::Constraint c)             { size_      = c;            return *this; }
    Switch& cross(strata::Constraint c)            { cross_     = c;            return *this; }
    Switch& change(std::function<void(bool)> f)    { on_change_ = std::move(f); return *this; }
    Switch& tab_index(int i)                       { tab_index_ = i;            return *this; }
    Switch& group(std::string g)                   { focus_group_ = std::move(g); return *this; }
    Switch& bind(strata::Switch*& ref)             { ref_       = &ref;         return *this; }

    std::unique_ptr<strata::Widget> build() const {
        auto w = std::make_unique<strata::Switch>(label_, on_);
        if (on_change_) w->on_change = on_change_;
        w->tab_index = tab_index_;
        if (!focus_group_.empty()) w->set_focus_group(focus_group_);
        if (ref_) *ref_ = w.get();
        return w;
    }
    strata::Constraint constraint() const       { return size_; }
    strata::Constraint cross_constraint() const { return cross_; }
};

// ── Input ─────────────────────────────────────────────────────────────────────
class Input {
    std::string                              placeholder_;
    std::string                              value_;
    strata::Constraint                       size_      = strata::Constraint::fill();
    strata::Constraint                       cross_     = strata::Constraint::fill();
    std::function<void(const std::string&)>  on_submit_;
    std::function<void(const std::string&)>  on_change_;
    int                                      tab_index_ = 0;
    std::string                              focus_group_;
    mutable strata::Input**                  ref_       = nullptr;
public:
    Input() = default;

    Input& placeholder(std::string s)                         { placeholder_ = std::move(s); return *this; }
    Input& value(std::string s)                               { value_       = std::move(s); return *this; }
    Input& size(strata::Constraint c)                         { size_        = c;            return *this; }
    Input& cross(strata::Constraint c)                        { cross_       = c;            return *this; }
    Input& submit(std::function<void(const std::string&)> f)  { on_submit_   = std::move(f); return *this; }
    Input& change(std::function<void(const std::string&)> f)  { on_change_   = std::move(f); return *this; }
    Input& tab_index(int i)                                   { tab_index_   = i;            return *this; }
    Input& group(std::string g)                               { focus_group_ = std::move(g); return *this; }
    Input& bind(strata::Input*& ref)                          { ref_         = &ref;         return *this; }

    std::unique_ptr<strata::Widget> build() const {
        auto w = std::make_unique<strata::Input>();
        if (!placeholder_.empty()) w->set_placeholder(placeholder_);
        if (!value_.empty())       w->set_value(value_);
        if (on_submit_)            w->on_submit = on_submit_;
        if (on_change_)            w->on_change = on_change_;
        w->tab_index = tab_index_;
        if (!focus_group_.empty()) w->set_focus_group(focus_group_);
        if (ref_) *ref_ = w.get();
        return w;
    }
    strata::Constraint constraint() const       { return size_; }
    strata::Constraint cross_constraint() const { return cross_; }
};

// ── Select ────────────────────────────────────────────────────────────────────
class Select {
    std::vector<std::string>                     items_;
    int                                          selected_  = 0;
    strata::Constraint                           size_      = strata::Constraint::fill();
    strata::Constraint                           cross_     = strata::Constraint::fill();
    std::function<void(int, const std::string&)> on_change_;
    int                                          tab_index_ = 0;
    std::string                                  focus_group_;
    mutable strata::Select**                     ref_       = nullptr;
public:
    Select() = default;

    Select& items(std::vector<std::string> v)                       { items_     = std::move(v); return *this; }
    Select& selected(int i)                                         { selected_  = i;            return *this; }
    Select& size(strata::Constraint c)                              { size_      = c;            return *this; }
    Select& cross(strata::Constraint c)                             { cross_     = c;            return *this; }
    Select& change(std::function<void(int, const std::string&)> f)  { on_change_ = std::move(f); return *this; }
    Select& tab_index(int i)                                        { tab_index_ = i;            return *this; }
    Select& group(std::string g)                                    { focus_group_ = std::move(g); return *this; }
    Select& bind(strata::Select*& ref)                              { ref_       = &ref;         return *this; }

    std::unique_ptr<strata::Widget> build() const {
        auto w = std::make_unique<strata::Select>();
        if (!items_.empty()) w->set_items(items_).set_selected(selected_);
        if (on_change_) w->on_change = on_change_;
        w->tab_index = tab_index_;
        if (!focus_group_.empty()) w->set_focus_group(focus_group_);
        if (ref_) *ref_ = w.get();
        return w;
    }
    strata::Constraint constraint() const       { return size_; }
    strata::Constraint cross_constraint() const { return cross_; }
};

// ── RadioGroup ────────────────────────────────────────────────────────────────
class RadioGroup {
    std::vector<std::string>                     items_;
    int                                          selected_  = 0;
    strata::Constraint                           size_      = strata::Constraint::fill();
    strata::Constraint                           cross_     = strata::Constraint::fill();
    std::function<void(int, const std::string&)> on_change_;
    int                                          tab_index_ = 0;
    std::string                                  focus_group_;
    mutable strata::RadioGroup**                 ref_       = nullptr;
public:
    RadioGroup() = default;

    RadioGroup& items(std::vector<std::string> v)                       { items_     = std::move(v); return *this; }
    RadioGroup& selected(int i)                                         { selected_  = i;            return *this; }
    RadioGroup& size(strata::Constraint c)                              { size_      = c;            return *this; }
    RadioGroup& cross(strata::Constraint c)                             { cross_     = c;            return *this; }
    RadioGroup& change(std::function<void(int, const std::string&)> f)  { on_change_ = std::move(f); return *this; }
    RadioGroup& tab_index(int i)                                        { tab_index_ = i;            return *this; }
    RadioGroup& group(std::string g)                                    { focus_group_ = std::move(g); return *this; }
    RadioGroup& bind(strata::RadioGroup*& ref)                          { ref_       = &ref;         return *this; }

    std::unique_ptr<strata::Widget> build() const {
        auto w = std::make_unique<strata::RadioGroup>();
        w->set_items(items_).set_selected(selected_);
        if (on_change_) w->on_change = on_change_;
        w->tab_index = tab_index_;
        if (!focus_group_.empty()) w->set_focus_group(focus_group_);
        if (ref_) *ref_ = w.get();
        return w;
    }
    strata::Constraint constraint() const       { return size_; }
    strata::Constraint cross_constraint() const { return cross_; }
};

// ── ProgressBar ───────────────────────────────────────────────────────────────
class ProgressBar {
    float                          value_        = 0.0f;
    bool                           show_percent_ = true;
    strata::Constraint             size_         = strata::Constraint::fill();
    strata::Constraint             cross_        = strata::Constraint::fill();
    mutable strata::ProgressBar**  ref_          = nullptr;
public:
    ProgressBar() = default;

    ProgressBar& value(float v)                  { value_        = v;    return *this; }
    ProgressBar& show_percent(bool s)            { show_percent_ = s;    return *this; }
    ProgressBar& size(strata::Constraint c)      { size_         = c;    return *this; }
    ProgressBar& cross(strata::Constraint c)     { cross_        = c;    return *this; }
    ProgressBar& bind(strata::ProgressBar*& ref) { ref_          = &ref; return *this; }

    std::unique_ptr<strata::Widget> build() const {
        auto w = std::make_unique<strata::ProgressBar>();
        w->set_value(value_).set_show_percent(show_percent_);
        if (ref_) *ref_ = w.get();
        return w;
    }
    strata::Constraint constraint() const       { return size_; }
    strata::Constraint cross_constraint() const { return cross_; }
};

// ── Spinner ───────────────────────────────────────────────────────────────────
class Spinner {
    std::string                label_;
    bool                       auto_animate_  = false;
    int                        animate_every_ = 6;
    strata::Constraint         size_          = strata::Constraint::fill();
    strata::Constraint         cross_         = strata::Constraint::fill();
    mutable strata::Spinner**  ref_           = nullptr;
public:
    explicit Spinner(std::string label = "") : label_(std::move(label)) {}

    Spinner& auto_animate(bool enable, int every = 6) {
        auto_animate_  = enable;
        animate_every_ = every;
        return *this;
    }
    Spinner& size(strata::Constraint c)   { size_  = c;    return *this; }
    Spinner& cross(strata::Constraint c)  { cross_ = c;    return *this; }
    Spinner& bind(strata::Spinner*& ref)  { ref_   = &ref; return *this; }

    std::unique_ptr<strata::Widget> build() const {
        auto w = std::make_unique<strata::Spinner>(label_);
        w->set_auto_animate(auto_animate_, animate_every_);
        if (ref_) *ref_ = w.get();
        return w;
    }
    strata::Constraint constraint() const       { return size_; }
    strata::Constraint cross_constraint() const { return cross_; }
};

// ── ScrollView ────────────────────────────────────────────────────────────────
// Container descriptor that scrolls its children vertically.
class ScrollView {
    std::vector<Node>              children_;
    strata::Constraint             size_      = strata::Constraint::fill();
    strata::Constraint             cross_     = strata::Constraint::fill();
    int                            tab_index_ = 0;
    std::string                    focus_group_;
    mutable strata::ScrollView**   ref_       = nullptr;
public:
    explicit ScrollView(std::initializer_list<Node> children) : children_(children) {}
    explicit ScrollView(std::vector<Node> children) : children_(std::move(children)) {}

    ScrollView& size(strata::Constraint c)     { size_      = c;            return *this; }
    ScrollView& cross(strata::Constraint c)    { cross_     = c;            return *this; }
    ScrollView& tab_index(int i)               { tab_index_ = i;            return *this; }
    ScrollView& group(std::string g)           { focus_group_ = std::move(g); return *this; }
    ScrollView& bind(strata::ScrollView*& ref) { ref_       = &ref;         return *this; }

    std::unique_ptr<strata::Widget> build() const {
        auto w = std::make_unique<strata::ScrollView>(
            strata::Layout(strata::Layout::Direction::Vertical));
        for (const Node& n : children_)
            w->add(n.build(), n.constraint());
        w->tab_index = tab_index_;
        if (!focus_group_.empty()) w->set_focus_group(focus_group_);
        if (ref_) *ref_ = w.get();
        return w;
    }
    strata::Constraint constraint() const       { return size_; }
    strata::Constraint cross_constraint() const { return cross_; }
};

// ── ModalDesc ─────────────────────────────────────────────────────────────────
// Descriptor for building a strata::Modal.
// Modals are opened imperatively: int id = app.open_modal(modal_desc.build_modal());
// They are closed with: app.close_modal(id);
class ModalDesc {
    std::string           title_;
    int                   width_  = 60;
    int                   height_ = 20;
    strata::Style         border_style_;
    strata::Style         title_style_;
    strata::Style         overlay_style_;
    std::function<void()> on_close_;
    std::optional<Node>   inner_;
    mutable strata::Modal** ref_ = nullptr;
public:
    ModalDesc() = default;

    ModalDesc& title(std::string t)              { title_         = std::move(t); return *this; }
    ModalDesc& size(int w, int h)                { width_ = w; height_ = h;       return *this; }
    ModalDesc& border(strata::Style s)           { border_style_  = s;            return *this; }
    ModalDesc& title_style(strata::Style s)      { title_style_   = s;            return *this; }
    ModalDesc& overlay(strata::Style s)          { overlay_style_ = s;            return *this; }
    ModalDesc& on_close(std::function<void()> f) { on_close_      = std::move(f); return *this; }
    ModalDesc& inner(Node n)                     { inner_         = std::move(n); return *this; }
    ModalDesc& bind(strata::Modal*& ref)         { ref_           = &ref;         return *this; }

    // Build a Modal — use with app.open_modal(desc.build_modal())
    std::unique_ptr<strata::Modal> build_modal() const {
        auto m = std::make_unique<strata::Modal>();
        m->set_title(title_);
        m->set_size(width_, height_);
        m->set_border_style(border_style_);
        m->set_title_style(title_style_);
        m->set_overlay_style(overlay_style_);
        if (on_close_) m->set_on_close(on_close_);
        if (inner_) m->set_inner(inner_->build());
        if (ref_) *ref_ = m.get();
        return m;
    }
};

} // namespace strata::ui