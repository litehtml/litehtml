#ifndef LITEHTML_EL_BEFORE_AFTER_H
#define LITEHTML_EL_BEFORE_AFTER_H

#include "html_tag.h"

namespace litehtml
{
    class el_before_after_base : public html_tag
    {
      public:
        el_before_after_base(const std::shared_ptr<document>& doc, bool before);

        void add_style(const style& style) override;
        void compute_styles(bool recursive = true) override;

        // Regenerates the content resolving the quote keywords against the document-wide quote nesting level.
        // `quote_depth` is updated with the nesting level after this element's content. Returns true when the
        // content was regenerated.
        bool resolve_quotes(int& quote_depth);

      private:
        enum class quote_action
        {
            open,
            close,
            no_open,
            no_close
        };

        // Children generated for one quote keyword, so resolve_quotes() can replace that text without
        // regenerating the rest of the content (counters and attr() must not be evaluated again).
        struct quote_span
        {
            std::size_t  index;
            std::size_t  count;
            quote_action action;
        };

        void               add_text(const std::string& txt);
        void               append_text(elements_list& children, const std::string& txt);
        void               add_function(const std::string& fnc, const std::string& params);
        void               parse_content(const std::string& content, int& quote_depth, const css_quotes& quotes);
        static void        advance_quote_depth(quote_action action, int& depth);
        static std::string convert_escape(const char* txt);

        std::vector<quote_span> m_quote_spans;

        bool       m_quotes_resolved = false;
        int        m_resolved_depth  = 0;
        css_quotes m_resolved_quotes;
    };

    class el_before : public el_before_after_base
    {
      public:
        explicit el_before(const std::shared_ptr<document>& doc) :
            el_before_after_base(doc, true)
        {
        }
    };

    class el_after : public el_before_after_base
    {
      public:
        explicit el_after(const std::shared_ptr<document>& doc) :
            el_before_after_base(doc, false)
        {
        }
    };
} // namespace litehtml

#endif // LITEHTML_EL_BEFORE_AFTER_H
