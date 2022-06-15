#include "html.h"
#include "el_comment.h"

litehtml::el_comment::el_comment(const std::shared_ptr<litehtml::document>& doc) : litehtml::element(doc)
{
	m_skip = true;
}

bool litehtml::el_comment::is_comment() const
{
	return true;
}

void litehtml::el_comment::get_text( tstring& text )
{
	text += m_text;
}

void litehtml::el_comment::set_data( const tchar_t* data )
{
	if(data)
	{
		m_text += data;
	}
}

litehtml::element::ptr litehtml::el_comment::clone(const element::ptr& cloned_el)
{
    auto ret = std::dynamic_pointer_cast<litehtml::el_comment>(cloned_el);
    if(!ret)
    {
        ret = std::make_shared<el_comment>(get_document());
        element::clone(ret);
    }

    return cloned_el ? nullptr : ret;
}
