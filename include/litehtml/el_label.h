#ifndef LITEHTML_EL_LABEL_H
#define LITEHTML_EL_LABEL_H

#include "document.h"
#include "html_tag.h"

namespace litehtml
{
    class el_label : public html_tag
    {
      public:
        explicit el_label(const document::ptr& doc);

        void parse_attributes() override;
        void on_click() override;

        std::string dump_get_name() override;

      private:
        string_id    m_for = empty_id;
        element::ptr find_control(const element::ptr& parent) const;
    };
} // namespace litehtml

#endif // LITEHTML_EL_LABEL_H
