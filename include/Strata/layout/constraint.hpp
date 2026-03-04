#pragma once

namespace strata {

struct Constraint {
    enum class Type { Fixed, Min, Max, Percentage, Fill };

    Type type  = Type::Fill;
    int  value = 1; // weight for Fill, n for others

    static Constraint fixed(int n)       { return {Type::Fixed,      n}; }
    static Constraint min(int n)         { return {Type::Min,        n}; }
    static Constraint max(int n)         { return {Type::Max,        n}; }
    static Constraint percentage(int p)  { return {Type::Percentage, p}; }
    static Constraint fill(int weight=1) { return {Type::Fill,       weight}; }
};

} // namespace strata
