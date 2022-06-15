#include "html.h"
#include "el_base.h"
#include "document.h"

litehtml::el_base::el_base(const std::shared_ptr<litehtml::document>& doc) : html_tag(doc)
{
	
}

void litehtml::el_base::parse_attributes()
{
	get_document()->container()->set_base_url(get_attr(_t("href")));
}

litehtml::element::ptr litehtml::el_base::clone(const element::ptr& cloned_el)
{
    auto ret = std::dynamic_pointer_cast<litehtml::el_base>(cloned_el);
    if(!ret)
    {
        ret = std::make_shared<el_base>(get_document());
        html_tag::clone(ret);
    }

    return cloned_el ? nullptr : ret;
}
