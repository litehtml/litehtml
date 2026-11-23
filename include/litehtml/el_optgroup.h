#ifndef LITEHTML_EL_OPTGROUP_H
#define LITEHTML_EL_OPTGROUP_H

#include "document.h"
#include "html_tag.h"

namespace litehtml
{
    class el_optgroup : public html_tag
    {
      public:
        explicit el_optgroup(const document::ptr& doc);

        void parse_attributes() override;

        const std::string& label_text() const;

        std::string dump_get_name() override;

      private:
        std::string m_label;
    };
} // namespace litehtml

#endif // LITEHTML_EL_OPTGROUP_H
