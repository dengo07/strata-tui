#pragma once
#include "color.hpp"

namespace strata {

struct Style {
    Color fg = color::Default;
    Color bg = color::Default;
    bool bold      = false;
    bool italic    = false;
    bool underline = false;
    bool dim       = false;
    bool blink     = false;
    bool reverse   = false;

    Style& with_fg(Color c)     { fg = c; return *this; }
    Style& with_bg(Color c)     { bg = c; return *this; }
    Style& with_bold()          { bold      = true; return *this; }
    Style& with_italic()        { italic    = true; return *this; }
    Style& with_underline()     { underline = true; return *this; }
    Style& with_dim()           { dim       = true; return *this; }
    Style& with_blink()         { blink     = true; return *this; }
    Style& with_reverse()       { reverse   = true; return *this; }

    bool operator==(const Style& o) const {
        return fg == o.fg && bg == o.bg && bold == o.bold && italic == o.italic
            && underline == o.underline && dim == o.dim && blink == o.blink && reverse == o.reverse;
    }
    bool operator!=(const Style& o) const { return !(*this == o); }
};

} // namespace strata
