#include "html.h"
#include "el_cdata.h"

litehtml::el_cdata::el_cdata(const std::shared_ptr<litehtml::document>& doc) : litehtml::element(doc)
{
	m_skip = true;
}

void litehtml::el_cdata::get_text( tstring& text )
{
	text += m_text;
}

void litehtml::el_cdata::set_data( const tchar_t* data )
{
	if(data)
	{
		m_text += data;
	}
}

litehtml::element::ptr litehtml::el_cdata::clone(const element::ptr& cloned_el)
{
    auto ret = std::dynamic_pointer_cast<litehtml::el_cdata>(cloned_el);
    if(!ret)
    {
        ret = std::make_shared<el_cdata>(get_document());
        element::clone(ret);
    }

    ret->m_text = m_text;

    return cloned_el ? nullptr : ret;
}
