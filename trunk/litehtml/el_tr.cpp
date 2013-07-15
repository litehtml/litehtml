#include "html.h"
#include "el_tr.h"


litehtml::el_tr::el_tr( litehtml::document* doc ) : element(doc)
{

}

litehtml::el_tr::~el_tr()
{

}

void litehtml::el_tr::parse_styles(bool is_reparse)
{
	const wchar_t* str = get_attr(L"align");
	if(str)
	{
		m_style.add_property(L"text-align", str, 0, false);
	}

	element::parse_styles(is_reparse);
}

void litehtml::el_tr::get_inline_boxes( position::vector& boxes )
{
	position pos;
	for(elements_vector::iterator iter = m_children.begin(); iter != m_children.end(); iter++)
	{
		element* el = (*iter);
		if(el->get_display() == display_table_cell)
		{
			pos.x		= el->left() + el->margin_left();
			pos.y		= el->top() - m_padding.top - m_borders.top;

			pos.width	= el->right() - pos.x - el->margin_right() - el->margin_left();
			pos.height	= el->height() + m_padding.top + m_padding.bottom + m_borders.top + m_borders.bottom;

			boxes.push_back(pos);
		}
	}
}
