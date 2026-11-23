#include "el_label.h"
#include "document.h"

namespace litehtml
{
    el_label::el_label(const document::ptr& doc) :
        html_tag(doc)
    {
    }

    void el_label::parse_attributes()
    {
        html_tag::parse_attributes();
        if(const char* for_attr = get_attr("for"))
        {
            m_for = _id(for_attr);
        }
    }

    element::ptr el_label::find_control(const element::ptr& parent) const
    {
        if(m_for == empty_id)
        {
            return nullptr;
        }
        for(const auto& child : parent->children())
        {
            if(child->id() == m_for)
            {
                return child;
            }
            if(auto found = find_control(child))
            {
                return found;
            }
        }
        return nullptr;
    }

    void el_label::on_click()
    {
        if(!is_inert())
        {
            const auto root    = get_document()->root();
            auto       control = root ? find_control(root) : nullptr;
            if(control && (control->tag() == _input_ || control->tag() == _textarea_ || control->tag() == _select_ ||
                           control->tag() == _button_))
            {
                // When the control is wrapped by this label the click already reached it.
                bool is_descendant = false;
                for(auto par = control->parent(); par; par = par->parent())
                {
                    if(par.get() == this)
                    {
                        is_descendant = true;
                        break;
                    }
                }
                if(!is_descendant)
                {
                    get_document()->set_focus(control, false, {});
                    control->on_click();
                }
            }
        }
        html_tag::on_click();
    }

    std::string el_label::dump_get_name()
    {
        return "label";
    }
} // namespace litehtml
