#include "html.h"
#include "el_style.h"
#include "document.h"


litehtml::el_style::el_style(const std::shared_ptr<litehtml::document>& doc) : litehtml::element(doc)
{

}

void litehtml::el_style::parse_attributes()
{
	tstring text;

	for(auto& el : m_children)
	{
		el->get_text(text);
	}
	get_document()->add_stylesheet( text.c_str(), nullptr, get_attr(_t("media")) );
}

bool litehtml::el_style::appendChild(const ptr &el)
{
	m_children.push_back(el);
	return true;
}

const litehtml::tchar_t* litehtml::el_style::get_tagName() const
{
	return _t("style");
}

litehtml::element::ptr litehtml::el_style::clone(const element::ptr& cloned_el)
{
    auto ret = std::dynamic_pointer_cast<litehtml::el_style>(cloned_el);
    if(!ret)
    {
        ret = std::make_shared<el_style>(get_document());
        element::clone(ret);
    }

    ret->m_children = m_children;

    return cloned_el ? nullptr : ret;
}
