#include "html.h"
#include "iterators.h"
#include "html_tag.h"
#include "render_item.h"
#include <iterator>

litehtml::elements_iterator::elements_iterator(bool return_parents, iterator_selector* go_inside, iterator_selector* select) :
    m_return_parent(return_parents),
    m_go_inside(go_inside),
    m_select(select)
{
}

bool litehtml::elements_iterator::go_inside(const std::shared_ptr<render_item>& el)
{
    return 	!el->children().empty() && m_go_inside && m_go_inside->select(el);
}

void litehtml::elements_iterator::process(const std::shared_ptr<render_item>& container, const std::function<void (std::shared_ptr<render_item>&)>& func)
{
    for(auto& el : container->children())
    {
        if(go_inside(el))
        {
            if(m_return_parent)
            {
                // call function for parent
                func(el);
            }
            // go inside element and process its items
            process(el, func);
        } else
        {
            // call function for element
            if(!m_select || m_select->select(el))
            {
                func(el);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


bool litehtml::go_inside_inline::select(const std::shared_ptr<render_item>& el)
{
	if(el->src_el()->css().get_display() == display_inline || el->src_el()->css().get_display() == display_inline_text)
	{
		return true;
	}
	return false;
}

bool litehtml::go_inside_table::select(const std::shared_ptr<render_item>& el)
{
	if(	el->src_el()->css().get_display() == display_table_row_group ||
		el->src_el()->css().get_display() == display_table_header_group ||
		el->src_el()->css().get_display() == display_table_footer_group)
	{
		return true;
	}
	return false;
}

bool litehtml::table_rows_selector::select(const std::shared_ptr<render_item>& el)
{
	if(	el->src_el()->css().get_display() == display_table_row)
	{
		return true;
	}
	return false;
}

bool litehtml::table_cells_selector::select(const std::shared_ptr<render_item>& el)
{
	if(	el->src_el()->css().get_display() == display_table_cell)
	{
		return true;
	}
	return false;
}
