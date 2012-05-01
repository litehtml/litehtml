#include "html.h"
#include "element.h"
#include "tokenizer.h"
#include "document.h"
#include "iterators.h"
#include "stylesheet.h"
#include <string>

litehtml::element::element(litehtml::document* doc)
{
	m_text_align			= text_align_left;
	m_el_position			= element_position_static;
	m_skip					= false;
	m_parent				= 0;
	m_doc					= doc;
	m_display				= display_inline;
	m_vertical_align		= va_baseline;
	m_list_style_type		= list_style_type_none;
	m_list_style_position	= list_style_position_outside;
	m_float					= float_none;
	m_clear					= clear_none;
}

litehtml::element::~element()
{

}

void litehtml::element::appendChild( litehtml::element* el )
{
	if(el)
	{
		el->m_parent = this;
		m_children.push_back(el);
	}
}

litehtml::element::ptr litehtml::element::parentElement() const
{
	return m_parent;
}

const wchar_t* litehtml::element::get_tagName() const
{
	return m_tag.c_str();
}

litehtml::style& litehtml::element::get_style()
{
	return m_style;
}

void litehtml::element::set_attr( const wchar_t* name, const wchar_t* val )
{
	if(name && val)
	{
		m_attrs[name] = val;
	}
}

const wchar_t* litehtml::element::get_attr( const wchar_t* name, const wchar_t* def )
{
	string_map::const_iterator attr = m_attrs.find(name);
	if(attr != m_attrs.end())
	{
		return attr->second.c_str();
	}
	return def;
}

bool litehtml::element::select( const wchar_t* selector )
{
	std::vector<std::wstring> tokens;
	tokenize(selector, tokens, L",");
	for(std::vector<std::wstring>::iterator i = tokens.begin(); i != tokens.end(); i++)
	{
		trim(*i);
		if(select_one(i->c_str()))
		{
			return true;
		}
	}
	return false;
}

bool litehtml::element::select_one( const std::wstring& selector )
{
	std::vector<std::wstring> tokens;
	tokenize(selector, tokens, L" \t>+", L" \t>+", L"[]");

	css_element_selector sel;
	sel.parse(L"div.hello#rt[rel=none][title]");


/*
	std::wstring::size_type pos = sel.find_first_of(L".#[");

	if(sel.substr(0, m_tag.length()) == m_tag || sel[0] == L'*' || !iswalpha(sel[0]))
	{
		std::find_first_of()
		std::wstring::size_type pos = sel.find_first_not_of()
		return true;
	}
*/

	return false;
}

void litehtml::element::apply_stylesheet( const litehtml::style_sheet& style )
{
	bool apply = false;
	for(css_selector::vector::const_iterator i = style.m_selectors.begin(); i != style.m_selectors.end(); i++)
	{
		if(*this == *i)
		{
			apply = true;
			break;
		}
	}
	if(apply)
	{
		m_style.combine(style.m_style);
	}

	for(elements_vector::iterator i = m_children.begin(); i != m_children.end(); i++)
	{
		(*i)->apply_stylesheet(style);
	}
}

void litehtml::element::get_content_size( uint_ptr hdc, size& sz, int max_width )
{
	sz.height	= 0;
	if(m_display == display_block)
	{
		sz.width	= max_width;
	} else
	{
		sz.width	= 0;
	}
}

void litehtml::element::draw( uint_ptr hdc, int x, int y, position* clip )
{
	position pos = m_pos;
	pos.x	+= x;
	pos.y	+= y;

	position el_pos = pos;
	el_pos += get_margins();

	if(el_pos.does_intersect(clip))
	{
		position bg_draw_pos = pos;
		switch(m_bg.m_clip)
		{
		case background_box_padding:
			bg_draw_pos += m_padding;
			break;
		case background_box_border:
			bg_draw_pos += m_padding;
			bg_draw_pos += m_borders;
			break;
		}

		if(m_bg.m_color.alpha)
		{
			m_doc->get_painter()->fill_rect(hdc, bg_draw_pos, m_bg.m_color);
		}
		if(!m_bg.m_image.empty())
		{
			m_doc->get_painter()->draw_background(hdc, 
				m_bg.m_image.c_str(), 
				m_bg.m_baseurl.c_str(), 
				bg_draw_pos, 
				m_bg.m_position,
				m_bg.m_repeat, 
				m_bg.m_attachment);
		}
	}

	if(m_display == display_list_item && m_list_style_type != list_style_type_none)
	{
		int marker_x		= pos.x;
		int marker_height	= get_font_size();
		if(m_list_style_position == list_style_position_outside)
		{
			marker_x -= marker_height;
		}
		litehtml::web_color color = get_color(L"color", true, web_color(0, 0, 0));
		m_doc->get_painter()->draw_list_marker(hdc, m_list_style_type, marker_x, pos.y, marker_height, color);
	}

	if(pos.does_intersect(clip))
	{
		draw_content(hdc, pos);
	}

	for(elements_vector::iterator i = m_inlines.begin(); i != m_inlines.end(); i++)
	{
		element* el = (*i);
		if(el->m_float == float_none && !el->m_skip)
		{
			el->draw(hdc, pos.left(), pos.top(), clip);
		}
	}

	for(elements_vector::iterator i = m_floats.begin(); i != m_floats.end(); i++)
	{
		element* el = (*i);
		position el_pos;
		el->get_abs_position(el_pos, this);
		el_pos.x = el_pos.x + pos.x - el->m_pos.x;
		el_pos.y = el_pos.y + pos.y - el->m_pos.y;
		el->draw(hdc, el_pos.left(), el_pos.top(), clip);
	}

	for(elements_vector::iterator i = m_absolutes.begin(); i != m_absolutes.end(); i++)
	{
		element* el = (*i);
		el->draw(hdc, pos.left(), pos.top(), clip);
	}
}

litehtml::uint_ptr litehtml::element::get_font()
{
	const wchar_t* name			= get_style_property(L"font-family",		true,	L"inherit");
	const wchar_t* size			= get_style_property(L"font-size",			true,	L"medium");
	const wchar_t* weight		= get_style_property(L"font-weight",		true,	L"normal");
	const wchar_t* style		= get_style_property(L"font-style",			true,	L"normal");
	const wchar_t* decoration	= get_style_property(L"text-decoration",	true,	L"none");

	return m_doc->get_font(name, size, weight, style, decoration);
}

const wchar_t* litehtml::element::get_style_property( const wchar_t* name, bool inherited, const wchar_t* def /*= 0*/ )
{
	const wchar_t* ret = m_style.get_property(name);
	bool pass_parent = false;
	if(m_parent)
	{
		if(ret && !_wcsicmp(ret, L"inherit"))
		{
			pass_parent = true;
		} else if(!ret && inherited)
		{
			pass_parent = true;
		}
	}
	if(pass_parent)
	{
		ret = m_parent->get_style_property(name, inherited, def);
	}

	if(!ret)
	{
		ret = def;
	}

	return ret;
}

litehtml::web_color litehtml::element::get_color( const wchar_t* prop_name, bool inherited, const litehtml::web_color& def_color )
{
	const wchar_t* clrstr = get_style_property(prop_name, inherited, 0);
	if(!clrstr)
	{
		return def_color;
	}
	return web_color::from_string(clrstr);
}

void litehtml::element::draw_content( uint_ptr hdc, const litehtml::position& pos )
{
}

void litehtml::element::parse_styles()
{
	m_id	= get_attr(L"id", L"");
	m_class	= get_attr(L"class", L"");

	const wchar_t* style = get_attr(L"style");

	if(style)
	{
		m_style.add(style, NULL);
	}

	int fntsize = get_font_size();

	m_el_position	= (element_position)	value_index(get_style_property(L"position",		false,	L"static"), element_position_strings,	element_position_fixed);
	m_text_align	= (text_align)			value_index(get_style_property(L"text-align",	true,	L"left"),	text_align_strings,			text_align_left);

	const wchar_t* va	= get_style_property(L"vertical-align", true,	L"baseline");
	m_vertical_align = (vertical_align) value_index(va, vertical_align_strings, va_baseline);

	const wchar_t* fl	= get_style_property(L"float", false,	L"none");
	m_float = (element_float) value_index(fl, element_float_strings, float_none);

	m_clear = (element_clear) value_index(get_style_property(L"clear", false, L"none"), element_clear_strings, clear_none);

	if(m_el_position == element_position_absolute)
	{
		m_display = display_block;
	} else
	{
		const wchar_t* display	= get_style_property(L"display", false,	L"inline");
		m_display = (style_display) value_index(display, style_display_strings, display_inline);
	}

	m_css_width.fromString(		get_style_property(L"width",			false,	L"auto"), L"auto");
	m_css_height.fromString(	get_style_property(L"height",			false,	L"auto"), L"auto");

	m_css_left.fromString(		get_style_property(L"left",				false,	L"auto"), L"auto");
	m_css_right.fromString(		get_style_property(L"right",			false,	L"auto"), L"auto");
	m_css_top.fromString(		get_style_property(L"top",				false,	L"auto"), L"auto");
	m_css_bottom.fromString(	get_style_property(L"bottom",			false,	L"auto"), L"auto");

	m_css_margins.left.fromString(		get_style_property(L"margin-left",			false,	L"0"), L"auto");
	m_css_margins.right.fromString(		get_style_property(L"margin-right",			false,	L"0"), L"auto");
	m_css_margins.top.fromString(		get_style_property(L"margin-top",			false,	L"0"), L"auto");
	m_css_margins.bottom.fromString(	get_style_property(L"margin-bottom",		false,	L"0"), L"auto");

	m_css_padding.left.fromString(		get_style_property(L"padding-left",			false,	L"0"), L"");
	m_css_padding.right.fromString(		get_style_property(L"padding-right",		false,	L"0"), L"");
	m_css_padding.top.fromString(		get_style_property(L"padding-top",			false,	L"0"), L"");
	m_css_padding.bottom.fromString(	get_style_property(L"padding-bottom",		false,	L"0"), L"");

	m_css_borders.left.fromString(		get_style_property(L"border-left-width",	false,	L"medium"), border_width_strings);
	m_css_borders.right.fromString(		get_style_property(L"border-right-width",	false,	L"medium"), border_width_strings);
	m_css_borders.top.fromString(		get_style_property(L"border-top-width",		false,	L"medium"), border_width_strings);
	m_css_borders.bottom.fromString(	get_style_property(L"border-bottom-width",	false,	L"medium"), border_width_strings);

	m_margins.left		= m_doc->cvt_units(m_css_margins.left,		fntsize);
	m_margins.right		= m_doc->cvt_units(m_css_margins.right,		fntsize);
	m_margins.top		= m_doc->cvt_units(m_css_margins.top,		fntsize);
	m_margins.bottom	= m_doc->cvt_units(m_css_margins.bottom,	fntsize);

	m_padding.left		= m_doc->cvt_units(m_css_padding.left,		fntsize);
	m_padding.right		= m_doc->cvt_units(m_css_padding.right,		fntsize);
	m_padding.top		= m_doc->cvt_units(m_css_padding.top,		fntsize);
	m_padding.bottom	= m_doc->cvt_units(m_css_padding.bottom,	fntsize);

	m_borders.left		= m_doc->cvt_units(m_css_borders.left,		fntsize);
	m_borders.right		= m_doc->cvt_units(m_css_borders.right,		fntsize);
	m_borders.top		= m_doc->cvt_units(m_css_borders.top,		fntsize);
	m_borders.bottom	= m_doc->cvt_units(m_css_borders.bottom,	fntsize);

	if(m_display == display_list_item)
	{
		const wchar_t* list_type = get_style_property(L"list-style-type", true, L"disc");
		m_list_style_type = (list_style_type) value_index(list_type, list_style_type_strings, list_style_type_disc);

		const wchar_t* list_pos = get_style_property(L"list-style-position", true, L"outside");
		m_list_style_position = (list_style_position) value_index(list_pos, list_style_position_strings, list_style_position_outside);
	}

	parse_background();

	for(elements_vector::iterator i = m_children.begin(); i != m_children.end(); i++)
	{
		(*i)->parse_styles();
	}

	find_inlines();
}

int litehtml::element::render_table( uint_ptr hdc, int x, int y, int max_width )
{
	m_pos.move_to(x, y);
	m_pos.x	+= content_margins_left();
	m_pos.y += content_margins_top();

	int block_width = m_css_width.calc_percent(max_width);

	if(block_width)
	{
		max_width = block_width;
	} else
	{
		max_width -= content_margins_left() + content_margins_right();
	}

	table_grid grid;

	elements_iterator row_iter(this, &go_inside_table(), &table_rows_selector());

	element* row = row_iter.next();
	while(row)
	{
		grid.begin_row();

		elements_iterator cell_iter(row, &go_inside_table(), &table_cells_selector());
		element* cell = cell_iter.next();
		while(cell)
		{
			grid.add_cell(cell);

			cell = cell_iter.next();
		}
		row = row_iter.next();
		grid.end_row();
	}

	// full width render
	int row_idx = 0;
	int col_idx = 0;
	for(table_grid::rows::iterator row = grid.m_rows.begin(); row != grid.m_rows.end(); row++, row_idx++)
	{
		col_idx = 0;
		for(table_grid::row::iterator cell = row->begin(); cell != row->end(); cell++, col_idx++)
		{
			if(cell->el)
			{
				int width = cell->el->render(hdc, 0, 0, max_width);
				grid.m_cols_width[col_idx] = max(grid.m_cols_width[col_idx], width);
			}
		}
	}

	// find the table width
	int width = 0;
	int max_col_width = 0;
	for(int i=0; i < (int) grid.m_cols_width.size(); i++)
	{
		width += grid.m_cols_width[i];
		max_col_width = max(max_col_width, grid.m_cols_width[i]);
	}


	// adjust the columns width
	if(width > max_width && max_col_width)
	{
		int mw = max_width;
		max_width = 0;

		for(int i=0; i < (int) grid.m_cols_width.size(); i++)
		{
			grid.m_cols_width[i] = (int) ((double) grid.m_cols_width[i] * (double) mw / (double) width);
			max_width += grid.m_cols_width[i];
		}
	}


	int top = 0;
	int left = 0;

	// render cells with computed width

	row_idx = 0;
	for(table_grid::rows::iterator row = grid.m_rows.begin(); row != grid.m_rows.end(); row++, row_idx++)
	{
		col_idx = 0;
		left = 0;
		for(table_grid::row::iterator cell = row->begin(); cell != row->end(); cell++, col_idx++)
		{
			if(cell->el || cell == row->begin())
			{
				int w = grid.m_cols_width[col_idx];
				for(int i = 1; i < cell->colspan && i + col_idx < (int) grid.m_cols_width.size(); i++)
				{
					w += grid.m_cols_width[col_idx + i];
				}
				if(cell->el)
				{
					cell->el->render(hdc, left, top, w);
					if(cell->rowspan == 1)
					{
						grid.m_rows_height[row_idx] = max(grid.m_rows_height[row_idx], cell->el->height());
					}
				}
			}
			left += grid.m_cols_width[col_idx];
		}
		top += grid.m_rows_height[row_idx];
	}

	// set the rows height

	bool need_second_pass = false;

	row_idx = 0;
	for(table_grid::rows::iterator row = grid.m_rows.begin(); row != grid.m_rows.end(); row++, row_idx++)
	{
		col_idx = 0;
		for(table_grid::row::iterator cell = row->begin(); cell != row->end(); cell++, col_idx++)
		{
			if(cell->el)
			{
				if(cell->rowspan == 1)
				{
					cell->el->m_pos.height = grid.m_rows_height[row_idx];
				} else
				{
					int h = 0;
					int last_row = row_idx;
					for(int i = 0; i + row_idx < (int) grid.m_rows_height.size() && i < cell->rowspan; i++)
					{
						h += grid.m_rows_height[row_idx + i];
						last_row = row_idx + i;
					}
					if(cell->el->m_pos.height > h)
					{
						need_second_pass = true;
						grid.m_rows_height[last_row] += cell->el->m_pos.height - h;
					} else
					{
						cell->el->m_pos.height = h;
					}
				}
			}
		}
	}

	if(need_second_pass)
	{
		// re-pos cells cells with computed width
		row_idx = 0;
		top		= 0;
		for(table_grid::rows::iterator row = grid.m_rows.begin(); row != grid.m_rows.end(); row++, row_idx++)
		{
			col_idx = 0;
			for(table_grid::row::iterator cell = row->begin(); cell != row->end(); cell++, col_idx++)
			{
				if(cell->el)
				{
					int h = 0;
					for(int i = 0; i + row_idx < (int) grid.m_rows_height.size() && i < cell->rowspan; i++)
					{
						h += grid.m_rows_height[row_idx + i];
					}
					cell->el->m_pos.height	= h;
					cell->el->m_pos.y		= top;						
				}
			}
			top += grid.m_rows_height[row_idx];
		}
	}


	m_pos.width		= max_col_width;
	m_pos.height	= top;

	return max_col_width;
}

int litehtml::element::render( uint_ptr hdc, int x, int y, int max_width )
{
	int parent_width = max_width;

	m_pos.move_to(x, y);

/*
	if(m_css_margins.left.is_predefined() || !m_css_margins.left.is_predefined() && m_css_margins.left.units() == css_units_percentage)
	{
		m_margins.left = 0;
	}
	if(m_css_margins.right.is_predefined() || !m_css_margins.right.is_predefined() && m_css_margins.right.units() == css_units_percentage)
	{
		m_margins.right = 0;
	}
*/

	m_pos.x	+= content_margins_left();
	m_pos.y += content_margins_top();

	int ret_width = 0;

	int block_width = m_css_width.calc_percent(parent_width);

	if(block_width)
	{
		ret_width = max_width = block_width;
	} else
	{
		if(max_width)
		{
			max_width -= content_margins_left() + content_margins_right();
		}
	}

	m_lines.clear();
	m_floats.clear();

	line ln;
	set_line_left(ln, 0);

	int max_x = get_line_right(0, max_width);

	for(elements_vector::iterator i = m_inlines.begin(); i != m_inlines.end(); i++)
	{
		element* el = (*i);

		if(el->m_float == float_left)
		{
			int rw = el->render(hdc, get_line_left(ln.get_top()), ln.get_top(), max_x);
			if(el->right() > max_x)
			{
				int new_top = find_next_line_top(el->top(), el->width());
				el->m_pos.x = get_line_left(new_top) + el->content_margins_left();
				el->m_pos.y = new_top + el->content_margins_top();
			}
			ret_width = max(ret_width, rw);
			add_float(el);
			fix_line_width(ln, max_width);
		} else if(el->m_float == float_right)
		{
			int rw = el->render(hdc, 0, ln.get_top(), max_x);
			if(ln.get_left() + el->width() > max_x)
			{
				int new_top = find_next_line_top(el->top(), el->width());
				el->m_pos.x = get_line_right(new_top, max_x) - (el->width() - el->content_margins_left());
				el->m_pos.y = new_top + el->content_margins_top();
			} else
			{
				el->m_pos.x = max_x - el->width();
			}
			ret_width = max(ret_width, rw);
			add_float(el);
			fix_line_width(ln, max_width);
			max_x = get_line_right(ln.get_top(), max_x);
		} else
		{
			switch(el->m_display)
			{
			case display_table:
				{
					if(!ln.empty())
					{
						max_x = add_line(ln, max_x);
					}
					int rw = el->render_table(hdc, ln.get_left(), ln.get_top(), max_x);
					ln += el;
					max_x = add_line(ln, max_width);
					ret_width = max(ret_width, rw);
				}
				break;
			case display_list_item:
				{
					if(!ln.empty())
					{
						max_x = add_line(ln, max_x);
					}
					int rw = el->render(hdc, ln.get_left(), ln.get_top(), max_x);
					ln += el;
					max_x = add_line(ln, max_width);
					ret_width = max(ret_width, rw);
				}
				break;
			case display_block:
				{
					if(!ln.empty())
					{
						max_x = add_line(ln, max_x);
					}
					ln.set_left(0);
					if(set_line_clear(ln, el->m_clear))
					{
						max_x = get_line_right(ln.get_top(), max_width);
					}
					int rw = el->render(hdc, ln.get_left(), ln.get_top(), max_x);
					ln += el;
					max_x = add_line(ln, max_width);
					ret_width = max(ret_width, rw);
				}
				break;
			case display_inline_block:
				{
					el->render(hdc, ln.get_left(), ln.get_top(), max_x);

					int oldTop = ln.get_top();
					max_x = place_inline(el, ln, max_x, max_width);
					if(oldTop != ln.get_top())
					{
						max_x = get_line_right(ln.get_top(), max_width);
					}
					ret_width = max(ret_width, el->right());
				}
				break;
			case display_inline:
				{
					litehtml::size sz;
					el->get_content_size(hdc, sz, max_x);
					el->m_pos = sz;
					int oldTop = ln.get_top();
					max_x = place_inline(el, ln, max_x, max_width);
					if(oldTop != ln.get_top())
					{
						max_x = get_line_right(ln.get_top(), max_width);
					}
					ret_width = max(ret_width, el->right());
				}
				break;
			}
		}
	}
	if(!ln.empty())
	{
		add_line(ln, max_width);
	}

	m_pos.width		= max_width;

	m_pos.move_to(x, y);

	// calculate outlines

	m_padding.left	= m_css_padding.left.calc_percent(parent_width);
	m_padding.right	= m_css_padding.right.calc_percent(parent_width);

	m_borders.left	= m_css_borders.left.calc_percent(parent_width);
	m_borders.right	= m_css_borders.right.calc_percent(parent_width);

	m_margins.left	= m_css_margins.left.calc_percent(parent_width);
	m_margins.right	= m_css_margins.right.calc_percent(parent_width);

	m_margins.top		= m_css_margins.top.calc_percent(0);
	m_margins.bottom	= m_css_margins.bottom.calc_percent(0);

	if(m_display == display_block)
	{
		if(m_css_margins.left.is_predefined() && m_css_margins.right.is_predefined())
		{
			int el_width = m_pos.width + m_borders.left + m_borders.right + m_padding.left + m_padding.right;
			m_margins.left	= (parent_width - el_width) / 2;
			m_margins.right	= (parent_width - el_width) - m_margins.left;
		} else if(m_css_margins.left.is_predefined() && !m_css_margins.right.is_predefined())
		{
			int el_width = m_pos.width + m_borders.left + m_borders.right + m_padding.left + m_padding.right + m_margins.right;
			m_margins.left	= parent_width - el_width;
		} else if(!m_css_margins.left.is_predefined() && m_css_margins.right.is_predefined())
		{
			int el_width = m_pos.width + m_borders.left + m_borders.right + m_padding.left + m_padding.right + m_margins.left;
			m_margins.right	= parent_width - el_width;
		}
	}

	//

	if(!m_lines.empty())
	{
		if(!m_padding.top && !m_borders.top && m_lines.front().collapse_top_margin())
		{
			m_margins.top		= max(m_margins.top,	m_lines.front().get_margin_top());
		} else
		{
			for(line::vector::iterator ln = m_lines.begin(); ln != m_lines.end(); ln++)
			{
				ln->add_top(m_lines.front().get_margin_top());
			}
		}
		m_pos.height = m_lines.back().get_top() + m_lines.back().get_height() - m_lines.back().get_margin_bottom() - m_lines.front().get_margin_top();

		if(!m_padding.bottom && !m_borders.bottom && m_lines.back().collapse_bottom_margin())
		{
			m_margins.bottom	= max(m_margins.bottom,	m_lines.back().get_margin_bottom());
		} else
		{
			m_pos.height	+= m_lines.back().get_margin_bottom();
		}
	}

	if(is_floats_holder())
	{
		int floats_height = get_floats_height();
		if(floats_height > m_pos.height)
		{
			m_pos.height = floats_height;
		}
	}


	m_pos.x	+= content_margins_left();
	m_pos.y += content_margins_top();

	// TODO: Percents are incorrect
	int block_height = m_css_height.calc_percent(100);

	if(block_height)
	{
		m_pos.height = block_height;
	}

	ret_width += content_margins_left() + content_margins_right();

	if((	m_display == display_inline_block						|| 
			(m_float != float_none && m_css_width.is_predefined())	|| 
			m_el_position == element_position_absolute ) 
			
			&& ret_width < max_width)
	{
		render(hdc, x, y, ret_width);
		m_pos.width = ret_width - (content_margins_left() + content_margins_right());
	}

	int abs_base_x	= - m_padding.left - m_borders.left;
	int abs_base_y	= - m_padding.top - m_borders.top;

	int abs_base_width	= m_pos.width + m_padding.left + m_borders.left + m_padding.right + m_borders.right;
	int abs_base_height	= m_pos.height + m_padding.top + m_borders.top + m_padding.bottom + m_borders.bottom;

	for(elements_vector::iterator abs_el = m_absolutes.begin(); abs_el != m_absolutes.end(); abs_el++)
	{
		element* el = (*abs_el);

		int left	= el->m_css_left.calc_percent(abs_base_width);
		int right	= m_pos.width - el->m_css_right.calc_percent(abs_base_width);
		int top		= el->m_css_top.calc_percent(abs_base_height);
		int bottom	= m_pos.height - el->m_css_bottom.calc_percent(abs_base_height);
		el->render(hdc, left, top, right - left);

		if(el->m_css_right.is_predefined())
		{
			el->m_pos.x = left - m_padding.left - m_borders.left;
		} else
		{
			if(el->m_css_left.is_predefined())
			{
				el->m_pos.x = right - el->width();
			} else
			{
				el->m_pos.x = left - m_padding.left - m_borders.left;
				el->m_pos.width = right - left;
			}
		}

		if(el->m_css_bottom.is_predefined())
		{
			el->m_pos.y = top - m_padding.top - m_borders.top;
		} else
		{
			if(el->m_css_top.is_predefined())
			{
				el->m_pos.y = bottom - el->height();
			} else
			{
				el->m_pos.y = top - m_padding.top - m_borders.top;
				el->m_pos.height = bottom - top;
			}
		}
	}

	return ret_width;
}

bool litehtml::element::is_white_space()
{
	return false;
}

int litehtml::element::get_font_size()
{
	return m_doc->cvt_font_size(get_style_property(L"font-size", true,	L"medium"));
}

void litehtml::element::clear_inlines()
{
	m_inlines.clear();
}

int litehtml::element::add_line( line& ln, int max_width )
{
	int top = ln.get_top();

	if(!m_lines.empty() && ln.get_clear_floats() == clear_none)
	{
		top = m_lines.back().get_top() + m_lines.back().get_height() - min(ln.get_margin_top(), m_lines.back().get_margin_bottom());
	}

	ln.set_top(top, this);

	int ln_top = 0;

	if(ln.finish())
	{
		m_lines.push_back(ln);
		ln_top = ln.get_top() + ln.get_height()/* + 1*/;
	} else
	{
		ln_top = ln.get_top();
	}


	ln.clear();
	set_line_left(ln, ln_top);
	ln.set_top(ln_top, this);
	return get_line_right(ln.get_top(), max_width);
}

int litehtml::element::get_base_line()
{
	uint_ptr font = get_font();
	return m_doc->get_painter()->get_text_base_line(0, font);
}

void litehtml::element::find_inlines()
{
	clear_inlines();
	elements_iterator iter(this, &go_inside_inline(), 0);
	element* el = iter.next();
	while(el)
	{
		bool add = true;

		if(el->m_el_position == element_position_absolute)
		{
			add_absolute(el);
			add = false;
		} else
		{
			switch(m_display)
			{
			case display_none:
				add = false;
				break;
			case display_table_row:
				if(	el->m_display != display_table_cell)
				{
					add = false;
				}
				break;
			case display_table_row_group:
			case display_table_header_group:
			case display_table_footer_group:
				if(	el->m_display != display_table_row)
				{
					add = false;
				}
				break;
			case display_table:
				if(	el->m_display != display_table_row_group &&
					el->m_display != display_table_header_group &&
					el->m_display != display_table_footer_group &&
					el->m_display != display_table_caption)
				{
					add = false;
				}
				break;
			}
		}
		if(add)
		{
			m_inlines.push_back(el);
		}
		el = iter.next();
	}
}

bool litehtml::element::operator==( const css_selector& selector )
{
	if(*this != selector.m_right)
	{
		return false;
	}
	if(selector.m_left)
	{
		if(!m_parent)
		{
			return false;
		}
		switch(selector.m_combinator)
		{
		case combinator_descendant:
			if(!find_ancestor(*selector.m_left))
			{
				return false;
			}
			break;
		case combinator_child:
			if(*m_parent != *selector.m_left)
			{
				return false;
			}
			break;
// TODO: add combinator_adjacent_sibling and combinator_general_sibling handling
		}
	}
	return true;
}

bool litehtml::element::operator==( const css_element_selector& selector )
{
	if(!selector.m_tag.empty() && selector.m_tag != L"*")
	{
		if(selector.m_tag != m_tag)
		{
			return false;
		}
	}

	for(css_attribute_selector::map::const_iterator i = selector.m_attrs.begin(); i != selector.m_attrs.end(); i++)
	{
		const wchar_t* attr_value = get_attr(i->first.c_str());
		switch(i->second.condition)
		{
		case select_exists:
			if(!attr_value)
			{
				return false;
			}
			break;
		case select_equal:
			if(!attr_value)
			{
				return false;
			} else 
			{
				if(i->first == L"class")
				{
					string_vector tokens;
					tokenize(attr_value, tokens, L" ");
					bool found = false;
					for(string_vector::iterator str = tokens.begin(); str != tokens.end() && !found; str++)
					{
						if(*str == i->second.val)
						{
							found = true;
						}
					}
					if(!found)
					{
						return false;
					}
				} else
				{
					if(i->second.val != attr_value)
					{
						return false;
					}
				}
			}
			break;
		case select_contain_str:
			if(!attr_value)
			{
				return false;
			} else if(!wcsstr(attr_value, i->second.val.c_str()))
			{
				return false;
			}
			break;
		case select_start_str:
			if(!attr_value)
			{
				return false;
			} else if(wcsncmp(attr_value, i->second.val.c_str(), i->second.val.length()))
			{
				return false;
			}
			break;
		case select_end_str:
			if(!attr_value)
			{
				return false;
			} else if(wcsncmp(attr_value, i->second.val.c_str(), i->second.val.length()))
			{
				const wchar_t* s = attr_value + wcslen(attr_value) - i->second.val.length() - 1;
				if(s < attr_value)
				{
					return false;
				}
				if(i->second.val != s)
				{
					return false;
				}
			}
			break;
		}
	}
	return true;
}

litehtml::element* litehtml::element::find_ancestor( const css_selector& selector )
{
	if(!m_parent)
	{
		return false;
	}
	if(*m_parent == selector)
	{
		return m_parent;
	}
	return m_parent->find_ancestor(selector);
}

int litehtml::element::get_floats_height() const
{
	if(is_floats_holder())
	{
		int h = 0;
		if(!m_floats.empty())
		{
			for(elements_vector::const_iterator i = m_floats.begin(); i != m_floats.end(); i++)
			{
				element::ptr el = (*i);

				position el_pos;
				el->get_abs_position(el_pos, this);
				el_pos += el->m_margins;
				el_pos += el->m_padding;
				el_pos += el->m_borders;

				h = max(h, el_pos.bottom());
			}
		}
		return h;
	}
	int h = m_parent->get_floats_height();
	return h - m_pos.y;
}

int litehtml::element::get_left_floats_height() const
{
	if(is_floats_holder())
	{
		int h = 0;
		if(!m_floats.empty())
		{
			for(elements_vector::const_iterator i = m_floats.begin(); i != m_floats.end(); i++)
			{
				element::ptr el = (*i);
				if(el->m_float == float_left)
				{
					position el_pos;
					el->get_abs_position(el_pos, this);
					el_pos += el->m_margins;
					el_pos += el->m_padding;
					el_pos += el->m_borders;

					h = max(h, el_pos.bottom());
				}
			}
		}
		return h;
	}
	int h = m_parent->get_left_floats_height();
	return h - m_pos.y;
}

int litehtml::element::get_right_floats_height() const
{
	if(is_floats_holder())
	{
		int h = 0;
		if(!m_floats.empty())
		{
			for(elements_vector::const_iterator i = m_floats.begin(); i != m_floats.end(); i++)
			{
				element::ptr el = (*i);
				if(el->m_float == float_right)
				{
					position el_pos;
					el->get_abs_position(el_pos, this);
					el_pos += el->m_margins;
					el_pos += el->m_padding;
					el_pos += el->m_borders;

					h = max(h, el_pos.bottom());
				}
			}
		}
		return h;
	}
	int h = m_parent->get_left_floats_height();
	return h - m_pos.y;
}

int litehtml::element::get_line_left( int y ) const
{
	if(is_floats_holder())
	{
		int w = 0;
		for(elements_vector::const_iterator i = m_floats.begin(); i != m_floats.end(); i++)
		{
			element::ptr el = (*i);
			if(el->m_float == float_left)
			{
				position el_pos;
				el->get_abs_position(el_pos, this);
				el_pos += el->m_margins;
				el_pos += el->m_padding;
				el_pos += el->m_borders;

				if(y >= el_pos.top() && y < el_pos.bottom())
				{
					w = max(w, el_pos.right());
				}
			}
		}
		return w;
	}
	int w = m_parent->get_line_left(y + m_pos.y);
	if(w < 0)
	{
		w = 0;
	}
	return w - (w ? m_pos.x : 0);
}

int litehtml::element::get_line_right( int y, int def_right ) const
{
	if(is_floats_holder())
	{
		int w = def_right;
		for(elements_vector::const_iterator i = m_floats.begin(); i != m_floats.end(); i++)
		{
			element::ptr el = (*i);
			if(el->m_float == float_right)
			{
				position el_pos;
				el->get_abs_position(el_pos, this);
				el_pos += el->m_margins;
				el_pos += el->m_padding;
				el_pos += el->m_borders;

				if(y >= el_pos.top() && y < el_pos.bottom())
				{
					w = min(w, el_pos.left());
				}
			}
		}
		return w;
	}
	int w = m_parent->get_line_right(y + m_pos.y, def_right + m_pos.x);
	return w - m_pos.x;
}

void litehtml::element::fix_line_width( line& ln, int max_width )
{
	int left	= get_line_left(ln.get_top());
	int right	= get_line_right(ln.get_top(), max_width);

	elements_vector els;
	ln.get_elements(els);

	line ln_new;
	ln_new.set_top(ln.get_top(), this);
	ln_new.set_left(left);

	for(elements_vector::iterator i = els.begin(); i != els.end(); i++)
	{
		object_ptr<element> el = (*i);
		place_inline(el, ln_new, right, max_width);
	}

	ln = ln_new;


/*
	int left	= get_line_left(ln.get_top());
	int right	= get_line_right(ln.get_top(), max_width);

	int width = max_width;
	elements_vector els;
	ln.fix_width(left, width, els);
	if(!els.empty())
	{
		width = add_line(ln, max_width);
		for (elements_vector::iterator i = els.begin(); i != els.end(); i++)
		{
			object_ptr<element> el = (*i);
			if(!ln.empty() && ln.get_left() + el->width() > width)
			{
				width = add_line(ln, max_width);
			} else
			{
				el->m_pos.x = ln.get_left() + el->content_margins_left();
			}
			ln += el;
		}
		if(!ln.empty())
		{
			width = add_line(ln, max_width);
		}
	}
*/
}

void litehtml::element::set_line_left(line& ln, int top)
{
	int left = 0;
	if(m_list_style_position == list_style_position_inside && m_display == display_list_item)
	{
		left = get_font_size();
	}
	int ll = get_line_left(top);
	if(ll >= content_margins_left())
	{
		left += get_line_left(top);
	}

	ln.set_left(left);
}

void litehtml::element::add_float( element* el )
{
	if(is_floats_holder())
	{
		if(el->m_float == float_left || el->m_float == float_right)
		{
			m_floats.push_back(el);
		}
	} else
	{
		m_parent->add_float(el);
	}
}

void litehtml::element::get_abs_position( position& pos, const element* root )
{
	if(root == m_parent || !m_parent)
	{
		pos = m_pos;
	} else
	{
		position parent_pos;
		m_parent->get_abs_position(parent_pos, root);
		pos = m_pos;
		pos.x += parent_pos.x;
		pos.y += parent_pos.y;
	}
}

bool litehtml::element::set_line_clear( line& ln, element_clear clr )
{
	switch(clr)
	{
	case clear_left:
		{
			int fh = get_left_floats_height();
			if(fh && fh > ln.get_top())
			{
				ln.set_top(fh, this);
				return true;
			}
		}
		break;
	case clear_right:
		{
			int fh = get_right_floats_height();
			if(fh && fh > ln.get_top())
			{
				ln.set_top(fh, this);
				return true;
			}
		}
		break;
	case clear_both:
		{
			int fh = get_floats_height();
			if(fh && fh > ln.get_top())
			{
				ln.set_top(fh, this);
				return true;
			}
		}
		break;
	}
	return false;
}

int litehtml::element::place_inline( element* el, line& ln, int max_x, int max_right )
{
	int ret = max_x;
	if(ln.get_left() + el->width() > max_x && !(el->is_white_space() && ln.is_white_space()))
	{
		if(!ln.empty())
		{
			ret = add_line(ln, max_right);
			return place_inline(el, ln, ret, max_right);
		} else
		{
			int new_top = find_next_line_top(ln.get_top(), el->width());
			if(new_top != ln.get_top())
			{
				ln.set_top(new_top, this);
				set_line_left(ln, ln.get_top());
				ret = get_line_right(ln.get_top(), max_x);
			}
		}
	}
	el->m_pos.y = ln.get_top() + el->content_margins_top();
	el->m_pos.x = ln.get_left() + el->content_margins_left();
	ln += el;

	return ret;
}

int litehtml::element::find_next_line_top( int top, int width )
{
	if(is_floats_holder())
	{
		int max_top = top;
		int new_top = 0;
		bool new_top_isvalid = false;
		for(elements_vector::const_iterator i = m_floats.begin(); i != m_floats.end(); i++)
		{
			element::ptr el = (*i);
			position el_pos;
			el->get_abs_position(el_pos, this);
			el_pos += el->m_margins;
			el_pos += el->m_padding;
			el_pos += el->m_borders;

			max_top = max(max_top, el_pos.bottom());

			if(top >= el_pos.top() && top < el_pos.bottom())
			{
				int pos_left = get_line_left(el_pos.top());
				int pos_right = get_line_right(el_pos.top(), m_pos.width);
				if(pos_right - pos_left >= width)
				{
					if(new_top_isvalid)
					{
						new_top = min(new_top, el_pos.top());
					} else
					{
						new_top = el_pos.top();
						new_top_isvalid = true;
					}
				}

				pos_left = get_line_left(el_pos.bottom());
				pos_right = get_line_right(el_pos.bottom(), m_pos.width);
				if(pos_right - pos_left >= width)
				{
					if(new_top_isvalid)
					{
						new_top = min(new_top, el_pos.bottom());
					} else
					{
						new_top = el_pos.bottom();
						new_top_isvalid = true;
					}
				}
			}
		}

		if(!new_top_isvalid)
		{
			new_top = max_top;
		}
		return new_top;
	}
	int new_top = m_parent->find_next_line_top(top + m_pos.y, width);
	return new_top - m_pos.y;
}

void litehtml::element::parse_background()
{
	// parse background-color
	m_bg.m_color		= get_color(L"background-color", false, web_color(0, 0, 0, 0));

	// parse background-position
	const wchar_t* str = get_style_property(L"background-position", false, L"0% 0%");
	if(str)
	{
		string_vector res;
		tokenize(str, res, L" \t");
		if(res.size() > 0)
		{
			m_bg.m_position.x.fromString(res[0], L"left;right;center");
			if(res.size() > 1)
			{
				m_bg.m_position.y.fromString(res[1], L"top;bottom;center");
			} else
			{
				m_bg.m_position.x.set_value(50, css_units_percentage);
			}
			if(m_bg.m_position.x.is_predefined())
			{
				switch(m_bg.m_position.x.predef())
				{
				case 0:
					m_bg.m_position.x.set_value(0, css_units_percentage);
					break;
				case 1:
					m_bg.m_position.x.set_value(100, css_units_percentage);
					break;
				case 2:
					m_bg.m_position.x.set_value(50, css_units_percentage);
					break;
				}
			}
			if(m_bg.m_position.y.is_predefined())
			{
				switch(m_bg.m_position.y.predef())
				{
				case 0:
					m_bg.m_position.y.set_value(0, css_units_percentage);
					break;
				case 1:
					m_bg.m_position.y.set_value(100, css_units_percentage);
					break;
				case 2:
					m_bg.m_position.y.set_value(50, css_units_percentage);
					break;
				}
			}
		} else
		{
			m_bg.m_position.x.set_value(0, css_units_percentage);
			m_bg.m_position.y.set_value(0, css_units_percentage);
		}
	} else
	{
		m_bg.m_position.y.set_value(0, css_units_percentage);
		m_bg.m_position.x.set_value(0, css_units_percentage);
	}

	// parse background_attachment
	m_bg.m_attachment = (background_attachment) value_index(
		get_style_property(L"background-attachment", false, L"scroll"), 
		background_attachment_strings, 
		background_attachment_scroll);

	// parse background_attachment
	m_bg.m_repeat = (background_repeat) value_index(
		get_style_property(L"background-repeat", false, L"repeat"), 
		background_repeat_strings, 
		background_repeat_repeat);

	// parse background_clip
	m_bg.m_clip = (background_box) value_index(
		get_style_property(L"background-clip", false, L"border-box"), 
		background_box_strings, 
		background_box_border);

	// parse background_origin
	m_bg.m_origin = (background_box) value_index(
		get_style_property(L"background-origin", false, L"content-box"), 
		background_box_strings, 
		background_box_content);

	// parse background-image
	parse_css_url(get_style_property(L"background-image", false, L""), m_bg.m_image);
	m_bg.m_baseurl = get_style_property(L"background-image-baseurl", false, L"");
}

int litehtml::element::margin_top() const
{
//	if(m_lines.empty() || m_padding.top)
	{
		return m_margins.top;
	}
//	return max(m_lines.front().get_margin_top(), m_margins.top);
}

int litehtml::element::margin_bottom() const
{
//	if(m_lines.empty() || m_padding.bottom)
	{
		return m_margins.bottom;
	}
//	return max(m_lines.front().get_margin_bottom(), m_margins.bottom);
}

int litehtml::element::margin_left() const
{
	return m_margins.left;
}

int litehtml::element::margin_right() const
{
	return m_margins.right;
}

litehtml::margins litehtml::element::get_margins() const
{
	margins ret;
	ret.left	= margin_left();
	ret.right	= margin_right();
	ret.top		= margin_top();
	ret.bottom	= margin_bottom();

	return ret;
}

void litehtml::element::add_absolute( element* el )
{
	if(m_el_position != element_position_static || m_tag == L"body")
	{
		m_absolutes.push_back(el);
	} else if(m_parent)
	{
		m_parent->add_absolute(el);
	}
}
