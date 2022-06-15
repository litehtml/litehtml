#include "html.h"
#include "el_body.h"
#include "document.h"

litehtml::el_body::el_body(const std::shared_ptr<litehtml::document>& doc) : litehtml::html_tag(doc)
{
}

bool litehtml::el_body::is_body()  const
{
	return true;
}

litehtml::element::ptr litehtml::el_body::clone(const element::ptr& cloned_el)
{
    auto ret = std::dynamic_pointer_cast<litehtml::el_body>(cloned_el);
    if(!ret)
    {
        ret = std::make_shared<el_body>(get_document());
        html_tag::clone(ret);
    }

    return cloned_el ? nullptr : ret;
}
