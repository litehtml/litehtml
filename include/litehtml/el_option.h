#ifndef LITEHTML_EL_OPTION_H
#define LITEHTML_EL_OPTION_H

#include "document.h"
#include "html_tag.h"
#include <optional>

namespace litehtml
{
    class el_option : public html_tag
    {
      public:
        explicit el_option(const document::ptr& doc);

        void parse_attributes() override;
        bool is_checked() const override;
        bool is_disabled() const override;

        const std::string& value() const;
        const std::string& label_text() const;
        bool               selected() const;
        void               set_selected(bool selected);

        std::string dump_get_name() override;

      private:
        std::string         m_value;
        std::string         m_label;
        std::optional<bool> m_selected;
    };
} // namespace litehtml

#endif // LITEHTML_EL_OPTION_H
