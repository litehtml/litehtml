#ifndef LITEHTML_CONTROL_TEXT_LAYOUT_H
#define LITEHTML_CONTROL_TEXT_LAYOUT_H

#include "litehtml/pixel_type.h"
#include "litehtml/types.h"
#include <cstddef>
#include <string>
#include <vector>

namespace litehtml
{
    class document;

    // Shared UTF-8 text measurement and line breaking used by the form controls. Not part of the
    // public API.
    namespace control_text
    {
        struct line
        {
            size_t  start = 0;
            size_t  end   = 0;
            pixel_t width = 0_px;
        };

        struct caret
        {
            size_t  line = 0;
            pixel_t x    = 0_px;
        };

        size_t next_offset(const std::string& text, size_t pos);
        size_t prev_offset(const std::string& text, size_t pos);

        pixel_t measure(const document& doc, uint_ptr font, const std::string& text, size_t start, size_t end);

        // Splits text into lines on '\n' and greedily wraps at spaces to max_width. max_width <= 0
        // only splits on explicit newlines.
        std::vector<line> layout_lines(const document& doc, uint_ptr font, const std::string& text, pixel_t max_width);

        caret caret_position(const document& doc, uint_ptr font, const std::vector<line>& lines,
                             const std::string& text, size_t caret);

        // Replaces every UTF-8 codepoint with U+2022 (used by password inputs).
        std::string masked_text(const std::string& text);
    } // namespace control_text
} // namespace litehtml

#endif // LITEHTML_CONTROL_TEXT_LAYOUT_H
