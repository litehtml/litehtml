#include "control_text_layout.h"
#include "document.h"
#include "document_container.h"
#include <algorithm>

namespace litehtml
{
    namespace control_text
    {
        size_t next_offset(const std::string& text, size_t pos)
        {
            if(pos >= text.size())
            {
                return text.size();
            }
            const auto first = static_cast<unsigned char>(text[pos]);
            size_t     len   = 1;
            if((first & 0xE0) == 0xC0)
            {
                len = 2;
            } else if((first & 0xF0) == 0xE0)
            {
                len = 3;
            } else if((first & 0xF8) == 0xF0)
            {
                len = 4;
            }
            while(pos + len > text.size() && len > 1)
            {
                len--;
            }
            return std::min(text.size(), pos + len);
        }

        size_t prev_offset(const std::string& text, size_t pos)
        {
            if(pos == 0 || text.empty())
            {
                return 0;
            }
            size_t ret = std::min(pos, text.size()) - 1;
            while(ret > 0 && (static_cast<unsigned char>(text[ret]) & 0xC0) == 0x80)
            {
                ret--;
            }
            return ret;
        }

        pixel_t measure(const document& doc, uint_ptr font, const std::string& text, size_t start, size_t end)
        {
            if(!font || start >= end || start >= text.size())
            {
                return 0_px;
            }
            end = std::min(end, text.size());
            return doc.container()->text_width(text.substr(start, end - start).c_str(), font);
        }

        std::vector<line> layout_lines(const document& doc, uint_ptr font, const std::string& text, pixel_t max_width)
        {
            std::vector<line> lines;
            if(text.empty())
            {
                lines.push_back({0, 0, 0_px});
                return lines;
            }

            size_t paragraph_start = 0;
            while(paragraph_start <= text.size())
            {
                size_t paragraph_end = text.find('\n', paragraph_start);
                if(paragraph_end == std::string::npos)
                {
                    paragraph_end = text.size();
                }

                if(paragraph_start == paragraph_end)
                {
                    lines.push_back({paragraph_start, paragraph_end, 0_px});
                } else
                {
                    size_t line_start = paragraph_start;
                    size_t last_break = std::string::npos;
                    size_t pos        = paragraph_start;

                    while(pos < paragraph_end)
                    {
                        const size_t next = std::min(next_offset(text, pos), paragraph_end);
                        const char   ch   = text[pos];
                        if(ch == ' ' || ch == '\t' || ch == '-')
                        {
                            last_break = next;
                        }

                        const pixel_t width = measure(doc, font, text, line_start, next);
                        if(max_width > 0_px && width > max_width && next > line_start)
                        {
                            size_t break_at =
                                (last_break != std::string::npos && last_break > line_start) ? last_break : next;
                            if(break_at <= line_start)
                            {
                                break_at = next;
                            }
                            lines.push_back({line_start, break_at, measure(doc, font, text, line_start, break_at)});
                            line_start = break_at;
                            last_break = std::string::npos;
                            pos        = break_at;
                            continue;
                        }
                        pos = next;
                    }
                    lines.push_back({line_start, paragraph_end, measure(doc, font, text, line_start, paragraph_end)});
                }

                if(paragraph_end >= text.size())
                {
                    break;
                }
                paragraph_start = paragraph_end + 1;
                if(paragraph_start > text.size())
                {
                    break;
                }
            }

            if(lines.empty())
            {
                lines.push_back({0, 0, 0_px});
            }
            return lines;
        }

        caret caret_position(const document& doc, uint_ptr font, const std::vector<line>& lines,
                             const std::string& text, size_t caret)
        {
            caret = std::min(caret, text.size());
            control_text::caret ret;
            for(size_t i = 0; i < lines.size(); i++)
            {
                const auto& ln = lines[i];
                if(caret <= ln.end || i + 1 == lines.size())
                {
                    ret.line = i;
                    ret.x    = measure(doc, font, text, ln.start, std::min(caret, ln.end));
                    return ret;
                }
            }
            return ret;
        }

        std::string masked_text(const std::string& text)
        {
            static const char bullet[] = "\xE2\x80\xA2";
            std::string       ret;
            for(size_t pos = 0; pos < text.size();)
            {
                pos  = next_offset(text, pos);
                ret += bullet;
            }
            return ret;
        }
    } // namespace control_text
} // namespace litehtml
