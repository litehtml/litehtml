#include "html.h"
#include "el_div.h"


litehtml::el_div::el_div(const std::shared_ptr<litehtml::document>& doc) : html_tag(doc)
{

}

void litehtml::el_div::parse_attributes()
{
	const tchar_t* str = get_attr(_t("align"));
	if(str)
	{
		m_style.add_property(_t("text-align"), str, 0, false, this);
	}
	html_tag::parse_attributes();
}

litehtml::element::ptr litehtml::el_div::clone(const element::ptr& cloned_el)
{
    auto ret = std::dynamic_pointer_cast<litehtml::el_div>(cloned_el);
    if(!ret)
    {
        ret = std::make_shared<el_div>(get_document());
        html_tag::clone(ret);
    }

    return cloned_el ? nullptr : ret;
}
