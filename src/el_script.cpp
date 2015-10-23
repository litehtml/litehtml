#include "html.h"
#include "el_script.h"
#include "document.h"


litehtml::el_script::el_script( litehtml::document* doc ) : litehtml::element(doc)
{

}

litehtml::el_script::~el_script()
{

}

void litehtml::el_script::parse_attributes()
{
	m_doc->container()->execute_script( m_doc, this );
}

bool litehtml::el_script::appendChild( litehtml::element* el )
{
	el->get_text(m_text);
	return true;
}

bool litehtml::el_script::addChildAfter( litehtml::element* new_el, litehtml::element* existing_el)
{
	//    :TODO:
	return false;
}

const litehtml::tchar_t* litehtml::el_script::get_tagName() const
{
	return _t("script");
}

void litehtml::el_script::set_attr(const tchar_t* name, const tchar_t* val)
{
	check_lower_case( name );
	if( !t_strcmp( name, _t("src")) )
	{
		m_src = val;
	}
}

const litehtml::tchar_t * litehtml::el_script::get_attr(const tchar_t* name, const tchar_t* def) const
{
	check_lower_case( name );
	if( !t_strcmp( name, _t("src")) )
	{
		return m_src.empty() ? 0 : m_src.c_str();
	}

	return def;
}
