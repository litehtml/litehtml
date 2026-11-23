#include "el_option.h"
#include "html.h"

namespace litehtml
{
    el_option::el_option(const document::ptr& doc) :
        html_tag(doc)
    {
    }

    void el_option::parse_attributes()
    {
        html_tag::parse_attributes();

        get_text(m_label);
        m_value    = get_attr("value", m_label.c_str());
        m_selected = get_attr("selected") != nullptr;
        if(*m_selected)
        {
            set_pseudo_class(_checked_, true);
        }
    }

    bool el_option::is_checked() const
    {
        if(m_selected.has_value())
        {
            return *m_selected;
        }
        return get_attr("selected") != nullptr;
    }

    bool el_option::is_disabled() const
    {
        if(element::is_disabled())
        {
            return true;
        }
        for(auto par = parent(); par && par->tag() != _select_; par = par->parent())
        {
            if(par->tag() == _optgroup_ && par->get_attr("disabled"))
            {
                return true;
            }
        }
        return false;
    }

    const std::string& el_option::value() const
    {
        return m_value;
    }

    const std::string& el_option::label_text() const
    {
        return m_label;
    }

    bool el_option::selected() const
    {
        return is_checked();
    }

    void el_option::set_selected(bool selected)
    {
        m_selected = selected;
        set_pseudo_class(_checked_, selected);
    }

    std::string el_option::dump_get_name()
    {
        return "option";
    }
} // namespace litehtml
