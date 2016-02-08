#include "html.h"
#include "el_style.h"
#include "document.h"


litehtml::el_style::el_style( litehtml::document* doc ) : litehtml::element(doc)
{

}

litehtml::el_style::~el_style()
{

}

void litehtml::el_style::parse_attributes()
{
	tstring text;

	for(elements_vector::iterator iter = m_children.begin(); iter != m_children.end(); iter++)
	{
		(*iter)->get_text(text);
	}
	m_doc->add_stylesheet( text.c_str(), 0, get_attr(_t("media")) );
}

bool litehtml::el_style::appendChild( litehtml::element* el )
{
	m_children.push_back(el);
	return true;
}

bool litehtml::el_style::addChildAfter( litehtml::element* new_el, litehtml::element* existing_el )
{
	auto iter = std::find( m_children.begin(), m_children.end(), existing_el );
	m_children.insert( ++iter, new_el );
	return true;
}


const litehtml::tchar_t* litehtml::el_style::get_tagName() const
{
	return _t("style");
}
