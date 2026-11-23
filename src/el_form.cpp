#include "el_form.h"
#include "document.h"
#include "document_container.h"
#include "el_button.h"
#include "el_input.h"
#include "el_select.h"
#include "el_textarea.h"
#include "html.h"

namespace litehtml
{
    el_form::el_form(const document::ptr& doc) :
        html_tag(doc)
    {
    }

    void el_form::collect_controls(const element::ptr& parent, std::vector<element::ptr>& controls) const
    {
        for(const auto& child : parent->children())
        {
            if(child->tag() == _form_)
            {
                // Forms cannot be nested; a nested form owns its own controls.
                continue;
            }
            if(child->tag() == _input_ || child->tag() == _textarea_ || child->tag() == _select_ ||
               child->tag() == _button_)
            {
                controls.push_back(child);
            }
            collect_controls(child, controls);
        }
    }

    void el_form::submit(const element::ptr& submitter)
    {
        element::ptr submit_el = submitter;
        if(!submit_el)
        {
            submit_el = find_default_submit_button();
        }

        form_data data;
        data.id      = get_attr("id", "");
        data.action  = get_attr("action", "");
        data.method  = lowcase(get_attr("method", "get"));
        data.target  = get_attr("target", "");
        data.enctype = lowcase(get_attr("enctype", "application/x-www-form-urlencoded"));

        std::vector<element::ptr> controls;
        collect_controls(shared_from_this(), controls);

        for(const auto& el : controls)
        {
            if(el->is_disabled())
            {
                continue;
            }
            if(auto* input = dynamic_cast<el_input*>(el.get()))
            {
                if(const auto field = input->create_form_field(submit_el.get() == el.get()))
                {
                    data.fields.push_back(*field);
                }
            } else if(auto* textarea = dynamic_cast<el_textarea*>(el.get()))
            {
                if(const auto field = textarea->create_form_field())
                {
                    data.fields.push_back(*field);
                }
            } else if(auto* select = dynamic_cast<el_select*>(el.get()))
            {
                for(auto& field : select->form_fields())
                {
                    data.fields.push_back(field);
                }
            } else if(auto* button = dynamic_cast<el_button*>(el.get()))
            {
                if(submit_el && submit_el == el)
                {
                    if(const auto field = button->create_form_field())
                    {
                        data.fields.push_back(*field);
                    }
                }
            }
        }

        if(auto* container = get_document()->container())
        {
            container->on_submit(data);
        }
    }

    void el_form::reset()
    {
        std::vector<element::ptr> controls;
        collect_controls(shared_from_this(), controls);
        for(const auto& el : controls)
        {
            if(auto* input = dynamic_cast<el_input*>(el.get()))
            {
                input->reset();
            } else if(auto* textarea = dynamic_cast<el_textarea*>(el.get()))
            {
                textarea->reset();
            } else if(auto* select = dynamic_cast<el_select*>(el.get()))
            {
                select->reset();
            }
        }
        get_document()->request_redraw();
    }

    element::ptr el_form::find_control_by_id(string_id id)
    {
        std::vector<element::ptr> controls;
        collect_controls(shared_from_this(), controls);
        for(const auto& el : controls)
        {
            if(el->id() == id)
            {
                return el;
            }
        }
        return nullptr;
    }

    element::ptr el_form::find_default_submit_button()
    {
        std::vector<element::ptr> controls;
        collect_controls(shared_from_this(), controls);
        for(const auto& el : controls)
        {
            if(el->is_disabled())
            {
                continue;
            }
            if(auto* input = dynamic_cast<el_input*>(el.get()))
            {
                if(input->is_submit_control())
                {
                    return el;
                }
            } else if(auto* button = dynamic_cast<el_button*>(el.get()))
            {
                if(button->is_submit_control())
                {
                    return el;
                }
            }
        }
        return nullptr;
    }

    std::string el_form::dump_get_name()
    {
        return "form";
    }
} // namespace litehtml
