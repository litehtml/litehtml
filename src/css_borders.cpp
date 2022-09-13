#include "html.h"
#include "borders.h"

litehtml::tstring litehtml::css_border::to_string()
{
    return width.to_string() + _t("/") + index_value(style, border_style_strings) + _t("/") + color.to_string();
}
