#include "html.h"
#include "el_break.h"

litehtml::el_break::el_break(const std::shared_ptr<litehtml::document>& doc) : html_tag(doc)
{

}

bool litehtml::el_break::is_break() const
{
	return true;
}

litehtml::element::ptr litehtml::el_break::clone(const element::ptr& cloned_el)
{
    auto ret = std::dynamic_pointer_cast<litehtml::el_break>(cloned_el);
    if(!ret)
    {
        ret = std::make_shared<el_break>(get_document());
        html_tag::clone(ret);
    }

    return cloned_el ? nullptr : ret;
}
