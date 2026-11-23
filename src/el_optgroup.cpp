#include "el_optgroup.h"

namespace litehtml
{
    el_optgroup::el_optgroup(const document::ptr& doc) :
        html_tag(doc)
    {
    }

    void el_optgroup::parse_attributes()
    {
        html_tag::parse_attributes();
        m_label = get_attr("label", "");
    }

    const std::string& el_optgroup::label_text() const
    {
        return m_label;
    }

    std::string el_optgroup::dump_get_name()
    {
        return "optgroup";
    }
} // namespace litehtml
