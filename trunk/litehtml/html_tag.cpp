#include "html.h"
#include "html_tag.h"
#include "tokenizer.h"
#include "document.h"
#include "iterators.h"
#include "stylesheet.h"
#include "table.h"
#include <algorithm>
#include <locale>
#include "el_before_after.h"

litehtml::html_tag::html_tag(litehtml::document* doc) : litehtml::element(doc)
{
	m_z_index				= 0;
	m_overflow				= overflow_visible;
	m_box					= 0;
	m_second_pass			= false;
	m_text_align			= text_align_left;
	m_el_position			= element_position_static;
	m_display				= display_inline;
	m_vertical_align		= va_baseline;
	m_list_style_type		= list_style_type_none;
	m_list_style_position	= list_style_position_outside;
	m_float					= float_none;
	m_clear					= clear_none;
	m_font					= 0;
	m_font_size				= 0;
	m_white_space			= white_space_normal;
	m_lh_predefined			= false;
	m_line_height			= 0;
	m_visibility			= visibility_visible;
}

litehtml::html_tag::~html_tag()
{

}

bool litehtml::html_tag::appendChild( litehtml::element* el )
{
	if(el)
	{
		el->parent(this);
		m_children.push_back(el);
		return true;
	}
	return false;
}

const litehtml::tchar_t* litehtml::html_tag::get_tagName() const
{
	return m_tag.c_str();
}

void litehtml::html_tag::set_attr( const tchar_t* name, const tchar_t* val )
{
	if(name && val)
	{
		tstring s_val = name;
		std::locale lc = std::locale::global(std::locale::classic());
		for(size_t i = 0; i < s_val.length(); i++)
		{
			s_val[i] = std::tolower(s_val[i], lc);
		}
		m_attrs[s_val] = val;
	}
}

const litehtml::tchar_t* litehtml::html_tag::get_attr( const tchar_t* name, const tchar_t* def )
{
	string_map::const_iterator attr = m_attrs.find(name);
	if(attr != m_attrs.end())
	{
		return attr->second.c_str();
	}
	return def;
}

litehtml::element::ptr litehtml::html_tag::select_one( const tstring& selector )
{
	css_selector sel;
	sel.parse(selector);

	return select_one(sel);
}

litehtml::element::ptr litehtml::html_tag::select_one( const css_selector& selector )
{
	if(select(selector))
	{
		return this;
	}

	for(elements_vector::iterator el = m_children.begin(); el != m_children.end(); el++)
	{
		element::ptr res = (*el)->select_one(selector);
		if(res)
		{
			return res;
		}
	}
	return 0;
}

void litehtml::html_tag::apply_stylesheet( const litehtml::css& stylesheet )
{
	remove_before_after();

	for(litehtml::css_selector::vector::const_iterator sel = stylesheet.selectors().begin(); sel != stylesheet.selectors().end(); sel++)
	{
		int apply = select(*(*sel), false);

		if(apply != select_no_match)
		{
			used_selector::ptr us = new used_selector((*sel), false);
			if(apply & select_match_pseudo_class)
			{
				if(select(*(*sel), true))
				{
					m_style.combine(*((*sel)->m_style));
					us->m_used = true;
				}
				m_used_styles.push_back(us);
			} else if(apply & select_match_with_after)
			{
				element* el = get_element_after();
				if(el)
				{
					el->add_style((*sel)->m_style);
				}
			} else if(apply & select_match_with_before)
			{
				element* el = get_element_before();
				if(el)
				{
					el->add_style((*sel)->m_style);
				}
			} else
			{
				m_style.combine(*((*sel)->m_style));
				us->m_used = true;
				m_used_styles.push_back(us);
			}
		}
	}

	for(elements_vector::iterator i = m_children.begin(); i != m_children.end(); i++)
	{
		(*i)->apply_stylesheet(stylesheet);
	}
}

void litehtml::html_tag::get_content_size( size& sz, int max_width )
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

void litehtml::html_tag::draw( uint_ptr hdc, int x, int y, const position* clip )
{
	position pos = m_pos;
	pos.x	+= x;
	pos.y	+= y;

	draw_background(hdc, x, y, clip);

	if(m_display == display_list_item && m_list_style_type != list_style_type_none)
	{
		if(m_overflow > overflow_visible)
		{
			m_doc->container()->set_clip(pos, true, true);
		}

		draw_list_marker(hdc, pos);

		if(m_overflow > overflow_visible)
		{
			m_doc->container()->del_clip();
		}
	}
}

litehtml::uint_ptr litehtml::html_tag::get_font(font_metrics* fm)
{
	if(fm)
	{
		*fm = m_font_metrics;
	}
	return m_font;
}

const litehtml::tchar_t* litehtml::html_tag::get_style_property( const tchar_t* name, bool inherited, const tchar_t* def /*= 0*/ )
{
	const tchar_t* ret = m_style.get_property(name);
	bool pass_parent = false;
	if(m_parent)
	{
		if(ret && !t_strcasecmp(ret, _t("inherit")))
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

void litehtml::html_tag::parse_styles(bool is_reparse)
{
	m_id	= get_attr(_t("id"), _t(""));
	m_class	= get_attr(_t("class"), _t(""));

	const tchar_t* style = get_attr(_t("style"));

	if(style)
	{
		m_style.add(style, NULL);
	}

	init_font();

	m_el_position	= (element_position)	value_index(get_style_property(_t("position"),		false,	_t("static")),		element_position_strings,	element_position_fixed);
	m_text_align	= (text_align)			value_index(get_style_property(_t("text-align"),	true,	_t("left")),		text_align_strings,			text_align_left);
	m_overflow		= (overflow)			value_index(get_style_property(_t("overflow"),		false,	_t("visible")),		overflow_strings,			overflow_visible);
	m_white_space	= (white_space)			value_index(get_style_property(_t("white-space"),	true,	_t("normal")),		white_space_strings,		white_space_normal);
	m_display		= (style_display)		value_index(get_style_property(_t("display"),		false,	_t("inline")),		style_display_strings,		display_inline);
	m_visibility	= (visibility)			value_index(get_style_property(_t("visibility"),	true,	_t("visible")),		visibility_strings,			visibility_visible);

	if(m_el_position != element_position_static)
	{
		const tchar_t* val = get_style_property(_t("z-index"), false, 0);
		if(val)
		{
			m_z_index = t_atoi(val);
		}
	}

	// support for fixed element will be added later
	if(m_el_position == element_position_fixed)
	{
		m_el_position = element_position_absolute;
	}

	const tchar_t* va	= get_style_property(_t("vertical-align"), true,	_t("baseline"));
	m_vertical_align = (vertical_align) value_index(va, vertical_align_strings, va_baseline);

	const tchar_t* fl	= get_style_property(_t("float"), false,	_t("none"));
	m_float = (element_float) value_index(fl, element_float_strings, float_none);

	m_clear = (element_clear) value_index(get_style_property(_t("clear"), false, _t("none")), element_clear_strings, clear_none);

	if((m_el_position == element_position_absolute || m_float != float_none) && m_display != display_none)
	{
		m_display = display_block;
	}

	m_css_width.fromString(		get_style_property(_t("width"),			false,	_t("auto")), _t("auto"));
	m_css_height.fromString(	get_style_property(_t("height"),			false,	_t("auto")), _t("auto"));

	m_doc->cvt_units(m_css_width,	m_font_size);
	m_doc->cvt_units(m_css_height,	m_font_size);

	m_css_min_width.fromString(		get_style_property(_t("min-width"),		false,	_t("0")));
	m_css_min_height.fromString(	get_style_property(_t("min-height"),	false,	_t("0")));

	m_css_max_width.fromString(		get_style_property(_t("max-width"),		false,	_t("none")),	_t("none"));
	m_css_max_height.fromString(	get_style_property(_t("max-height"),	false,	_t("none")),	_t("none"));
	
	m_doc->cvt_units(m_css_min_width,	m_font_size);
	m_doc->cvt_units(m_css_min_height,	m_font_size);

	m_css_left.fromString(		get_style_property(_t("left"),				false,	_t("auto")), _t("auto"));
	m_css_right.fromString(		get_style_property(_t("right"),				false,	_t("auto")), _t("auto"));
	m_css_top.fromString(		get_style_property(_t("top"),				false,	_t("auto")), _t("auto"));
	m_css_bottom.fromString(	get_style_property(_t("bottom"),			false,	_t("auto")), _t("auto"));

	m_doc->cvt_units(m_css_left,	m_font_size);
	m_doc->cvt_units(m_css_right,	m_font_size);
	m_doc->cvt_units(m_css_top,		m_font_size);
	m_doc->cvt_units(m_css_bottom,	m_font_size);

	m_css_margins.left.fromString(		get_style_property(_t("margin-left"),			false,	_t("0")), _t("auto"));
	m_css_margins.right.fromString(		get_style_property(_t("margin-right"),			false,	_t("0")), _t("auto"));
	m_css_margins.top.fromString(		get_style_property(_t("margin-top"),			false,	_t("0")), _t("auto"));
	m_css_margins.bottom.fromString(	get_style_property(_t("margin-bottom"),		false,	_t("0")), _t("auto"));

	m_css_padding.left.fromString(		get_style_property(_t("padding-left"),			false,	_t("0")), _t(""));
	m_css_padding.right.fromString(		get_style_property(_t("padding-right"),		false,	_t("0")), _t(""));
	m_css_padding.top.fromString(		get_style_property(_t("padding-top"),			false,	_t("0")), _t(""));
	m_css_padding.bottom.fromString(	get_style_property(_t("padding-bottom"),		false,	_t("0")), _t(""));

	m_css_borders.left.width.fromString(	get_style_property(_t("border-left-width"),	false,	_t("medium")), border_width_strings);
	m_css_borders.right.width.fromString(	get_style_property(_t("border-right-width"),	false,	_t("medium")), border_width_strings);
	m_css_borders.top.width.fromString(		get_style_property(_t("border-top-width"),		false,	_t("medium")), border_width_strings);
	m_css_borders.bottom.width.fromString(	get_style_property(_t("border-bottom-width"),	false,	_t("medium")), border_width_strings);

	m_css_borders.left.color = web_color::from_string(get_style_property(_t("border-left-color"),	false,	_t("")));
	m_css_borders.left.style = (border_style) value_index(get_style_property(_t("border-left-style"), false, _t("none")), border_style_strings, border_style_none);

	m_css_borders.right.color = web_color::from_string(get_style_property(_t("border-right-color"),	false,	_t("")));
	m_css_borders.right.style = (border_style) value_index(get_style_property(_t("border-right-style"), false, _t("none")), border_style_strings, border_style_none);

	m_css_borders.top.color = web_color::from_string(get_style_property(_t("border-top-color"),	false,	_t("")));
	m_css_borders.top.style = (border_style) value_index(get_style_property(_t("border-top-style"), false, _t("none")), border_style_strings, border_style_none);

	m_css_borders.bottom.color = web_color::from_string(get_style_property(_t("border-bottom-color"),	false,	_t("")));
	m_css_borders.bottom.style = (border_style) value_index(get_style_property(_t("border-bottom-style"), false, _t("none")), border_style_strings, border_style_none);

	m_css_borders.radius.top_left_x.fromString(get_style_property(_t("border-top-left-radius-x"), false, _t("0")));
	m_css_borders.radius.top_left_y.fromString(get_style_property(_t("border-top-left-radius-y"), false, _t("0")));

	m_css_borders.radius.top_right_x.fromString(get_style_property(_t("border-top-right-radius-x"), false, _t("0")));
	m_css_borders.radius.top_right_y.fromString(get_style_property(_t("border-top-right-radius-y"), false, _t("0")));

	m_css_borders.radius.bottom_right_x.fromString(get_style_property(_t("border-bottom-right-radius-x"), false, _t("0")));
	m_css_borders.radius.bottom_right_y.fromString(get_style_property(_t("border-bottom-right-radius-y"), false, _t("0")));

	m_css_borders.radius.bottom_left_x.fromString(get_style_property(_t("border-bottom-left-radius-x"), false, _t("0")));
	m_css_borders.radius.bottom_left_y.fromString(get_style_property(_t("border-bottom-left-radius-y"), false, _t("0")));

	m_doc->cvt_units(m_css_borders.radius.bottom_left_x,			m_font_size);
	m_doc->cvt_units(m_css_borders.radius.bottom_left_y,			m_font_size);
	m_doc->cvt_units(m_css_borders.radius.bottom_right_x,			m_font_size);
	m_doc->cvt_units(m_css_borders.radius.bottom_right_y,			m_font_size);
	m_doc->cvt_units(m_css_borders.radius.top_left_x,				m_font_size);
	m_doc->cvt_units(m_css_borders.radius.top_left_y,				m_font_size);
	m_doc->cvt_units(m_css_borders.radius.top_right_x,				m_font_size);
	m_doc->cvt_units(m_css_borders.radius.top_right_y,				m_font_size);

	m_margins.left		= m_doc->cvt_units(m_css_margins.left,		m_font_size);
	m_margins.right		= m_doc->cvt_units(m_css_margins.right,		m_font_size);
	m_margins.top		= m_doc->cvt_units(m_css_margins.top,		m_font_size);
	m_margins.bottom	= m_doc->cvt_units(m_css_margins.bottom,	m_font_size);

	m_padding.left		= m_doc->cvt_units(m_css_padding.left,		m_font_size);
	m_padding.right		= m_doc->cvt_units(m_css_padding.right,		m_font_size);
	m_padding.top		= m_doc->cvt_units(m_css_padding.top,		m_font_size);
	m_padding.bottom	= m_doc->cvt_units(m_css_padding.bottom,	m_font_size);

	m_borders.left		= m_doc->cvt_units(m_css_borders.left.width,	m_font_size);
	m_borders.right		= m_doc->cvt_units(m_css_borders.right.width,	m_font_size);
	m_borders.top		= m_doc->cvt_units(m_css_borders.top.width,		m_font_size);
	m_borders.bottom	= m_doc->cvt_units(m_css_borders.bottom.width,	m_font_size);

	css_length line_height;
	line_height.fromString(get_style_property(_t("line-height"),	true,	_t("normal")), _t("normal"));
	if(line_height.is_predefined())
	{
		m_line_height = m_font_metrics.height;
		m_lh_predefined = true;
	} else if(line_height.units() == css_units_none)
	{
		m_line_height = (int) (line_height.val() * m_font_size);
		m_lh_predefined = false;
	} else
	{
		m_line_height =  m_doc->cvt_units(line_height,	m_font_size, m_font_size);
		m_lh_predefined = false;
	}


	if(m_display == display_list_item)
	{
		const tchar_t* list_type = get_style_property(_t("list-style-type"), true, _t("disc"));
		m_list_style_type = (list_style_type) value_index(list_type, list_style_type_strings, list_style_type_disc);

		const tchar_t* list_pos = get_style_property(_t("list-style-position"), true, _t("outside"));
		m_list_style_position = (list_style_position) value_index(list_pos, list_style_position_strings, list_style_position_outside);

		const tchar_t* list_image = get_style_property(_t("list-style-image"), true, 0);
		if(list_image)
		{
			tstring url;
			css::parse_css_url(list_image, url);

			const tchar_t* list_image_baseurl = get_style_property(_t("list-style-image-baseurl"), true, 0);
			m_doc->container()->load_image(url.c_str(), list_image_baseurl, true);
		}

	}

	parse_background();

	if(!is_reparse)
	{
		for(elements_vector::iterator i = m_children.begin(); i != m_children.end(); i++)
		{
			(*i)->parse_styles();
		}

		init();
	}
}

int litehtml::html_tag::render( int x, int y, int max_width )
{
	int parent_width = max_width;
/*
	if(m_parent)
	{
		parent_width = m_parent->get_predefined_width(max_width);
	}
*/

	// restore margins after collapse
	m_margins.top		= m_doc->cvt_units(m_css_margins.top,		m_font_size, max_width);
	m_margins.bottom	= m_doc->cvt_units(m_css_margins.bottom,	m_font_size, max_width);

	// reset auto margins
	if(m_css_margins.left.is_predefined())
	{
		m_margins.left = 0;
	}
	if(m_css_margins.right.is_predefined())
	{
		m_margins.right = 0;
	}

	m_pos.clear();
	m_pos.move_to(x, y);

	m_pos.x	+= content_margins_left();
	m_pos.y += content_margins_top();

	if(m_el_position == element_position_relative)
	{
		m_pos.x += m_css_left.calc_percent(parent_width);
	}

	int ret_width = 0;

	int block_width = 0;

	if(m_display != display_table_cell && !m_css_width.is_predefined())
	{
		block_width = calc_width(parent_width);
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

	// check for max-width
	if(!m_css_max_width.is_predefined())
	{
		int mw = m_doc->cvt_units(m_css_max_width, m_font_size, parent_width);
		if(max_width > mw)
		{
			max_width = mw;
		}
	}

	m_floats_left.clear();
	m_floats_right.clear();
	m_boxes.clear();
	m_cahe_line_left.invalidate();
	m_cahe_line_right.invalidate();

	calc_outlines(parent_width);

	for(elements_vector::iterator i = m_children.begin(); i != m_children.end(); i++)
	{
		element* el = (*i);
		int rw = place_element(el, max_width);
		if(rw > ret_width)
		{
			ret_width = rw;
		}
	}

	m_pos.height = 0;

	finish_last_box(true);

	if(is_inline_box() && !block_width)
	{
		m_pos.width		= ret_width;
	} else
	{
		m_pos.width		= max_width;
	}
	calc_outlines(parent_width);

	if(!m_boxes.empty())
	{
		if(!is_body())
		{
			if(collapse_top_margin())
			{
				int old_top = m_margins.top;
				m_margins.top = std::max(m_boxes.front()->top_margin(), m_margins.top);
				if(m_margins.top != old_top)
				{
					update_floats(m_margins.top - old_top, this);
				}
			}
			if(collapse_bottom_margin())
			{
				m_margins.bottom = std::max(m_boxes.back()->bottom_margin(), m_margins.bottom);
			}
		}
		m_pos.height = m_boxes.back()->bottom() - m_boxes.back()->bottom_margin();
	}

	// add the floats height to the block height
	if(is_floats_holder())
	{
		int floats_height = get_floats_height();
		if(floats_height > m_pos.height)
		{
			m_pos.height = floats_height;
		}
	}

	// calculate the final position

	m_pos.move_to(x, y);
	m_pos.x	+= content_margins_left();
	m_pos.y += content_margins_top();
	if(m_el_position == element_position_relative)
	{
		m_pos.x += m_doc->cvt_units(m_css_left, m_font_size, parent_width);
	}

	int block_height = 0;
	if(get_predefined_height(block_height))
	{
		m_pos.height = block_height;
	}

	int min_height = 0;
	if(!m_css_min_height.is_predefined() && m_css_min_height.units() == css_units_percentage)
	{
		if(m_parent)
		{
			if(m_parent->get_predefined_height(block_height))
			{
				min_height = m_css_min_height.calc_percent(block_height);
			}
		}
	} else
	{
		min_height = (int) m_css_min_height.val();
	}

	if(m_display == display_list_item)
	{
		const tchar_t* list_image = get_style_property(_t("list-style-image"), true, 0);
		if(list_image)
		{
			tstring url;
			css::parse_css_url(list_image, url);

			size sz;
			const tchar_t* list_image_baseurl = get_style_property(_t("list-style-image-baseurl"), true, 0);
			m_doc->container()->get_image_size(url.c_str(), list_image_baseurl, sz);
			if(min_height < sz.height)
			{
				min_height = sz.height;
			}
		}

	}

	if(min_height > m_pos.height)
	{
		m_pos.height = min_height;
	}

	int min_width = m_css_min_width.calc_percent(parent_width);
	if(min_width != 0)
	{
		if(min_width > m_pos.width)
		{
			m_pos.width = min_width;
		}
		if(min_width >ret_width)
		{
			ret_width	= min_width;
		}
	}

	ret_width += content_margins_left() + content_margins_right();

	// re-render the inline-block with new width
	if((	m_display == display_inline_block						|| 
			(m_float != float_none && m_css_width.is_predefined())	|| 
			m_el_position == element_position_absolute ) 

			&& ret_width < max_width && !m_second_pass && m_parent)
	{
		m_second_pass = true;
		render(x, y, ret_width);
		m_second_pass = false;
		m_pos.width = ret_width - (content_margins_left() + content_margins_right());
	}

	return ret_width;
}

bool litehtml::html_tag::is_white_space()
{
	return false;
}

int litehtml::html_tag::get_font_size() const
{
	return m_font_size;
}

int litehtml::html_tag::get_base_line()
{
	if(is_replaced())
	{
		return 0;
	}
	int bl = 0;
	if(!m_boxes.empty())
	{
		bl = m_boxes.back()->baseline() + content_margins_bottom();
	}
	return bl;
}

void litehtml::html_tag::init()
{
	//remove white white spaces
	elements_vector::iterator i = m_children.begin();
	while(i != m_children.end())
	{
		if((*i)->is_white_space())
		{
			i++;
			while(i != m_children.end() && (*i)->is_white_space())
			{
				i = m_children.erase(i);
			}
		} else
		{
			i++;
		}
	}
}

int litehtml::html_tag::select(const css_selector& selector, bool apply_pseudo)
{
	int right_res = select(selector.m_right, apply_pseudo);
	if(right_res == select_no_match)
	{
		return select_no_match;
	}
	if(selector.m_left)
	{
		if(!m_parent)
		{
			return select_no_match;
		}
		switch(selector.m_combinator)
		{
		case combinator_descendant:
			{
				bool is_pseudo = false;
				element* res = find_ancestor(*selector.m_left, apply_pseudo, &is_pseudo);
				if(!res)
				{
					return select_no_match;
				} else
				{
					if(is_pseudo)
					{
						right_res |= select_match_pseudo_class;
					}
				}
			}
			break;
		case combinator_child:
			{
				int res = m_parent->select(*selector.m_left, apply_pseudo);
				if(res == select_no_match)
				{
					return select_no_match;
				} else
				{
					if(right_res != select_match_pseudo_class)
					{
						right_res |= res;
					}
				}
			}
			break;
		case combinator_adjacent_sibling:
			{
				bool is_pseudo = false;
				element* res =  m_parent->find_adjacent_sibling(this, *selector.m_left, apply_pseudo, &is_pseudo);
				if(!res)
				{
					return select_no_match;
				} else
				{
					if(is_pseudo)
					{
						right_res |= select_match_pseudo_class;
					}
				}
			}
			break;
		case combinator_general_sibling:
			{
				bool is_pseudo = false;
				element* res =  m_parent->find_sibling(this, *selector.m_left, apply_pseudo, &is_pseudo);
				if(!res)
				{
					return select_no_match;
				} else
				{
					if(is_pseudo)
					{
						right_res |= select_match_pseudo_class;
					}
				}
			}
			break;
		default:
			right_res = select_no_match;
		}
	}
	return right_res;
}

int litehtml::html_tag::select(const css_element_selector& selector, bool apply_pseudo)
{
	if(!selector.m_tag.empty() && selector.m_tag != _t("*"))
	{
		if(selector.m_tag != m_tag)
		{
			return select_no_match;
		}
	}

	int res = select_match;


	for(css_attribute_selector::map::const_iterator i = selector.m_attrs.begin(); i != selector.m_attrs.end(); i++)
	{
		const tchar_t* attr_value = get_attr(i->first.c_str());
		switch(i->second.condition)
		{
		case select_exists:
			if(!attr_value)
			{
				return select_no_match;
			}
			break;
		case select_equal:
			if(!attr_value)
			{
				return select_no_match;
			} else 
			{
				if(i->first == _t("class"))
				{
					string_vector tokens1;
					tokenize(attr_value, tokens1, _t(" "));
					string_vector tokens2;
					tokenize(i->second.val, tokens2, _t(" "));
					bool found = true;
					for(string_vector::iterator str1 = tokens2.begin(); str1 != tokens2.end() && found; str1++)
					{
						bool f = false;
						for(string_vector::iterator str2 = tokens1.begin(); str2 != tokens1.end() && !f; str2++)
						{
							if( !t_strcasecmp(str1->c_str(), str2->c_str()) )
							{
								f = true;
							}
						}
						if(!f)
						{
							found = false;
						}
					}
					if(!found)
					{
						return select_no_match;
					}
				} else
				{
					if( t_strcasecmp(i->second.val.c_str(), attr_value) )
					{
						return select_no_match;
					}
				}
			}
			break;
		case select_contain_str:
			if(!attr_value)
			{
				return select_no_match;
			} else if(!t_strstr(attr_value, i->second.val.c_str()))
			{
				return select_no_match;
			}
			break;
		case select_start_str:
			if(!attr_value)
			{
				return select_no_match;
			} else if(t_strncmp(attr_value, i->second.val.c_str(), i->second.val.length()))
			{
				return select_no_match;
			}
			break;
		case select_end_str:
			if(!attr_value)
			{
				return select_no_match;
			} else if(t_strncmp(attr_value, i->second.val.c_str(), i->second.val.length()))
			{
				const tchar_t* s = attr_value + t_strlen(attr_value) - i->second.val.length() - 1;
				if(s < attr_value)
				{
					return select_no_match;
				}
				if(i->second.val != s)
				{
					return select_no_match;
				}
			}
			break;
		case select_pseudo_element:
			if(i->second.val == _t("after"))
			{
				res |= select_match_with_after;
			} else if(i->second.val == _t("before"))
			{
				res |= select_match_with_before;
			} else
			{
				return select_no_match;
			}
			break;
		case select_pseudo_class:
			if(apply_pseudo)
			{
				if(!m_parent) return select_no_match;

				tstring selector_param;
				tstring	selector_name;

				tstring::size_type begin	= i->second.val.find_first_of(_t('('));
				tstring::size_type end		= i->second.val.find_last_of(_t(')'));
				if(begin != tstring::npos && end != tstring::npos)
				{
					selector_param = i->second.val.substr(begin + 1, end - begin - 1);
				}
				if(begin != tstring::npos)
				{
					selector_name = i->second.val.substr(0, begin);
					litehtml::trim(selector_name);
				} else
				{
					selector_name = i->second.val;
				}

				int selector = value_index(selector_name.c_str(), pseudo_class_strings);
				
				switch(selector)
				{
				case pseudo_class_only_child:
					if( !m_parent->is_only_child(this, false) )
					{
						return select_no_match;
					}
					break;
				case pseudo_class_only_of_type:
					if( !m_parent->is_only_child(this, true) )
					{
						return select_no_match;
					}
					break;
				case pseudo_class_first_child:
					if( !m_parent->is_nth_child(this, 0, 1, false) )
					{
						return select_no_match;
					}
					break;
				case pseudo_class_first_of_type:
					if( !m_parent->is_nth_child(this, 0, 1, true) )
					{
						return select_no_match;
					}
					break;
				case pseudo_class_last_child:
					if( !m_parent->is_nth_last_child(this, 0, 1, false) )
					{
						return select_no_match;
					}
					break;
				case pseudo_class_last_of_type:
					if( !m_parent->is_nth_last_child(this, 0, 1, true) )
					{
						return select_no_match;
					}
					break;
				case pseudo_class_nth_child:
				case pseudo_class_nth_of_type:
				case pseudo_class_nth_last_child:
				case pseudo_class_nth_last_of_type:
					{
						if(selector_param.empty()) return select_no_match;

						int num = 0;
						int off = 0;

						parse_nth_child_params(selector_param, num, off);
						if(!num && !off) return select_no_match;
						switch(selector)
						{
						case pseudo_class_nth_child:
							if( !m_parent->is_nth_child(this, num, off, false) )
							{
								return select_no_match;
							}
							break;
						case pseudo_class_nth_of_type:
							if( !m_parent->is_nth_child(this, num, off, true) )
							{
								return select_no_match;
							}
							break;
						case pseudo_class_nth_last_child:
							if( !m_parent->is_nth_last_child(this, num, off, false) )
							{
								return select_no_match;
							}
							break;
						case pseudo_class_nth_last_of_type:
							if( !m_parent->is_nth_last_child(this, num, off, true) )
							{
								return select_no_match;
							}
							break;
						}

					}
					break;
				case pseudo_class_not:
					{
						css_element_selector sel;
						sel.parse(selector_param);
						if(select(sel, apply_pseudo))
						{
							return select_no_match;
						}
					}
					break;
				default:
					if(std::find(m_pseudo_classes.begin(), m_pseudo_classes.end(), i->second.val) == m_pseudo_classes.end())
					{
						return select_no_match;
					}
					break;
				}
			} else
			{
				res |= select_match_pseudo_class;
			}
			break;
		}
	}
	return res;
}

litehtml::element* litehtml::html_tag::find_ancestor( const css_selector& selector, bool apply_pseudo, bool* is_pseudo )
{
	if(!m_parent)
	{
		return 0;
	}
	int res = m_parent->select(selector, apply_pseudo);
	if(res != select_no_match)
	{
		if(is_pseudo)
		{
			if(res & select_match_pseudo_class)
			{
				*is_pseudo = true;
			} else
			{
				*is_pseudo = false;
			}
		}
		return m_parent;
	}
	return m_parent->find_ancestor(selector, apply_pseudo, is_pseudo);
}

int litehtml::html_tag::get_floats_height(element_float el_float) const
{
	if(is_floats_holder())
	{
		int h = 0;

		bool process = false;

		for(floated_box::vector::const_iterator i = m_floats_left.begin(); i != m_floats_left.end(); i++)
		{
			process = false;
			switch(el_float)
			{
			case float_none:
				process = true;
				break;
			case float_left:
				if(i->clear_floats == clear_left || i->clear_floats == clear_both)
				{
					process = true;
				}
				break;
			case float_right:
				if(i->clear_floats == clear_right || i->clear_floats == clear_both)
				{
					process = true;
				}
				break;
			}
			if(process)
			{
				if(el_float == float_none)
				{
					h = std::max(h, i->pos.bottom());
				} else
				{
					h = std::max(h, i->pos.top());
				}
			}
		}


		for(floated_box::vector::const_iterator i = m_floats_right.begin(); i != m_floats_right.end(); i++)
		{
			process = false;
			switch(el_float)
			{
			case float_none:
				process = true;
				break;
			case float_left:
				if(i->clear_floats == clear_left || i->clear_floats == clear_both)
				{
					process = true;
				}
				break;
			case float_right:
				if(i->clear_floats == clear_right || i->clear_floats == clear_both)
				{
					process = true;
				}
				break;
			}
			if(process)
			{
				if(el_float == float_none)
				{
					h = std::max(h, i->pos.bottom());
				} else
				{
					h = std::max(h, i->pos.top());
				}
			}
		}

		return h;
	}
	int h = m_parent->get_floats_height(el_float);
	return h - m_pos.y;
}

int litehtml::html_tag::get_left_floats_height() const
{
	if(is_floats_holder())
	{
		int h = 0;
		if(!m_floats_left.empty())
		{
			for(floated_box::vector::const_iterator el = m_floats_left.begin(); el != m_floats_left.end(); el++)
			{
				h = std::max(h, el->pos.bottom());
			}
		}
		return h;
	}
	int h = m_parent->get_left_floats_height();
	return h - m_pos.y;
}

int litehtml::html_tag::get_right_floats_height() const
{
	if(is_floats_holder())
	{
		int h = 0;
		if(!m_floats_right.empty())
		{
			for(floated_box::vector::const_iterator el = m_floats_right.begin(); el != m_floats_right.end(); el++)
			{
				h = std::max(h, el->pos.bottom());
			}
		}
		return h;
	}
	int h = m_parent->get_right_floats_height();
	return h - m_pos.y;
}

int litehtml::html_tag::get_line_left( int y )
{
	if(is_floats_holder())
	{
		if(m_cahe_line_left.is_valid && m_cahe_line_left.hash == y)
		{
			return m_cahe_line_left.val;
		}

		int w = 0;
		for(floated_box::vector::const_iterator el = m_floats_left.begin(); el != m_floats_left.end(); el++)
		{
			if(y >= el->pos.top() && y < el->pos.bottom())
			{
				w = std::max(w, el->pos.right());
				if(w < el->pos.right())
				{
					break;
				}
			}
		}
		m_cahe_line_left.set_value(y, w);
		return w;
	}
	int w = m_parent->get_line_left(y + m_pos.y);
	if(w < 0)
	{
		w = 0;
	}
	return w - (w ? m_pos.x : 0);
}

int litehtml::html_tag::get_line_right( int y, int def_right )
{
	if(is_floats_holder())
	{
		if(m_cahe_line_right.is_valid && m_cahe_line_right.hash == y)
		{
			if(m_cahe_line_right.is_default)
			{
				return def_right;
			} else
			{
				return std::min(m_cahe_line_right.val, def_right);
			}
		}

		int w = def_right;
		m_cahe_line_right.is_default = true;
		for(floated_box::vector::const_iterator el = m_floats_right.begin(); el != m_floats_right.end(); el++)
		{
			if(y >= el->pos.top() && y < el->pos.bottom())
			{
				w = std::min(w, el->pos.left());
				m_cahe_line_right.is_default = false;
				if(w > el->pos.left())
				{
					break;
				}
			}
		}
		m_cahe_line_right.set_value(y, w);
		return w;
	}
	int w = m_parent->get_line_right(y + m_pos.y, def_right + m_pos.x);
	return w - m_pos.x;
}


void litehtml::html_tag::get_line_left_right( int y, int def_right, int& ln_left, int& ln_right )
{
	if(is_floats_holder())
	{
		ln_left		= get_line_left(y);
		ln_right	= get_line_right(y, def_right);
	} else
	{
		m_parent->get_line_left_right(y + m_pos.y, def_right + m_pos.x, ln_left, ln_right);
		ln_right -= m_pos.x;

		if(ln_left < 0)
		{
			ln_left = 0;
		} else if(ln_left)
		{
			ln_left -= m_pos.x;
		}
	}
}

void litehtml::html_tag::fix_line_width( int max_width, element_float flt )
{
	if(!m_boxes.empty())
	{
		elements_vector els;
		m_boxes.back()->get_elements(els);
		bool was_cleared = false;
		if(!els.empty() && els.front()->get_clear() != clear_none)
		{
			if(els.front()->get_clear() == clear_both)
			{
				was_cleared = true;
			} else
			{
				if(	(flt == float_left	&& els.front()->get_clear() == clear_left) ||
					(flt == float_right	&& els.front()->get_clear() == clear_right) )
				{
					was_cleared = true;
				}
			}
		}

		if(!was_cleared)
		{
			m_boxes.pop_back();

			for(elements_vector::iterator i = els.begin(); i != els.end(); i++)
			{
				place_element((*i), max_width);
			}
		} else
		{
			int line_top = 0;
			if(m_boxes.back()->get_type() == box_line)
			{
				line_top = m_boxes.back()->top();
			} else
			{
				line_top = m_boxes.back()->bottom();
			}

			int line_left	= 0;
			int line_right	= max_width;
			get_line_left_right(line_top, max_width, line_left, line_right);

			elements_vector els;
			m_boxes.back()->new_width(line_left, line_right, els);
			for(elements_vector::iterator i = els.begin(); i != els.end(); i++)
			{
				place_element((*i), max_width);
			}
		}
	}
}

void litehtml::html_tag::add_float( element* el, int x, int y )
{
	if(is_floats_holder())
	{
		floated_box fb;
		fb.pos.x		= el->left() + x;
		fb.pos.y		= el->top()  + y;
		fb.pos.width	= el->width();
		fb.pos.height	= el->height();
		fb.float_side	= el->get_float();
		fb.clear_floats	= el->get_clear();
		fb.el			= el;

		if(fb.float_side == float_left)
		{
			if(m_floats_left.empty())
			{
				m_floats_left.push_back(fb);
			} else
			{
				bool inserted = false;
				for(floated_box::vector::iterator i = m_floats_left.begin(); i != m_floats_left.end(); i++)
				{
					if(fb.pos.right() > i->pos.right())
					{
						m_floats_left.insert(i, fb);
						inserted = true;
						break;
					}
				}
				if(!inserted)
				{
					m_floats_left.push_back(fb);
				}
			}
			m_cahe_line_left.invalidate();
		} else if(fb.float_side == float_right)
		{
			if(m_floats_right.empty())
			{
				m_floats_right.push_back(fb);
			} else
			{
				bool inserted = false;
				for(floated_box::vector::iterator i = m_floats_right.begin(); i != m_floats_right.end(); i++)
				{
					if(fb.pos.left() < i->pos.left())
					{
						m_floats_right.insert(i, fb);
						inserted = true;
						break;
					}
				}
				if(!inserted)
				{
					m_floats_right.push_back(fb);
				}
			}
			m_cahe_line_right.invalidate();
		}
	} else
	{
		m_parent->add_float(el, x + m_pos.x, y + m_pos.y);
	}
}

int litehtml::html_tag::find_next_line_top( int top, int width, int def_right )
{
	if(is_floats_holder())
	{
		int new_top = top;
		int_vector points;

		for(floated_box::vector::const_iterator el = m_floats_left.begin(); el != m_floats_left.end(); el++)
		{
			if(el->pos.top() >= top)
			{
				if(find(points.begin(), points.end(), el->pos.top()) == points.end())
				{
					points.push_back(el->pos.top());
				}
			}
			if(el->pos.bottom() >= top)
			{
				if(find(points.begin(), points.end(), el->pos.bottom()) == points.end())
				{
					points.push_back(el->pos.bottom());
				}
			}
		}

		for(floated_box::vector::const_iterator el = m_floats_right.begin(); el != m_floats_right.end(); el++)
		{
			if(el->pos.top() >= top)
			{
				if(find(points.begin(), points.end(), el->pos.top()) == points.end())
				{
					points.push_back(el->pos.top());
				}
			}
			if(el->pos.bottom() >= top)
			{
				if(find(points.begin(), points.end(), el->pos.bottom()) == points.end())
				{
					points.push_back(el->pos.bottom());
				}
			}
		}

		if(!points.empty())
		{
			sort(points.begin(), points.end(), std::less<int>( ));
			new_top = points.back();

			for(int_vector::iterator i = points.begin(); i != points.end(); i++)
			{
				int pos_left	= 0;
				int pos_right	= def_right;
				get_line_left_right((*i), def_right, pos_left, pos_right);

				if(pos_right - pos_left >= width)
				{
					new_top = (*i);
					break;
				}
			}
		}
		return new_top;
	}
	int new_top = m_parent->find_next_line_top(top + m_pos.y, width, def_right + m_pos.x);
	return new_top - m_pos.y;
}

void litehtml::html_tag::parse_background()
{
	// parse background-color
	m_bg.m_color		= get_color(_t("background-color"), false, web_color(0, 0, 0, 0));

	// parse background-position
	const tchar_t* str = get_style_property(_t("background-position"), false, _t("0% 0%"));
	if(str)
	{
		string_vector res;
		tokenize(str, res, _t(" \t"));
		if(res.size() > 0)
		{
			if(res.size() == 1)
			{
				if( value_in_list(res[0].c_str(), _t("left;right;center")) )
				{
					m_bg.m_position.x.fromString(res[0], _t("left;right;center"));
					m_bg.m_position.y.set_value(50, css_units_percentage);
				} else if( value_in_list(res[0].c_str(), _t("top;bottom;center")) )
				{
					m_bg.m_position.y.fromString(res[0], _t("top;bottom;center"));
					m_bg.m_position.x.set_value(50, css_units_percentage);
				} else
				{
					m_bg.m_position.x.fromString(res[0], _t("left;right;center"));
					m_bg.m_position.y.set_value(50, css_units_percentage);
				}
			} else
			{
				if(value_in_list(res[0].c_str(), _t("left;right")))
				{
					m_bg.m_position.x.fromString(res[0], _t("left;right;center"));
					m_bg.m_position.y.fromString(res[1], _t("top;bottom;center"));
				} else if(value_in_list(res[0].c_str(), _t("top;bottom")))
				{
					m_bg.m_position.x.fromString(res[1], _t("left;right;center"));
					m_bg.m_position.y.fromString(res[0], _t("top;bottom;center"));
				} else if(value_in_list(res[1].c_str(), _t("left;right")))
				{
					m_bg.m_position.x.fromString(res[1], _t("left;right;center"));
					m_bg.m_position.y.fromString(res[0], _t("top;bottom;center"));
				}else if(value_in_list(res[1].c_str(), _t("top;bottom")))
				{
					m_bg.m_position.x.fromString(res[0], _t("left;right;center"));
					m_bg.m_position.y.fromString(res[1], _t("top;bottom;center"));
				} else
				{
					m_bg.m_position.x.fromString(res[0], _t("left;right;center"));
					m_bg.m_position.y.fromString(res[1], _t("top;bottom;center"));
				}
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

	str = get_style_property(_t("background-size"), false, _t("auto"));
	if(str)
	{
		string_vector res;
		tokenize(str, res, _t(" \t"));
		if(!res.empty())
		{
			m_bg.m_position.width.fromString(res[0], background_size_strings);
			if(res.size() > 1)
			{
				m_bg.m_position.height.fromString(res[1], background_size_strings);
			} else
			{
				m_bg.m_position.height.predef(background_size_auto);
			}
		} else
		{
			m_bg.m_position.width.predef(background_size_auto);
			m_bg.m_position.height.predef(background_size_auto);
		}
	}

	m_doc->cvt_units(m_bg.m_position.x,			m_font_size);
	m_doc->cvt_units(m_bg.m_position.y,			m_font_size);
	m_doc->cvt_units(m_bg.m_position.width,		m_font_size);
	m_doc->cvt_units(m_bg.m_position.height,	m_font_size);

	// parse background_attachment
	m_bg.m_attachment = (background_attachment) value_index(
		get_style_property(_t("background-attachment"), false, _t("scroll")), 
		background_attachment_strings, 
		background_attachment_scroll);

	// parse background_attachment
	m_bg.m_repeat = (background_repeat) value_index(
		get_style_property(_t("background-repeat"), false, _t("repeat")), 
		background_repeat_strings, 
		background_repeat_repeat);

	// parse background_clip
	m_bg.m_clip = (background_box) value_index(
		get_style_property(_t("background-clip"), false, _t("border-box")), 
		background_box_strings, 
		background_box_border);

	// parse background_origin
	m_bg.m_origin = (background_box) value_index(
		get_style_property(_t("background-origin"), false, _t("padding-box")), 
		background_box_strings, 
		background_box_content);

	// parse background-image
	css::parse_css_url(get_style_property(_t("background-image"), false, _t("")), m_bg.m_image);
	m_bg.m_baseurl = get_style_property(_t("background-image-baseurl"), false, _t(""));

	if(!m_bg.m_image.empty())
	{
		m_doc->container()->load_image(m_bg.m_image.c_str(), m_bg.m_baseurl.empty() ? 0 : m_bg.m_baseurl.c_str(), true);
	}
}

void litehtml::html_tag::add_absolute( element* el )
{
	if( (m_display != display_inline && m_el_position != element_position_static) || (!m_parent) )
	{
		m_positioned.push_back(el);
	} else
	{
		m_parent->add_absolute(el);
	}
}

void litehtml::html_tag::calc_outlines( int parent_width )
{
	m_padding.left	= m_css_padding.left.calc_percent(parent_width);
	m_padding.right	= m_css_padding.right.calc_percent(parent_width);

	m_borders.left	= m_css_borders.left.width.calc_percent(parent_width);
	m_borders.right	= m_css_borders.right.width.calc_percent(parent_width);

	m_margins.left	= m_css_margins.left.calc_percent(parent_width);
	m_margins.right	= m_css_margins.right.calc_percent(parent_width);

	m_margins.top		= m_css_margins.top.calc_percent(parent_width);
	m_margins.bottom	= m_css_margins.bottom.calc_percent(parent_width);

	m_padding.top		= m_css_padding.top.calc_percent(parent_width);
	m_padding.bottom	= m_css_padding.bottom.calc_percent(parent_width);

	if(m_display == display_block || m_display == display_table)
	{
		if(m_css_margins.left.is_predefined() && m_css_margins.right.is_predefined())
		{
			int el_width = m_pos.width + m_borders.left + m_borders.right + m_padding.left + m_padding.right;
			if(el_width <= parent_width)
			{
				m_margins.left	= (parent_width - el_width) / 2;
				m_margins.right	= (parent_width - el_width) - m_margins.left;
			} else
			{
				m_margins.left	= 0;
				m_margins.right	= 0;
			}
		} else if(m_css_margins.left.is_predefined() && !m_css_margins.right.is_predefined())
		{
			int el_width = m_pos.width + m_borders.left + m_borders.right + m_padding.left + m_padding.right + m_margins.right;
			m_margins.left	= parent_width - el_width;
			if(m_margins.left < 0) m_margins.left = 0;
		} else if(!m_css_margins.left.is_predefined() && m_css_margins.right.is_predefined())
		{
			int el_width = m_pos.width + m_borders.left + m_borders.right + m_padding.left + m_padding.right + m_margins.left;
			m_margins.right	= parent_width - el_width;
			if(m_margins.right < 0) m_margins.right = 0;
		}
	}
}

void litehtml::html_tag::parse_attributes()
{
	for(elements_vector::iterator i = m_children.begin(); i != m_children.end(); i++)
	{
		(*i)->parse_attributes();
	}
}

void litehtml::html_tag::get_text( tstring& text )
{
	for(elements_vector::iterator i = m_children.begin(); i != m_children.end(); i++)
	{
		(*i)->get_text(text);
	}
}

bool litehtml::html_tag::is_body()  const
{
	return false;
}

void litehtml::html_tag::set_data( const tchar_t* data )
{

}

void litehtml::html_tag::get_inline_boxes( position::vector& boxes )
{
	litehtml::box* old_box = 0;
	position pos;
	for(elements_vector::iterator iter = m_children.begin(); iter != m_children.end(); iter++)
	{
		element* el = (*iter);
		if(!el->skip())
		{
			if(el->m_box)
			{
				if(el->m_box != old_box)
				{
					if(old_box)
					{
						boxes.push_back(pos);
					}
					old_box		= el->m_box;
					pos.x		= el->left() + el->margin_left();
					pos.y		= el->top() - m_padding.top - m_borders.top;
					pos.width	= 0;
					pos.height	= 0;
				}
				pos.width	= el->right() - pos.x - el->margin_right() - el->margin_left();
				pos.height	= std::max(pos.height, el->height() + m_padding.top + m_padding.bottom + m_borders.top + m_borders.bottom);
			} else if(el->get_display() == display_inline)
			{
				el->get_inline_boxes(boxes);
			}
		}
	}
	if(pos.width || pos.height)
	{
		boxes.push_back(pos);
	}
}

bool litehtml::html_tag::on_mouse_over( int x, int y )
{
	if(m_display == display_inline_text)
	{
		return false;
	}

	bool ret = set_pseudo_class(_t("hover"), is_point_inside(x, y));

	for(elements_vector::iterator iter = m_children.begin(); iter != m_children.end(); iter++)
	{
		if(!(*iter)->skip())
		{
			if((*iter)->on_mouse_over(x - m_pos.x, y - m_pos.y))
			{
				ret = true;
			}
		}
	}

	return ret;
}

bool litehtml::html_tag::find_styles_changes( position::vector& redraw_boxes, int x, int y )
{
	if(m_display == display_inline_text)
	{
		return false;
	}

	bool ret = false;
	bool apply = false;
	for (used_selector::vector::iterator iter = m_used_styles.begin(); iter != m_used_styles.end() && !apply; iter++)
	{
		int res = select(*((*iter)->m_selector), true);
		if( (!(res && (*iter)->m_used) || (!res && !(*iter)->m_used)) )
		{
			apply = true;
		}
	}

	if(apply)
	{
		if(m_display == display_inline ||  m_display == display_table_row)
		{
			position::vector boxes;
			get_inline_boxes(boxes);
			for(position::vector::iterator pos = boxes.begin(); pos != boxes.end(); pos++)
			{
				pos->x	+= x;
				pos->y	+= y;
				redraw_boxes.push_back(*pos);
			}
		} else
		{
			position pos = m_pos;
			pos.x += x;
			pos.y += y;
			pos += m_padding;
			pos += m_borders;
			redraw_boxes.push_back(pos);
		}

		ret = true;
		m_style.clear();
		for (used_selector::vector::iterator iter = m_used_styles.begin(); iter != m_used_styles.end(); iter++)
		{
			if(select(*(*iter)->m_selector, true))
			{
				(*iter)->m_used = true;
				m_style.combine(*((*iter)->m_selector->m_style));
			} else
			{
				(*iter)->m_used = false;
			}
		}
		parse_styles(/*true*/);
	}
	for(elements_vector::iterator i = m_children.begin(); i != m_children.end(); i++)
	{
		if(!(*i)->skip())
		{
			if((*i)->find_styles_changes(redraw_boxes, x + m_pos.x, y + m_pos.y))
			{
				ret = true;
			}
		}
	}
	return ret;
}

bool litehtml::html_tag::on_mouse_leave()
{
	bool ret = false;

	if(set_pseudo_class(_t("hover"), false))
	{
		ret = true;
	}

	if(set_pseudo_class(_t("active"), false))
	{
		ret = true;
	}

	for(elements_vector::iterator iter = m_children.begin(); iter != m_children.end(); iter++)
	{
		if(!(*iter)->skip())
		{
			if((*iter)->on_mouse_leave())
			{
				ret = true;
			}
		}
	}

	return ret;
}

bool litehtml::html_tag::on_lbutton_down( int x, int y )
{
	if(m_display == display_inline_text)
	{
		return false;
	}

	bool ret = set_pseudo_class(_t("active"), is_point_inside(x, y));

	for(elements_vector::iterator iter = m_children.begin(); iter != m_children.end(); iter++)
	{
		if(!(*iter)->skip())
		{
			if((*iter)->on_lbutton_down(x - m_pos.x, y - m_pos.y))
			{
				ret = true;
			}
		}
	}

	return ret;
}

bool litehtml::html_tag::on_lbutton_up( int x, int y )
{
	if(m_display == display_inline_text)
	{
		return false;
	}

	bool ret = false;

	if(set_pseudo_class(_t("active"), false))
	{
		ret = true;
		on_click(x, y);
	}

	for(elements_vector::iterator iter = m_children.begin(); iter != m_children.end(); iter++)
	{
		if(!(*iter)->skip())
		{
			if((*iter)->on_lbutton_up(x, y))
			{
				ret = true;
			}
		}
	}

	return ret;
}

void litehtml::html_tag::on_click( int x, int y )
{

}

const litehtml::tchar_t* litehtml::html_tag::get_cursor()
{
	const tchar_t* ret = 0;

	if(std::find(m_pseudo_classes.begin(), m_pseudo_classes.end(), _t("hover")) != m_pseudo_classes.end())
	{
		ret = get_style_property(_t("cursor"), true, 0);
	}

	for(elements_vector::iterator iter = m_children.begin(); iter != m_children.end(); iter++)
	{
		const tchar_t* cursor = (*iter)->get_cursor();
		if(cursor)
		{
			ret = cursor;
		}
	}

	return ret;
}

static const int font_size_table[8][7] =
{
	{ 9,    9,     9,     9,    11,    14,    18},
	{ 9,    9,     9,    10,    12,    15,    20},
	{ 9,    9,     9,    11,    13,    17,    22},
	{ 9,    9,    10,    12,    14,    18,    24},
	{ 9,    9,    10,    13,    16,    20,    26},
	{ 9,    9,    11,    14,    17,    21,    28},
	{ 9,   10,    12,    15,    17,    23,    30},
	{ 9,   10,    13,    16,    18,    24,    32}
};


void litehtml::html_tag::init_font()
{
	// initialize font size
	const tchar_t* str = get_style_property(_t("font-size"), false, 0);

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
		m_font_size = parent_sz;
	} else
	{
		m_font_size = parent_sz;

		css_length sz;
		sz.fromString(str, font_size_strings);
		if(sz.is_predefined())
		{
			int idx_in_table = doc_font_size - 9;
			if(idx_in_table >= 0 && idx_in_table <= 7)
			{
				if(sz.predef() >= fontSize_xx_small && sz.predef() <= fontSize_xx_large)
				{
					m_font_size = font_size_table[idx_in_table][sz.predef()];
				} else
				{
					m_font_size = doc_font_size;
				}
			} else			
			{
				switch(sz.predef())
				{
				case fontSize_xx_small:
					m_font_size = doc_font_size * 3 / 5;
					break;
				case fontSize_x_small:
					m_font_size = doc_font_size * 3 / 4;
					break;
				case fontSize_small:
					m_font_size = doc_font_size * 8 / 9;
					break;
				case fontSize_large:
					m_font_size = doc_font_size * 6 / 5;
					break;
				case fontSize_x_large:
					m_font_size = doc_font_size * 3 / 2;
					break;
				case fontSize_xx_large:
					m_font_size = doc_font_size * 2;
					break;
				default:
					m_font_size = doc_font_size;
					break;
				}
			}
		} else
		{
			if(sz.units() == css_units_percentage)
			{
				m_font_size = sz.calc_percent(parent_sz);
			} else if(sz.units() == css_units_none)
			{
				m_font_size = parent_sz;
			} else
			{
				m_font_size = m_doc->cvt_units(sz, parent_sz);
			}
		}
	}

	// initialize font
	const tchar_t* name			= get_style_property(_t("font-family"),		true,	_t("inherit"));
	const tchar_t* weight		= get_style_property(_t("font-weight"),		true,	_t("normal"));
	const tchar_t* style		= get_style_property(_t("font-style"),		true,	_t("normal"));
	const tchar_t* decoration	= get_style_property(_t("text-decoration"),	true,	_t("none"));

	m_font = m_doc->get_font(name, m_font_size, weight, style, decoration, &m_font_metrics);
}

bool litehtml::html_tag::is_break() const
{
	return false;
}

void litehtml::html_tag::set_tagName( const tchar_t* tag )
{
	tstring s_val = tag;
	std::locale lc = std::locale::global(std::locale::classic());
	for(size_t i = 0; i < s_val.length(); i++)
	{
		s_val[i] = std::tolower(s_val[i], lc);
	}
	m_tag = s_val;
}

void litehtml::html_tag::draw_background( uint_ptr hdc, int x, int y, const position* clip )
{
	position pos = m_pos;
	pos.x	+= x;
	pos.y	+= y;

	position el_pos = pos;
	el_pos += m_padding;
	el_pos += m_borders;

	if(m_display != display_inline && m_display != display_table_row)
	{
		if(el_pos.does_intersect(clip))
		{
			background_paint bg_paint;
			init_background_paint(pos, bg_paint);

			m_doc->container()->draw_background(hdc, bg_paint);
			m_doc->container()->draw_borders(hdc, m_css_borders, bg_paint.border_box);
		}
	} else
	{
		position::vector boxes;
		get_inline_boxes(boxes);

		for(position::vector::iterator box = boxes.begin(); box != boxes.end(); box++)
		{
			box->x	+= x;
			box->y	+= y;

			if(box->does_intersect(clip))
			{
				background_paint bg_paint;
				position content_box = *box;
				content_box -= m_borders;
				content_box -= m_padding;

				init_background_paint(content_box, bg_paint);

				bg_paint.border_radius	= css_border_radius();

				if(box == boxes.begin())
				{
					bg_paint.border_radius.bottom_left_x	= m_css_borders.radius.bottom_left_x;
					bg_paint.border_radius.bottom_left_y	= m_css_borders.radius.bottom_left_y;
					bg_paint.border_radius.top_left_x		= m_css_borders.radius.top_left_x;
					bg_paint.border_radius.top_left_y		= m_css_borders.radius.top_left_y;
				}
				if(box == boxes.end() - 1)
				{
					bg_paint.border_radius.bottom_right_x	= m_css_borders.radius.bottom_right_x;
					bg_paint.border_radius.bottom_right_y	= m_css_borders.radius.bottom_right_y;
					bg_paint.border_radius.top_right_x		= m_css_borders.radius.top_right_x;
					bg_paint.border_radius.top_right_y		= m_css_borders.radius.top_right_y;
				}

				m_doc->container()->draw_background(hdc, bg_paint);

				css_borders bdr;
				bdr.top		= m_css_borders.top;
				bdr.bottom	= m_css_borders.bottom;
				if(box == boxes.begin())
				{
					bdr.left	= m_css_borders.left;
				}
				if(box == boxes.end() - 1)
				{
					bdr.right	= m_css_borders.right;
				}
				bdr.radius = bg_paint.border_radius;
				m_doc->container()->draw_borders(hdc, bdr, *box);
			}
		}
	}
}

int litehtml::html_tag::render_inline( element* container, int max_width )
{
	int ret_width = 0;
	int rw = 0;
	for(elements_vector::iterator i = m_children.begin(); i != m_children.end(); i++)
	{
		rw = container->place_element( (*i), max_width );
		if(rw > ret_width)
		{
			ret_width = rw;
		}
	}
	return ret_width;
}

int litehtml::html_tag::place_element( element* el, int max_width )
{
	if(el->get_display() == display_none) return 0;

	if(el->get_display() == display_inline)
	{
		return el->render_inline(this, max_width);
	}

	if(el->get_element_position() == element_position_absolute)
	{
		int line_top = 0;
		if(!m_boxes.empty())
		{
			if(m_boxes.back()->get_type() == box_line)
			{
				line_top = m_boxes.back()->top();
				if(!m_boxes.back()->is_empty())
				{
					line_top += line_height();
				}
			} else
			{
				line_top = m_boxes.back()->bottom();
			}
		}

		el->render(0, line_top, max_width);
		el->m_pos.x	+= el->content_margins_left();
		el->m_pos.y	+= el->content_margins_top();
		return 0;
	}

	int ret_width = 0;

	switch(el->get_float())
	{
	case float_left:
		{
			int line_top = 0;
			if(!m_boxes.empty())
			{
				if(m_boxes.back()->get_type() == box_line)
				{
					line_top = m_boxes.back()->top();
				} else
				{
					line_top = m_boxes.back()->bottom();
				}
			}
			line_top		= get_cleared_top(el, line_top);
			int line_left	= 0;
			int line_right	= max_width;
			get_line_left_right(line_top, max_width, line_left, line_right);

			el->render(line_left, line_top, line_right);
			if(el->right() > line_right)
			{
				int new_top = find_next_line_top(el->top(), el->width(), max_width);
				el->m_pos.x = get_line_left(new_top) + el->content_margins_left();
				el->m_pos.y = new_top + el->content_margins_top();
			}
			add_float(el, 0, 0);
			fix_line_width(max_width, float_left);
			ret_width = el->right();
		}
		break;
	case float_right:
		{
			int line_top = 0;
			if(!m_boxes.empty())
			{
				if(m_boxes.back()->get_type() == box_line)
				{
					line_top = m_boxes.back()->top();
				} else
				{
					line_top = m_boxes.back()->bottom();
				}
			}
			line_top		= get_cleared_top(el, line_top);
			int line_left	= 0;
			int line_right	= max_width;
			get_line_left_right(line_top, max_width, line_left, line_right);

			el->render(0, line_top, line_right);

			if(line_left + el->width() > line_right)
			{
				int new_top = find_next_line_top(el->top(), el->width(), max_width);
				el->m_pos.x = get_line_right(new_top, max_width) - el->width() + el->content_margins_left();
				el->m_pos.y = new_top + el->content_margins_top();
			} else
			{
				el->m_pos.x = line_right - el->width() + el->content_margins_left();
			}
			add_float(el, 0, 0);
			fix_line_width(max_width, float_right);

			line_left	= 0;
			line_right	= max_width;
			get_line_left_right(line_top, max_width, line_left, line_right);

			ret_width = line_left + (max_width - line_right);
		}
		break;
	default:
		{
			int line_top = 0;
			if(!m_boxes.empty())
			{
				line_top = m_boxes.back()->top();
			}
			int line_left	= 0;
			int line_right	= max_width;
			get_line_left_right(line_top, max_width, line_left, line_right);

			switch(el->get_display())
			{
			case display_inline_block:
				ret_width = el->render(line_left, line_top, line_right);
				break;
			case display_block:		
				if(el->is_replaced() || el->is_floats_holder())
				{
					el->m_pos.width		= el->get_css_width().calc_percent(line_right - line_left);
					el->m_pos.height	= el->get_css_height().calc_percent(0);
					if(el->m_pos.width || el->m_pos.height)
					{
						el->calc_outlines(line_right - line_left);
					}
				}
				break;
			case display_inline_text:
				{
					litehtml::size sz;
					el->get_content_size(sz, line_right);
					el->m_pos = sz;
				}
				break;
			default:
				ret_width = 0;
				break;
			}

			bool add_box = true;
			if(!m_boxes.empty())
			{
				if(m_boxes.back()->can_hold(el, m_white_space /*el->parent()->get_white_space()*/))
				{
					add_box = false;
				}
			}
			if(add_box)
			{
				line_top = new_box(el, max_width);
			} else if(!m_boxes.empty())
			{
				line_top = m_boxes.back()->top();
			}

			line_left	= 0;
			line_right	= max_width;
			get_line_left_right(line_top, max_width, line_left, line_right);

			if(!el->is_inline_box())
			{
				if(m_boxes.size() == 1)
				{
					if(collapse_top_margin())
					{
						int shift = el->margin_top();
						if(shift >= 0)
						{
							line_top -=shift;
							m_boxes.back()->y_shift(-shift);
						}
					}
				} else
				{
					int shift = 0;
					int prev_margin = m_boxes[m_boxes.size() - 2]->bottom_margin();

					if(prev_margin > el->margin_top())
					{
						shift = el->margin_top();
					} else
					{
						shift = prev_margin;
					}
					if(shift >= 0)
					{
						line_top -= shift;
						m_boxes.back()->y_shift(-shift);
					}
				}
			}

			switch(el->get_display())
			{
			case display_table:
			case display_list_item:
				ret_width = el->render(line_left, line_top, line_right - line_left);
				break;
			case display_block:
			case display_table_cell:
			case display_table_caption:
			case display_table_row:
				if(el->is_replaced() || el->is_floats_holder())
				{
					ret_width = el->render(line_left, line_top, line_right - line_left) + line_left + (max_width - line_right);
				} else
				{
					ret_width = el->render(0, line_top, max_width);
				}
				break;
			default:
				ret_width = 0;
				break;
			}

			m_boxes.back()->add_element(el);

			if(el->is_inline_box() && !el->skip())
			{
				ret_width = el->right() + (max_width - line_right);
			}
		}
		break;
	}

	return ret_width;
}

bool litehtml::html_tag::set_pseudo_class( const tchar_t* pclass, bool add )
{
	if(add)
	{
		if(std::find(m_pseudo_classes.begin(), m_pseudo_classes.end(), pclass) == m_pseudo_classes.end())
		{
			m_pseudo_classes.push_back(pclass);
			return true;
		}
	} else
	{
		string_vector::iterator pi = std::find(m_pseudo_classes.begin(), m_pseudo_classes.end(), pclass);
		if(pi != m_pseudo_classes.end())
		{
			m_pseudo_classes.erase(pi);
			return true;
		}
	}
	return false;
}

int litehtml::html_tag::line_height() const
{
	return m_line_height;
}

bool litehtml::html_tag::is_replaced() const
{
	return false;
}

int litehtml::html_tag::finish_last_box(bool end_of_render)
{
	int line_top = 0;

	if(!m_boxes.empty())
	{
		m_boxes.back()->finish(end_of_render);

		if(m_boxes.back()->is_empty())
		{
			line_top = m_boxes.back()->top();
			m_boxes.pop_back();
		}

		if(!m_boxes.empty())
		{
			line_top = m_boxes.back()->bottom();
		}
	}
	return line_top;
}

int litehtml::html_tag::new_box( element* el, int max_width )
{
	int line_top	= get_cleared_top(el, finish_last_box());

	int line_left	= 0;
	int line_right	= max_width;
	get_line_left_right(line_top, max_width, line_left, line_right);

	if(el->is_inline_box() || el->is_floats_holder())
	{
		if(el->width() > line_right - line_left)
		{
			line_top	= find_next_line_top(line_top, el->width(), max_width);
			line_left	= 0;
			line_right	= max_width;
			get_line_left_right(line_top, max_width, line_left, line_right);
		}
	}

	int first_line_margin = 0;
	if(m_boxes.empty() && m_list_style_type != list_style_type_none && m_list_style_position == list_style_position_inside)
	{
		int sz_font = get_font_size();
		first_line_margin = sz_font;
	}

	if(el->is_inline_box())
	{
		font_metrics fm;
		get_font(&fm);
		line_box* lb = new line_box(line_top, line_left + first_line_margin, line_right, line_height(), fm, m_text_align);
		m_boxes.push_back(lb);
	} else
	{
		block_box* bb = new block_box(line_top, line_left, line_right);
		m_boxes.push_back(bb);
	}

	return line_top;
}

int litehtml::html_tag::get_cleared_top( element* el, int line_top )
{
	switch(el->get_clear())
	{
	case clear_left:
		{
			int fh = get_left_floats_height();
			if(fh && fh > line_top)
			{
				line_top = fh;
			}
		}
		break;
	case clear_right:
		{
			int fh = get_right_floats_height();
			if(fh && fh > line_top)
			{
				line_top = fh;
			}
		}
		break;
	case clear_both:
		{
			int fh = get_floats_height();
			if(fh && fh > line_top)
			{
				line_top = fh;
			}
		}
		break;
	default:
		if(el->get_float() != float_none)
		{
			int fh = get_floats_height(el->get_float());
			if(fh && fh > line_top)
			{
				line_top = fh;
			}
		}
		break;
	}
	return line_top;
}

litehtml::style_display litehtml::html_tag::get_display() const
{
	return m_display;
}

litehtml::element_float litehtml::html_tag::get_float() const
{
	return m_float;
}

bool litehtml::html_tag::is_floats_holder() const
{
	if(	m_display == display_inline_block || 
		m_display == display_table_cell || 
		!m_parent || 
		is_body() || 
		m_float != float_none ||
		m_el_position == element_position_absolute ||
		m_overflow > overflow_visible)
	{
		return true;
	}
	return false;
}

bool litehtml::html_tag::is_first_child( const element* el )
{
	if(!m_children.empty())
	{
		if(el == m_children.front())
		{
			return true;
		}
	}
	return false;
}

bool litehtml::html_tag::is_last_child( const element* el )
{
	if(!m_children.empty())
	{
		if(el == m_children.back())
		{
			return true;
		}
	}
	return false;
}

litehtml::white_space litehtml::html_tag::get_white_space() const
{
	return m_white_space;
}

litehtml::vertical_align litehtml::html_tag::get_vertical_align() const
{
	return m_vertical_align;
}

litehtml::css_length litehtml::html_tag::get_css_left() const
{
	return m_css_left;
}

litehtml::css_length litehtml::html_tag::get_css_right() const
{
	return m_css_right;
}

litehtml::css_length litehtml::html_tag::get_css_top() const
{
	return m_css_top;
}

litehtml::css_length litehtml::html_tag::get_css_bottom() const
{
	return m_css_bottom;
}

litehtml::element_clear litehtml::html_tag::get_clear() const
{
	return m_clear;
}

litehtml::css_length litehtml::html_tag::get_css_width() const
{
	return m_css_width;
}

litehtml::css_length litehtml::html_tag::get_css_height() const
{
	return m_css_height;
}

size_t litehtml::html_tag::get_children_count() const
{
	return m_children.size();
}

litehtml::element::ptr litehtml::html_tag::get_child( int idx ) const
{
	return m_children[idx];
}

void litehtml::html_tag::set_css_width( css_length& w )
{
	m_css_width = w;
}

void litehtml::html_tag::apply_vertical_align()
{
	if(!m_boxes.empty())
	{
		int add = 0;
		int content_height	= m_boxes.back()->bottom();

		if(m_pos.height > content_height)
		{
			switch(m_vertical_align)
			{
			case va_middle:
				add = (m_pos.height - content_height) / 2;
				break;
			case va_bottom:
				add = m_pos.height - content_height;
				break;
			default:
				add = 0;
				break;
			}
		}

		if(add)
		{
			for(size_t i = 0; i < m_boxes.size(); i++)
			{
				m_boxes[i]->y_shift(add);
			}
		}
	}
}

litehtml::element_position litehtml::html_tag::get_element_position() const
{
	return m_el_position;
}

void litehtml::html_tag::init_background_paint( position pos, background_paint &bg_paint )
{
	bg_paint = m_bg;
	position content_box	= pos;
	position padding_box	= pos;
	padding_box += m_padding;
	position border_box		= padding_box;
	border_box += m_borders;

	switch(m_bg.m_clip)
	{
	case litehtml::background_box_padding:
		bg_paint.clip_box = padding_box;
		break;
	case litehtml::background_box_content:
		bg_paint.clip_box = content_box;
		break;
	default:
		bg_paint.clip_box = border_box;
		break;
	}

	switch(m_bg.m_origin)
	{
	case litehtml::background_box_border:
		bg_paint.origin_box = border_box;
		break;
	case litehtml::background_box_content:
		bg_paint.origin_box = content_box;
		break;
	default:
		bg_paint.origin_box = padding_box;
		break;
	}

	if(!bg_paint.image.empty())
	{
		m_doc->container()->get_image_size(bg_paint.image.c_str(), bg_paint.baseurl.c_str(), bg_paint.image_size);
		if(bg_paint.image_size.width && bg_paint.image_size.height)
		{
			litehtml::size img_new_sz = bg_paint.image_size;
			double img_ar_width		= (double) bg_paint.image_size.width / (double) bg_paint.image_size.height;
			double img_ar_height	= (double) bg_paint.image_size.height / (double) bg_paint.image_size.width;


			if(m_bg.m_position.width.is_predefined())
			{
				switch(m_bg.m_position.width.predef())
				{
				case litehtml::background_size_contain:
					if( (int) ((double) bg_paint.origin_box.width * img_ar_height) <= bg_paint.origin_box.height )
					{
						img_new_sz.width = bg_paint.origin_box.width;
						img_new_sz.height	= (int) ((double) bg_paint.origin_box.width * img_ar_height);
					} else
					{
						img_new_sz.height = bg_paint.origin_box.height;
						img_new_sz.width	= (int) ((double) bg_paint.origin_box.height * img_ar_width);
					}
					break;
				case litehtml::background_size_cover:
					if( (int) ((double) bg_paint.origin_box.width * img_ar_height) >= bg_paint.origin_box.height )
					{
						img_new_sz.width = bg_paint.origin_box.width;
						img_new_sz.height	= (int) ((double) bg_paint.origin_box.width * img_ar_height);
					} else
					{
						img_new_sz.height = bg_paint.origin_box.height;
						img_new_sz.width	= (int) ((double) bg_paint.origin_box.height * img_ar_width);
					}
					break;
					break;
				case litehtml::background_size_auto:
					if(!m_bg.m_position.height.is_predefined())
					{
						img_new_sz.height	= m_bg.m_position.height.calc_percent(bg_paint.origin_box.height);
						img_new_sz.width	= (int) ((double) img_new_sz.height * img_ar_width);
					}
					break;
				}
			} else
			{
				img_new_sz.width = m_bg.m_position.width.calc_percent(bg_paint.origin_box.width);
				if(m_bg.m_position.height.is_predefined())
				{
					img_new_sz.height = (int) ((double) img_new_sz.width * img_ar_height);
				} else
				{
					img_new_sz.height = m_bg.m_position.height.calc_percent(bg_paint.origin_box.height);
				}
			}

			bg_paint.image_size = img_new_sz;
			bg_paint.position_x = bg_paint.origin_box.x + (int) m_bg.m_position.x.calc_percent(bg_paint.origin_box.width - bg_paint.image_size.width);
			bg_paint.position_y = bg_paint.origin_box.y + (int) m_bg.m_position.y.calc_percent(bg_paint.origin_box.height - bg_paint.image_size.height);
		}

	}
	bg_paint.border_radius	= m_css_borders.radius;
	bg_paint.border_box		= border_box;
}

litehtml::visibility litehtml::html_tag::get_visibility() const
{
	return m_visibility;
}

void litehtml::html_tag::draw_list_marker( uint_ptr hdc, const position &pos )
{
	list_marker lm;

	const tchar_t* list_image = get_style_property(_t("list-style-image"), true, 0);
	size img_size;
	if(list_image)
	{
		css::parse_css_url(list_image, lm.image);
		lm.baseurl = get_style_property(_t("list-style-image-baseurl"), true, 0);
		m_doc->container()->get_image_size(lm.image.c_str(), lm.baseurl, img_size);
	} else
	{
		lm.baseurl = 0;
	}


	int ln_height	= line_height();
	int sz_font		= get_font_size();
	lm.pos.x		= pos.x;
	lm.pos.width	= sz_font	- sz_font * 2 / 3;
	lm.pos.height	= sz_font	- sz_font * 2 / 3;
	lm.pos.y		= pos.y		+ ln_height / 2 - lm.pos.height / 2;

	if(img_size.width && img_size.height)
	{
		if(lm.pos.y + img_size.height > pos.y + pos.height)
		{
			lm.pos.y = pos.y + pos.height - img_size.height;
		}
		if(img_size.width > lm.pos.width)
		{
			lm.pos.x -= img_size.width - lm.pos.width;
		}

		lm.pos.width	= img_size.width;
		lm.pos.height	= img_size.height;
	}
	if(m_list_style_position == list_style_position_outside)
	{
		lm.pos.x -= sz_font;
	}

	lm.color = get_color(_t("color"), true, web_color(0, 0, 0));
	lm.marker_type = m_list_style_type;
	m_doc->container()->draw_list_marker(hdc, lm);
}

void litehtml::html_tag::draw_children( uint_ptr hdc, int x, int y, const position* clip, draw_flag flag, int zindex )
{
	position pos = m_pos;
	pos.x	+= x;
	pos.y	+= y;

	if(m_overflow > overflow_visible)
	{
		m_doc->container()->set_clip(pos, true, true);
	}

	for(elements_vector::iterator i = m_children.begin(); i != m_children.end(); i++)
	{
		element* el = (*i);

		if(el->is_visible())
		{
			switch(flag)
			{
			case draw_positioned:
				if(el->is_positioned() && el->get_zindex() == zindex)
				{
					el->draw(hdc, pos.x, pos.y, clip);
					el->draw_stacking_context(hdc, pos.x, pos.y, clip, true);
					el = 0;
				}
				break;
			case draw_block:
				if(!el->is_inline_box() && el->get_float() == float_none && !el->is_positioned())
				{
					el->draw(hdc, pos.x, pos.y, clip);
				}
				break;
			case draw_floats:
				if(el->get_float() != float_none && !el->is_positioned())
				{
					el->draw(hdc, pos.x, pos.y, clip);
					el->draw_stacking_context(hdc, pos.x, pos.y, clip, false);
					el = 0;
				}
				break;
			case draw_inlines:
				if(el->is_inline_box() && el->get_float() == float_none && !el->is_positioned())
				{
					el->draw(hdc, pos.x, pos.y, clip);
					if(el->get_display() == display_inline_block)
					{
						el->draw_stacking_context(hdc, pos.x, pos.y, clip, false);
						el = 0;
					}
				}
				break;
			default:
				break;
			}

			if(el)
			{
				if(flag == draw_positioned)
				{
					if(!el->is_positioned())
					{
						el->draw_children(hdc, pos.x, pos.y, clip, flag, zindex);
					}
				} else
				{
					if(	el->get_float() == float_none &&
						el->get_display() != display_inline_block && 
						!el->is_positioned())
					{
						el->draw_children(hdc, pos.x, pos.y, clip, flag, zindex);
					}
				}
			}
		}
	}

	if(m_overflow > overflow_visible)
	{
		m_doc->container()->del_clip();
	}
}

class element_zindex_sort
{
public:
	bool operator()(const litehtml::element* _Left, const litehtml::element* _Right) const
	{
		return (_Left->get_zindex() < _Right->get_zindex());
	}
};

bool litehtml::html_tag::fetch_positioned()
{
	bool ret = false;

	m_positioned.clear();

	for(elements_vector::iterator i = m_children.begin(); i != m_children.end(); i++)
	{
		element* el = (*i);
		if(el->get_element_position() != element_position_static)
		{
			add_absolute(el);
		}
		if(!ret && el->get_element_position() == element_position_absolute)
		{
			ret = true;
		}
		if(el->fetch_positioned())
		{
			ret = true;
		}
	}
	return ret;
}

int litehtml::html_tag::get_zindex() const
{
	return m_z_index;
}

void litehtml::html_tag::render_absolutes()
{
	for(elements_vector::iterator abs_el = m_positioned.begin(); abs_el != m_positioned.end(); abs_el++)
	{
		element* el = (*abs_el);

		if(el->get_display() != display_none && el->get_element_position() == element_position_absolute)
		{
			int parent_height	= 0;
			int parent_width	= 0;
			if(el->parent())
			{
				parent_height	= el->parent()->height() - el->parent()->content_margins_bottom() - el->parent()->content_margins_top();
				parent_width	= el->parent()->width() - el->parent()->content_margins_left() - el->parent()->content_margins_right();
			}

			css_length	css_left	= el->get_css_left();
			css_length	css_right	= el->get_css_right();
			css_length	css_top		= el->get_css_top();
			css_length	css_bottom	= el->get_css_bottom();

			bool need_render = false;

			css_length el_w = el->get_css_width();
			css_length el_h = el->get_css_height();
			if(el_w.units() == css_units_percentage && parent_width)
			{
				int w = el_w.calc_percent(parent_width);
				if(el->m_pos.width != w)
				{
					need_render = true;
					el->m_pos.width = w;
				}
			}

			if(el_h.units() == css_units_percentage && parent_height)
			{
				int h = el_h.calc_percent(parent_height);
				if(el->m_pos.height != h)
				{
					need_render = true;
					el->m_pos.height = h;
				}
			}

			bool cvt_x = false;
			bool cvt_y = false;

			if(!css_left.is_predefined() || !css_right.is_predefined())
			{
				if(!css_left.is_predefined() && css_right.is_predefined())
				{
					el->m_pos.x = css_left.calc_percent(parent_width) + el->content_margins_left() - m_padding.left;
				} else if(css_left.is_predefined() && !css_right.is_predefined())
				{
					el->m_pos.x = m_pos.width + m_padding.right - css_right.calc_percent(parent_width) - el->m_pos.width - el->content_margins_right();
				} else
				{
					el->m_pos.x		= css_left.calc_percent(parent_width) + el->content_margins_left() - m_padding.left;
					el->m_pos.width	= m_pos.width + m_padding.left + m_padding.right - css_left.calc_percent(parent_width) - css_right.calc_percent(parent_width) - (el->content_margins_left() + el->content_margins_right());
					need_render = true;
				}
				cvt_x = true;
			}

			if(!css_top.is_predefined() || !css_bottom.is_predefined())
			{
				if(!css_top.is_predefined() && css_bottom.is_predefined())
				{
					el->m_pos.y = css_top.calc_percent(parent_height) + el->content_margins_top() - m_padding.top;
				} else if(css_top.is_predefined() && !css_bottom.is_predefined())
				{
					el->m_pos.y = m_pos.height + m_padding.bottom - css_bottom.calc_percent(parent_height) - el->m_pos.height - el->content_margins_bottom();
				} else
				{
					el->m_pos.y			= css_top.calc_percent(parent_height) + el->content_margins_top() - m_padding.top;
					el->m_pos.height	= m_pos.height + m_padding.top + m_padding.bottom - css_top.calc_percent(parent_height) - css_bottom.calc_percent(parent_height) - (el->content_margins_top() + el->content_margins_bottom());
					need_render = true;
				}
				cvt_y = true;
			}

			if(cvt_x || cvt_y)
			{
				int offset_x = 0;
				int offset_y = 0;
				element* cur_el = el->parent();
				while(cur_el && cur_el != this)
				{
					offset_x += cur_el->m_pos.x;
					offset_y += cur_el->m_pos.y;
					cur_el = cur_el->parent();
				}
				if(cvt_x)	el->m_pos.x -= offset_x;
				if(cvt_y)	el->m_pos.y -= offset_y;
			}

			if(need_render)
			{
				position pos = el->m_pos;
				el->render(el->left(), el->right(), el->width());
				el->m_pos = pos;
			}
		}

		el->render_absolutes();
	}

	if(!m_positioned.empty())
	{
		std::stable_sort(m_positioned.begin(), m_positioned.end(), element_zindex_sort());
	}
}

void litehtml::html_tag::draw_stacking_context( uint_ptr hdc, int x, int y, const position* clip, bool with_positioned )
{
	std::map<int, bool> zindexes;
	if(with_positioned)
	{
		for(elements_vector::iterator i = m_positioned.begin(); i != m_positioned.end(); i++)
		{
			zindexes[(*i)->get_zindex()];
		}

		for(std::map<int, bool>::iterator idx = zindexes.begin(); idx != zindexes.end(); idx++)
		{
			if(idx->first < 0)
			{
				draw_children(hdc, x, y, clip, draw_positioned, idx->first);
			}
		}
	}
	draw_children(hdc, x, y, clip, draw_block, 0);
	draw_children(hdc, x, y, clip, draw_floats, 0);
	draw_children(hdc, x, y, clip, draw_inlines, 0);
	if(with_positioned)
	{
		for(std::map<int, bool>::iterator idx = zindexes.begin(); idx != zindexes.end(); idx++)
		{
			if(idx->first == 0)
			{
				draw_children(hdc, x, y, clip, draw_positioned, idx->first);
			}
		}

		for(std::map<int, bool>::iterator idx = zindexes.begin(); idx != zindexes.end(); idx++)
		{
			if(idx->first > 0)
			{
				draw_children(hdc, x, y, clip, draw_positioned, idx->first);
			}
		}
	}
}

litehtml::overflow litehtml::html_tag::get_overflow() const
{
	return m_overflow;
}

bool litehtml::html_tag::is_nth_child( element* el, int num, int off, bool of_type )
{
	int idx = 1;
	for(elements_vector::iterator child = m_children.begin(); child != m_children.end(); child++)
	{
		if((*child)->get_display() != display_inline_text)
		{
			if( (!of_type) || (of_type && !t_strcmp(el->get_tagName(), (*child)->get_tagName())) )
			{
				if(el == (*child))
				{
					if(num != 0)
					{
						if((idx - off) >= 0 && (idx - off) % num == 0)
						{
							return true;
						}

					} else if(idx == off)
					{
						return true;
					}
					return false;
				}
				idx++;
			}
			if(el == (*child)) break;
		}
	}
	return false;
}

bool litehtml::html_tag::is_nth_last_child( element* el, int num, int off, bool of_type )
{
	int idx = 1;
	for(elements_vector::reverse_iterator child = m_children.rbegin(); child != m_children.rend(); child++)
	{
		if((*child)->get_display() != display_inline_text)
		{
			if( !of_type || (of_type && !t_strcmp(el->get_tagName(), (*child)->get_tagName())) )
			{
				if(el == (*child))
				{
					if(num != 0)
					{
						if((idx - off) >= 0 && (idx - off) % num == 0)
						{
							return true;
						}

					} else if(idx == off)
					{
						return true;
					}
					return false;
				}
				idx++;
			}
			if(el == (*child)) break;
		}
	}
	return false;
}

void litehtml::html_tag::parse_nth_child_params( tstring param, int &num, int &off )
{
	if(param == _t("odd"))
	{
		num = 2;
		off = 1;
	} else if(param == _t("even"))
	{
		num = 2;
		off = 0;
	} else
	{
		string_vector tokens;
		tokenize(param, tokens, _t(" n"), _t("n"));

		tstring s_num;
		tstring s_off;

		tstring s_int;
		for(string_vector::iterator tok = tokens.begin(); tok != tokens.end(); tok++)
		{
			if((*tok) == _t("n"))
			{
				s_num = s_int;
				s_int.clear();
			} else
			{
				s_int += (*tok);
			}
		}
		s_off = s_int;

		num = t_atoi(s_num.c_str());
		off = t_atoi(s_off.c_str());
	}
}

void litehtml::html_tag::calc_document_size( litehtml::size& sz, int x /*= 0*/, int y /*= 0*/ )
{
	if(is_visible())
	{
		element::calc_document_size(sz, x, y);

		if(m_overflow == overflow_visible)
		{
			position pos = m_pos;
			pos.x	+= x;
			pos.y	+= y;

			for(elements_vector::iterator el = m_children.begin(); el != m_children.end(); el++)
			{
				(*el)->calc_document_size(sz, x + m_pos.x, y + m_pos.y);
			}
		}
	}
}

litehtml::element* litehtml::html_tag::find_adjacent_sibling( element* el, const css_selector& selector, bool apply_pseudo /*= true*/, bool* is_pseudo /*= 0*/ )
{
	element* ret = 0;
	for(elements_vector::iterator i = m_children.begin(); i != m_children.end(); i++)
	{
		element* e = (*i);
		if(e->get_display() != display_inline_text)
		{
			if(e == el)
			{
				if(ret)
				{
					int res = ret->select(selector, apply_pseudo);
					if(res != select_no_match)
					{
						if(is_pseudo)
						{
							if(res & select_match_pseudo_class)
							{
								*is_pseudo = true;
							} else
							{
								*is_pseudo = false;
							}
						}
						return ret;
					}
				}
				return 0;
			} else
			{
				ret = e;
			}
		}
	}
	return 0;
}

litehtml::element* litehtml::html_tag::find_sibling( element* el, const css_selector& selector, bool apply_pseudo /*= true*/, bool* is_pseudo /*= 0*/ )
{
	element* ret = 0;
	for(elements_vector::iterator i = m_children.begin(); i != m_children.end(); i++)
	{
		element* e = (*i);
		if(e->get_display() != display_inline_text)
		{
			if(e == el)
			{
				return ret;
			} else if(!ret)
			{
				int res = e->select(selector, apply_pseudo);
				if(res != select_no_match)
				{
					if(is_pseudo)
					{
						if(res & select_match_pseudo_class)
						{
							*is_pseudo = true;
						} else
						{
							*is_pseudo = false;
						}
					}
					ret = e;
				}
			}
		}
	}
	return 0;
}

bool litehtml::html_tag::is_only_child( element* el, bool of_type )
{
	int child_count = 0;
	for(elements_vector::iterator child = m_children.begin(); child != m_children.end(); child++)
	{
		if((*child)->get_display() != display_inline_text)
		{
			if( !of_type || (of_type && !t_strcmp(el->get_tagName(), (*child)->get_tagName())) )
			{
				child_count++;
			}
			if(child_count > 1) break;
		}
	}
	if(child_count > 1)
	{
		return false;
	}
	return true;
}

void litehtml::html_tag::update_floats( int dy, element* parent )
{
	if(is_floats_holder())
	{
		bool reset_cache = false;
		for(floated_box::vector::reverse_iterator fb = m_floats_left.rbegin(); fb != m_floats_left.rend(); fb++)
		{
			if(fb->el->is_ancestor(parent))
			{
				reset_cache	= true;
				fb->pos.y	+= dy;
			}
		}
		if(reset_cache)
		{
			m_cahe_line_left.invalidate();
		}
		reset_cache = false;
		for(floated_box::vector::reverse_iterator fb = m_floats_right.rbegin(); fb != m_floats_right.rend(); fb++)
		{
			if(fb->el->is_ancestor(parent))
			{
				reset_cache	= true;
				fb->pos.y	+= dy;
			}
		}
		if(reset_cache)
		{
			m_cahe_line_right.invalidate();
		}
	} else
	{
		m_parent->update_floats(dy, parent);
	}
}

void litehtml::html_tag::remove_before_after()
{
	if(!m_children.empty())
	{
		if( !t_strcmp(m_children.front()->get_tagName(), _t("::before")) )
		{
			m_children.erase(m_children.begin());
		}
	}
	if(!m_children.empty())
	{
		if( !t_strcmp(m_children.back()->get_tagName(), _t("::after")) )
		{
			m_children.erase(m_children.end() - 1);
		}
	}
}

litehtml::element* litehtml::html_tag::get_element_before()
{
	if(!m_children.empty())
	{
		if( !t_strcmp(m_children.front()->get_tagName(), _t("::before")) )
		{
			return m_children.front();
		}
	}
	element* el = new el_before(m_doc);
	el->parent(this);
	m_children.insert(m_children.begin(), el);
	return el;
}

litehtml::element* litehtml::html_tag::get_element_after()
{
	if(!m_children.empty())
	{
		if( !t_strcmp(m_children.back()->get_tagName(), _t("::after")) )
		{
			return m_children.back();
		}
	}
	element* el = new el_after(m_doc);
	el->parent(this);
	m_children.push_back(el);
	return el;
}

void litehtml::html_tag::add_style( litehtml::style::ptr st )
{
	m_style.combine(*st);
}
