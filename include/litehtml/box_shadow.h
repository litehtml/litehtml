#ifndef LITEHTML_BOX_SHADOW_H
#define LITEHTML_BOX_SHADOW_H

#include <vector>

#include "css_length.h"
#include "types.h"
#include "web_color.h"

namespace litehtml
{
    // A single computed box shadow. The lengths are resolved to the document units when the style is
    // computed; colors are resolved to the used color. https://drafts.csswg.org/css-backgrounds/#box-shadow
    struct box_shadow
    {
        css_length x;
        css_length y;
        css_length blur;
        css_length spread;
        web_color  color = web_color::current_color;
        bool       inset = false;
    };
    using box_shadow_vector = std::vector<box_shadow>;

    // A box shadow in pixels, passed to document_container::draw_box_shadow.
    struct shadow_paint
    {
        pixel_t   x      = 0_px;
        pixel_t   y      = 0_px;
        pixel_t   blur   = 0_px;
        pixel_t   spread = 0_px;
        web_color color;
        bool      inset = false;
    };
} // namespace litehtml

#endif // LITEHTML_BOX_SHADOW_H
