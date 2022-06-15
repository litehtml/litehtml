#include "html.h"
#include "el_para.h"
#include "document.h"

litehtml::el_para::el_para(const std::shared_ptr<litehtml::document>& doc) : litehtml::html_tag(doc)
{
}

void litehtml::el_para::parse_attributes()
{
	const tchar_t* str = get_attr(_t("align"));
	if(str)
	{
		m_style.add_property(_t("text-align"), str, nullptr, false, this);
	}

	html_tag::parse_attributes();
}

litehtml::element::ptr litehtml::el_para::clone(const element::ptr& cloned_el)
{
    auto ret = std::dynamic_pointer_cast<litehtml::el_para>(cloned_el);
    if(!ret)
    {
        ret = std::make_shared<el_para>(get_document());
        html_tag::clone(ret);
    }

    return cloned_el ? nullptr : ret;
}
