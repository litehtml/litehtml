#include "html.h"
#include "el_script.h"
#include "document.h"


litehtml::el_script::el_script(const std::shared_ptr<litehtml::document>& doc) : litehtml::element(doc)
{

}

void litehtml::el_script::parse_attributes()
{
	//TODO: pass script text to document container
}

bool litehtml::el_script::appendChild(const ptr &el)
{
	el->get_text(m_text);
	return true;
}

const litehtml::tchar_t* litehtml::el_script::get_tagName() const
{
	return _t("script");
}

litehtml::element::ptr litehtml::el_script::clone(const element::ptr& cloned_el)
{
    auto ret = std::dynamic_pointer_cast<litehtml::el_script>(cloned_el);
    if(!ret)
    {
        ret = std::make_shared<el_script>(get_document());
        element::clone(ret);
    }

    return cloned_el ? nullptr : ret;
}
