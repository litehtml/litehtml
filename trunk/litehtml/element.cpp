#include "html.h"
#include "element.h"
#include "tokenizer.h"
#include "document.h"
#include "iterators.h"
#include "stylesheet.h"
#include <string>
#include "table.h"

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

bool litehtml::element::appendChild( litehtml::element* el )
{
	bool add = true;
	if(el->is_white_space())
	{
		if(m_children.empty())
		{
			add = false;
		} else
		{
			if(m_children.back()->is_white_space())
			{
				add = false;
			}
		}
	}
	if(el && add)
	{
		el->m_parent = this;
		m_children.push_back(el);
		return true;
	}
	return false;
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
	el_pos += content_margins();

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
			m_doc->container()->fill_rect(hdc, bg_draw_pos, m_bg.m_color);
		}
		if(!m_bg.m_image.empty())
		{
			m_doc->container()->draw_background(hdc, 
				m_bg.m_image.c_str(), 
				m_bg.m_baseurl.c_str(), 
				bg_draw_pos, 
				m_bg.m_position,
				m_bg.m_repeat, 
				m_bg.m_attachment);
		}

		bg_draw_pos = pos;
		bg_draw_pos += m_padding;
		bg_draw_pos += m_borders;
		m_doc->container()->draw_borders(hdc, m_css_borders, bg_draw_pos);
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
		m_doc->container()->draw_list_marker(hdc, m_list_style_type, marker_x, pos.y, marker_height, color);
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
	const wchar_t* weight		= get_style_property(L"font-weight",		true,	L"normal");
	const wchar_t* style		= get_style_property(L"font-style",			true,	L"normal");
	const wchar_t* decoration	= get_style_property(L"text-decoration",	true,	L"none");

	return m_doc->get_font(name, get_font_size(), weight, style, decoration);
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

	// TODO: remove this nahren
	if(m_class == L"logo")
	{
		int i=0;
		i++;
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

	m_css_borders.left.width.fromString(	get_style_property(L"border-left-width",	false,	L"medium"), border_width_strings);
	m_css_borders.right.width.fromString(	get_style_property(L"border-right-width",	false,	L"medium"), border_width_strings);
	m_css_borders.top.width.fromString(		get_style_property(L"border-top-width",		false,	L"medium"), border_width_strings);
	m_css_borders.bottom.width.fromString(	get_style_property(L"border-bottom-width",	false,	L"medium"), border_width_strings);

	m_css_borders.left.color = web_color::from_string(get_style_property(L"border-left-color",	false,	L""));
	m_css_borders.left.style = (border_style) value_index(get_style_property(L"border-left-style", false, L"none"), border_style_strings, border_style_none);

	m_css_borders.right.color = web_color::from_string(get_style_property(L"border-right-color",	false,	L""));
	m_css_borders.right.style = (border_style) value_index(get_style_property(L"border-right-style", false, L"none"), border_style_strings, border_style_none);

	m_css_borders.top.color = web_color::from_string(get_style_property(L"border-top-color",	false,	L""));
	m_css_borders.top.style = (border_style) value_index(get_style_property(L"border-top-style", false, L"none"), border_style_strings, border_style_none);

	m_css_borders.bottom.color = web_color::from_string(get_style_property(L"border-bottom-color",	false,	L""));
	m_css_borders.bottom.style = (border_style) value_index(get_style_property(L"border-bottom-style", false, L"none"), border_style_strings, border_style_none);

	m_css_borders.radius.top_left_x.fromString(get_style_property(L"border-top-left-radius-x", false, L"0"));
	m_css_borders.radius.top_left_y.fromString(get_style_property(L"border-top-left-radius-y", false, L"0"));

	m_css_borders.radius.top_right_x.fromString(get_style_property(L"border-top-right-radius-x", false, L"0"));
	m_css_borders.radius.top_right_y.fromString(get_style_property(L"border-top-right-radius-y", false, L"0"));

	m_css_borders.radius.bottom_right_x.fromString(get_style_property(L"border-bottom-right-radius-x", false, L"0"));
	m_css_borders.radius.bottom_right_y.fromString(get_style_property(L"border-bottom-right-radius-y", false, L"0"));

	m_css_borders.radius.bottom_left_x.fromString(get_style_property(L"border-bottom-left-radius-x", false, L"0"));
	m_css_borders.radius.bottom_left_y.fromString(get_style_property(L"border-bottom-left-radius-y", false, L"0"));

	m_doc->cvt_units(m_css_borders.radius.bottom_left_x,			fntsize);
	m_doc->cvt_units(m_css_borders.radius.bottom_left_y,			fntsize);
	m_doc->cvt_units(m_css_borders.radius.bottom_right_x,			fntsize);
	m_doc->cvt_units(m_css_borders.radius.bottom_right_y,			fntsize);
	m_doc->cvt_units(m_css_borders.radius.top_left_x,				fntsize);
	m_doc->cvt_units(m_css_borders.radius.top_left_y,				fntsize);
	m_doc->cvt_units(m_css_borders.radius.top_right_x,				fntsize);
	m_doc->cvt_units(m_css_borders.radius.top_right_y,				fntsize);

	m_margins.left		= m_doc->cvt_units(m_css_margins.left,		fntsize);
	m_margins.right		= m_doc->cvt_units(m_css_margins.right,		fntsize);
	m_margins.top		= m_doc->cvt_units(m_css_margins.top,		fntsize);
	m_margins.bottom	= m_doc->cvt_units(m_css_margins.bottom,	fntsize);

	m_padding.left		= m_doc->cvt_units(m_css_padding.left,		fntsize);
	m_padding.right		= m_doc->cvt_units(m_css_padding.right,		fntsize);
	m_padding.top		= m_doc->cvt_units(m_css_padding.top,		fntsize);
	m_padding.bottom	= m_doc->cvt_units(m_css_padding.bottom,	fntsize);

	m_borders.left		= m_doc->cvt_units(m_css_borders.left.width,	fntsize);
	m_borders.right		= m_doc->cvt_units(m_css_borders.right.width,	fntsize);
	m_borders.top		= m_doc->cvt_units(m_css_borders.top.width,		fntsize);
	m_borders.bottom	= m_doc->cvt_units(m_css_borders.bottom.width,	fntsize);

	css_length line_height;
	line_height.fromString(get_style_property(L"line-height",	true,	L"normal"));
	if(line_height.is_predefined())
	{
		line_height.set_value(110, css_units_percentage);
		m_line_height = line_height.calc_percent(fntsize);
	} else if(line_height.units() == css_units_none)
	{
		m_line_height = (int) (line_height.val() * fntsize);
	} else
	{
		m_line_height = line_height.calc_percent(fntsize);
	}


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

int litehtml::element::render( uint_ptr hdc, int x, int y, int max_width )
{
	// TODO: remove this nahren
	if(m_class == L"quicklinks")
	{
		int i=0;
		i++;
	}

	int parent_width = max_width;

	m_pos.move_to(x, y);

	m_pos.x	+= content_margins_left();
	m_pos.y += content_margins_top();

	if(m_el_position == element_position_relative)
	{
		m_pos.x += m_css_left.calc_percent(parent_width);
	}

	int ret_width = 0;

	int block_width = 0;

	if(m_display != display_table_cell)
	{
		block_width = m_css_width.calc_percent(parent_width);
	}

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
	init_line(ln, 0, max_width);
	int max_x = ln.line_right();

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
			case display_list_item:
				{
					if(!ln.empty())
					{
						max_x = add_line(ln, max_width);
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
						add_line(ln, max_width);
					}
					init_line(ln, ln.get_top(), max_width, el->m_clear);
					int rw = el->render(hdc, 0, ln.get_top(), max_width);
					ln += el;
					add_line(ln, max_width);

					max_x = ln.line_right();
					ret_width = max(ret_width, rw);
				}
				break;
			case display_inline_block:
				{
					el->render(hdc, ln.get_left(), ln.get_top(), max_x);
					max_x = place_inline(el, ln, max_width);
					ret_width = max(ret_width, el->right());
				}
				break;
			case display_inline:
				{
					litehtml::size sz;
					el->get_content_size(hdc, sz, max_x);
					el->m_pos = sz;
					max_x = place_inline(el, ln, max_width);
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

	// calculate outlines

	calc_outlines(parent_width);

	//

	if(!m_lines.empty())
	{
		m_pos.height = m_lines.back().get_top() + m_lines.back().get_height();

		if(!m_padding.top && !m_borders.top && m_lines.front().collapse_top_margin() || is_body())
		{
			int add = m_lines.front().get_margin_top();
			m_margins.top = max(m_margins.top,	m_lines.front().get_margin_top());
			for(line::vector::iterator ln = m_lines.begin(); ln != m_lines.end(); ln++)
			{
				ln->add_top(-add);
			}

			m_pos.height -= add;
		}

		if(!m_padding.bottom && !m_borders.bottom && m_lines.back().collapse_bottom_margin())
		{
			m_margins.bottom	= max(m_margins.bottom,	m_lines.back().get_margin_bottom());
			m_pos.height		-= m_lines.back().get_margin_bottom();
		}
	} else
	{
		m_pos.height = m_css_height.calc_percent(0);
	}

	if(is_floats_holder())
	{
		int floats_height = get_floats_height();
		if(floats_height > m_pos.height)
		{
			m_pos.height = floats_height;
		}
	}


	m_pos.move_to(x, y);
	m_pos.x	+= content_margins_left();
	m_pos.y += content_margins_top();
	if(m_el_position == element_position_relative)
	{
		m_pos.x += m_css_left.calc_percent(parent_width);
	}

	// TODO: Percents are incorrect
	int block_height = m_css_height.calc_percent(m_pos.height);

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
	const wchar_t* str = get_style_property(L"font-size", false, 0);

	int parent_sz = 0;
	int doc_font_size = m_doc->container()->get_default_font_size();
	if(m_parent)
	{
		parent_sz = m_parent->get_font_size();
	} else
	{
		parent_sz = doc_font_size;
	}


	if(!str)
	{
		return parent_sz;
	}

	int ret = 0;

	css_length sz;
	sz.fromString(str, font_size_strings);
	if(sz.is_predefined())
	{
		switch(sz.predef())
		{
		case fontSize_xx_small:
			ret = doc_font_size * 3 / 5;
			break;
		case fontSize_x_small:
			ret = doc_font_size * 3 / 4;
			break;
		case fontSize_small:
			ret = doc_font_size * 8 / 9;
			break;
		case fontSize_large:
			ret = doc_font_size * 6 / 5;
			break;
		case fontSize_x_large:
			ret = doc_font_size * 3 / 2;
			break;
		case fontSize_xx_large:
			ret = doc_font_size * 2;
			break;
		default:
			ret = doc_font_size;
			break;
		}
	} else
	{
		if(sz.units() == css_units_percentage)
		{
			ret = sz.calc_percent(parent_sz);
		} else
		{
			ret = m_doc->cvt_units(sz, parent_sz);
		}
	}

	return ret;
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

	if(ln.finish(m_text_align))
	{
		m_lines.push_back(ln);
		ln_top = ln.get_top() + ln.get_height()/* + 1*/;
	} else
	{
		ln_top = ln.get_top();
	}


	ln.clear();
	init_line(ln, ln_top, max_width);
	
	return ln.line_right();
}

int litehtml::element::get_base_line()
{
	uint_ptr font = get_font();
	return m_doc->container()->get_text_base_line(0, font);
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
	elements_vector els;
	ln.get_elements(els);

	line ln_new;
	init_line(ln_new, ln.get_top(), max_width);

	for(elements_vector::iterator i = els.begin(); i != els.end(); i++)
	{
		object_ptr<element> el = (*i);
		place_inline(el, ln_new, max_width);
	}

	ln = ln_new;
}

void litehtml::element::init_line(line& ln, int top, int def_right, element_clear el_clear)
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

	switch(el_clear)
	{
	case clear_left:
		{
			int fh = get_left_floats_height();
			if(fh && fh > ln.get_top())
			{
				top = fh;
			}
		}
		break;
	case clear_right:
		{
			int fh = get_right_floats_height();
			if(fh && fh > ln.get_top())
			{
				top = fh;
			}
		}
		break;
	case clear_both:
		{
			int fh = get_floats_height();
			if(fh && fh > ln.get_top())
			{
				top = fh;
			}
		}
		break;
	}

	ln.init(left, get_line_right(top, def_right), top, m_line_height);
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

int litehtml::element::place_inline( element* el, line& ln, int max_width )
{
	if(!ln.have_room_for(el))
	{
		if(!ln.empty())
		{
			add_line(ln, max_width);
			return place_inline(el, ln, max_width);
		} else
		{
			int new_top = find_next_line_top(ln.get_top(), el->width());
			if(new_top != ln.get_top())
			{
				init_line(ln, new_top, max_width);
			}
		}
	}
	el->m_pos.y = ln.get_top() + el->content_margins_top();
	el->m_pos.x = ln.get_left() + el->content_margins_left();
	ln += el;

	return ln.line_right();
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
			if(res.size() == 1)
			{
				if( value_in_list(res[0].c_str(), L"left;right;center") )
				{
					m_bg.m_position.x.fromString(res[0], L"left;right;center");
					m_bg.m_position.y.set_value(50, css_units_percentage);
				} else if( value_in_list(res[0].c_str(), L"top;bottom;center") )
				{
					m_bg.m_position.y.fromString(res[0], L"top;bottom;center");
					m_bg.m_position.x.set_value(50, css_units_percentage);
				} else
				{
					m_bg.m_position.x.fromString(res[0], L"left;right;center");
					m_bg.m_position.y.set_value(50, css_units_percentage);
				}
			} else
			{
				m_bg.m_position.x.fromString(res[0], L"left;right;center");
				m_bg.m_position.y.fromString(res[1], L"top;bottom;center");
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

void litehtml::element::calc_outlines( int parent_width )
{
	m_padding.left	= m_css_padding.left.calc_percent(parent_width);
	m_padding.right	= m_css_padding.right.calc_percent(parent_width);

	m_borders.left	= m_css_borders.left.width.calc_percent(parent_width);
	m_borders.right	= m_css_borders.right.width.calc_percent(parent_width);

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
}

void litehtml::element::finish()
{
	for(elements_vector::iterator i = m_children.begin(); i != m_children.end(); i++)
	{
		(*i)->finish();
	}
}

void litehtml::element::get_text( std::wstring& text )
{
	for(elements_vector::iterator i = m_children.begin(); i != m_children.end(); i++)
	{
		(*i)->get_text(text);
	}
}

bool litehtml::element::is_body()
{
	return false;
}

void litehtml::element::set_data( const wchar_t* data )
{

}