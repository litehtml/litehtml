#ifndef LITEHTML_CSS_QUOTES_H
#define LITEHTML_CSS_QUOTES_H

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace litehtml
{
    // CSS Property: quotes
    // https://drafts.csswg.org/css-content-3/#quotes
    class css_quotes
    {
      public:
        css_quotes() = default; // auto

        static css_quotes none()
        {
            css_quotes quotes;
            quotes.m_type = quotes_none;
            return quotes;
        }

        static css_quotes from_strings(std::vector<std::string> values)
        {
            css_quotes quotes;
            quotes.m_type   = quotes_strings;
            quotes.m_values = std::move(values);
            return quotes;
        }

        bool operator==(const css_quotes& other) const
        {
            return m_type == other.m_type && (m_type != quotes_strings || m_values == other.m_values);
        }

        bool operator!=(const css_quotes& other) const
        {
            return !(*this == other);
        }

        // Returns the opening/closing quotation mark for the zero-based nesting level `depth`. The last
        // pair is repeated when the level exceeds the number of pairs. The returned string is empty when
        // no quotation marks are specified.
        const std::string& open_quote(int depth) const
        {
            if(m_type == quotes_auto)
            {
                // “ ‘
                static const std::string default_quotes[2] = {"\xE2\x80\x9C", "\xE2\x80\x98"};
                return default_quotes[std::min(std::max(depth, 0), 1)];
            }
            return pair_value(depth, 0);
        }

        const std::string& close_quote(int depth) const
        {
            if(m_type == quotes_auto)
            {
                // ” ’
                static const std::string default_quotes[2] = {"\xE2\x80\x9D", "\xE2\x80\x99"};
                return default_quotes[std::min(std::max(depth, 0), 1)];
            }
            return pair_value(depth, 1);
        }

      private:
        enum quotes_type
        {
            quotes_auto,
            quotes_none,
            quotes_strings
        };

        const std::string& pair_value(int depth, size_t index) const
        {
            static const std::string empty;
            const size_t             pairs = m_values.size() / 2;
            if(m_type != quotes_strings || pairs == 0)
            {
                return empty;
            }
            const auto pair = static_cast<size_t>(std::max(depth, 0));
            return m_values[std::min(pair, pairs - 1) * 2 + index];
        }

        quotes_type m_type = quotes_auto;
        // open0, close0, open1, close1, ...
        std::vector<std::string> m_values;
    };
} // namespace litehtml

#endif // LITEHTML_CSS_QUOTES_H
