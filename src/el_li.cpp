#include "html.h"
#include "el_li.h"
#include "document.h"

litehtml::el_li::el_li(const std::shared_ptr<litehtml::document>& doc) : litehtml::html_tag(doc)
{
}

int litehtml::el_li::render(int x, int y, int max_width, bool second_pass)
{
	if (css().get_list_style_type() >= list_style_type_armenian && !m_index_initialized)
	{
		if (auto p = parent())
		{
			const auto hasStart = p->get_attr(_t("start"));
			const int start = hasStart ? t_atoi(hasStart) : 1;
			int val = start;
			for (int i = 0, n = (int)p->get_children_count(); i < n; ++i)
			{
				auto child = p->get_child(i);
				if (child.get() == this)
				{
					set_attr(_t("list_index"), t_to_string(val).c_str());
					break;
				}
				else if (!t_strcmp(child->get_tagName(), _t("li")))
					++val;
			}
		}

		m_index_initialized = true;
	}

	return html_tag::render(x, y, max_width, second_pass);
}

litehtml::element::ptr litehtml::el_li::clone(const element::ptr& cloned_el)
{
    auto ret = std::dynamic_pointer_cast<litehtml::el_li>(cloned_el);
    if(!ret)
    {
        ret = std::make_shared<el_li>(get_document());
        html_tag::clone(ret);
    }

    ret->m_index_initialized = m_index_initialized;

    return cloned_el ? nullptr : ret;
}
