#include "html.h"
#include "document.h"
#include "types.h"
#include "element.h"
#include "css_properties.h"

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


void litehtml::css_properties::parse(const std::shared_ptr<element>& el, const std::shared_ptr<document>& doc)
{
    parse_font(el, doc);

    m_el_position	= (element_position)	value_index(el->get_style_property(_t("position"),		false,	_t("static")),		element_position_strings,	element_position_fixed);
    m_text_align	= (text_align)			value_index(el->get_style_property(_t("text-align"),	true,	_t("left")),		text_align_strings,			text_align_left);
    m_overflow		= (overflow)			value_index(el->get_style_property(_t("overflow"),		false,	_t("visible")),		overflow_strings,			overflow_visible);
    m_white_space	= (white_space)			value_index(el->get_style_property(_t("white-space"),	true,	_t("normal")),		white_space_strings,		white_space_normal);
    m_display		= (style_display)		value_index(el->get_style_property(_t("display"),		false,	_t("inline")),		style_display_strings,		display_inline);
    m_visibility	= (visibility)			value_index(el->get_style_property(_t("visibility"),	true,	_t("visible")),		visibility_strings,			visibility_visible);
    m_box_sizing	= (box_sizing)			value_index(el->get_style_property(_t("box-sizing"),	false,	_t("content-box")),	box_sizing_strings,			box_sizing_content_box);

    if(m_el_position != element_position_static)
    {
        const tchar_t* val = el->get_style_property(_t("z-index"), false, nullptr);
        if(val)
        {
            m_z_index = t_atoi(val);
        }
    }

    const tchar_t* fl	= el->get_style_property(_t("float"), false,	_t("none"));
    m_float = (element_float) value_index(fl, element_float_strings, float_none);


    // https://www.w3.org/TR/CSS22/visuren.html#dis-pos-flo
    if(m_display == display_none)
    {
        // 1. If 'display' has the value 'none', then 'position' and 'float' do not apply. In this case, the element
        //    generates no box.
        m_float = float_none;
    } else
    {
        // 2. Otherwise, if 'position' has the value 'absolute' or 'fixed', the box is absolutely positioned,
        //    the computed value of 'float' is 'none', and display is set according to the table below.
        //    The position of the box will be determined by the 'top', 'right', 'bottom' and 'left' properties
        //    and the box's containing block.
        if (m_el_position == element_position_absolute || m_el_position == element_position_fixed)
        {
            m_float = float_none;

            if (m_display == display_inline_table)
            {
                m_display = display_table;
            } else if (m_display == display_inline ||
                       m_display == display_table_row_group ||
                       m_display == display_table_column ||
                       m_display == display_table_column_group ||
                       m_display == display_table_header_group ||
                       m_display == display_table_footer_group ||
                       m_display == display_table_row ||
                       m_display == display_table_cell ||
                       m_display == display_table_caption ||
                       m_display == display_inline_block)
            {
                m_display = display_block;
            }
        } else if (m_float != float_none)
        {
            // 3. Otherwise, if 'float' has a value other than 'none', the box is floated and 'display' is set
            //    according to the table below.
            if (m_display == display_inline_table)
            {
                m_display = display_table;
            } else if (m_display == display_inline ||
                       m_display == display_table_row_group ||
                       m_display == display_table_column ||
                       m_display == display_table_column_group ||
                       m_display == display_table_header_group ||
                       m_display == display_table_footer_group ||
                       m_display == display_table_row ||
                       m_display == display_table_cell ||
                       m_display == display_table_caption ||
                       m_display == display_inline_block)
            {
                m_display = display_block;
            }
        } else if(!el->have_parent())
        {
            // 4. Otherwise, if the element is the root element, 'display' is set according to the table below,
            //    except that it is undefined in CSS 2.2 whether a specified value of 'list-item' becomes a
            //    computed value of 'block' or 'list-item'.
            if (m_display == display_inline_table)
            {
                m_display = display_table;
            } else if (m_display == display_inline ||
                m_display == display_table_row_group ||
                m_display == display_table_column ||
                m_display == display_table_column_group ||
                m_display == display_table_header_group ||
                m_display == display_table_footer_group ||
                m_display == display_table_row ||
                m_display == display_table_cell ||
                m_display == display_table_caption ||
                m_display == display_inline_block ||
                m_display == display_list_item)
            {
                m_display = display_block;
            }
        }
    }
    // 5. Otherwise, the remaining 'display' property values apply as specified.

    if (m_el_position == element_position_absolute || m_el_position == element_position_fixed || m_el_position == element_position_relative)
    {
        m_css_offsets.left.fromString(el->get_style_property(_t("left"), false, _t("auto")), _t("auto"));
        m_css_offsets.right.fromString(el->get_style_property(_t("right"), false, _t("auto")), _t("auto"));
        m_css_offsets.top.fromString(el->get_style_property(_t("top"), false, _t("auto")), _t("auto"));
        m_css_offsets.bottom.fromString(el->get_style_property(_t("bottom"), false, _t("auto")), _t("auto"));

        doc->cvt_units(m_css_offsets.left, m_font_size);
        doc->cvt_units(m_css_offsets.right, m_font_size);
        doc->cvt_units(m_css_offsets.top, m_font_size);
        doc->cvt_units(m_css_offsets.bottom, m_font_size);
    }

    const tchar_t* va	= el->get_style_property(_t("vertical-align"), true,	_t("baseline"));
    m_vertical_align = (vertical_align) value_index(va, vertical_align_strings, va_baseline);

    m_clear = (element_clear) value_index(el->get_style_property(_t("clear"), false, _t("none")), element_clear_strings, clear_none);

    m_css_text_indent.fromString(	el->get_style_property(_t("text-indent"),	true,	_t("0")),	_t("0"));

    m_css_width.fromString(			el->get_style_property(_t("width"),			false,	_t("auto")), _t("auto"));
    m_css_height.fromString(		el->get_style_property(_t("height"),		false,	_t("auto")), _t("auto"));

    doc->cvt_units(m_css_width, m_font_size);
    doc->cvt_units(m_css_height, m_font_size);

    m_css_min_width.fromString(		el->get_style_property(_t("min-width"),		false,	_t("0")));
    m_css_min_height.fromString(	el->get_style_property(_t("min-height"),	false,	_t("0")));

    m_css_max_width.fromString(		el->get_style_property(_t("max-width"),		false,	_t("none")),	_t("none"));
    m_css_max_height.fromString(	el->get_style_property(_t("max-height"),	false,	_t("none")),	_t("none"));

    doc->cvt_units(m_css_min_width, m_font_size);
    doc->cvt_units(m_css_min_height, m_font_size);

    m_css_margins.left.fromString(		el->get_style_property(_t("margin-left"),		false,	_t("0")), _t("auto"));
    m_css_margins.right.fromString(		el->get_style_property(_t("margin-right"),		false,	_t("0")), _t("auto"));
    m_css_margins.top.fromString(		el->get_style_property(_t("margin-top"),		false,	_t("0")), _t("auto"));
    m_css_margins.bottom.fromString(	el->get_style_property(_t("margin-bottom"),		false,	_t("0")), _t("auto"));

    doc->cvt_units(m_css_margins.left,	m_font_size);
    doc->cvt_units(m_css_margins.right,	m_font_size);
    doc->cvt_units(m_css_margins.top,		m_font_size);
    doc->cvt_units(m_css_margins.bottom,	m_font_size);

    m_css_padding.left.fromString(		el->get_style_property(_t("padding-left"),		false,	_t("0")), _t(""));
    m_css_padding.right.fromString(		el->get_style_property(_t("padding-right"),		false,	_t("0")), _t(""));
    m_css_padding.top.fromString(		el->get_style_property(_t("padding-top"),		false,	_t("0")), _t(""));
    m_css_padding.bottom.fromString(	el->get_style_property(_t("padding-bottom"),	false,	_t("0")), _t(""));

    doc->cvt_units(m_css_padding.left,	m_font_size);
    doc->cvt_units(m_css_padding.right,	m_font_size);
    doc->cvt_units(m_css_padding.top,		m_font_size);
    doc->cvt_units(m_css_padding.bottom,	m_font_size);

    m_css_borders.left.width.fromString(	el->get_style_property(_t("border-left-width"),		false,	_t("medium")), border_width_strings);
    m_css_borders.right.width.fromString(	el->get_style_property(_t("border-right-width"),	false,	_t("medium")), border_width_strings);
    m_css_borders.top.width.fromString(		el->get_style_property(_t("border-top-width"),		false,	_t("medium")), border_width_strings);
    m_css_borders.bottom.width.fromString(	el->get_style_property(_t("border-bottom-width"),	false,	_t("medium")), border_width_strings);

    doc->cvt_units(m_css_borders.left.width,	m_font_size);
    doc->cvt_units(m_css_borders.right.width,	m_font_size);
    doc->cvt_units(m_css_borders.top.width,		m_font_size);
    doc->cvt_units(m_css_borders.bottom.width,	m_font_size);

    m_css_borders.left.color = web_color::from_string(el->get_style_property(_t("border-left-color"),	false,	_t("")), doc->container());
    m_css_borders.left.style = (border_style) value_index(el->get_style_property(_t("border-left-style"), false, _t("none")), border_style_strings, border_style_none);

    m_css_borders.right.color = web_color::from_string(el->get_style_property(_t("border-right-color"), false, _t("")), doc->container());
    m_css_borders.right.style = (border_style) value_index(el->get_style_property(_t("border-right-style"), false, _t("none")), border_style_strings, border_style_none);

    m_css_borders.top.color = web_color::from_string(el->get_style_property(_t("border-top-color"), false, _t("")), doc->container());
    m_css_borders.top.style = (border_style) value_index(el->get_style_property(_t("border-top-style"), false, _t("none")), border_style_strings, border_style_none);

    m_css_borders.bottom.color = web_color::from_string(el->get_style_property(_t("border-bottom-color"), false, _t("")), doc->container());
    m_css_borders.bottom.style = (border_style) value_index(el->get_style_property(_t("border-bottom-style"), false, _t("none")), border_style_strings, border_style_none);

    m_css_borders.radius.top_left_x.fromString(el->get_style_property(_t("border-top-left-radius-x"), false, _t("0")));
    m_css_borders.radius.top_left_y.fromString(el->get_style_property(_t("border-top-left-radius-y"), false, _t("0")));

    m_css_borders.radius.top_right_x.fromString(el->get_style_property(_t("border-top-right-radius-x"), false, _t("0")));
    m_css_borders.radius.top_right_y.fromString(el->get_style_property(_t("border-top-right-radius-y"), false, _t("0")));

    m_css_borders.radius.bottom_right_x.fromString(el->get_style_property(_t("border-bottom-right-radius-x"), false, _t("0")));
    m_css_borders.radius.bottom_right_y.fromString(el->get_style_property(_t("border-bottom-right-radius-y"), false, _t("0")));

    m_css_borders.radius.bottom_left_x.fromString(el->get_style_property(_t("border-bottom-left-radius-x"), false, _t("0")));
    m_css_borders.radius.bottom_left_y.fromString(el->get_style_property(_t("border-bottom-left-radius-y"), false, _t("0")));

    doc->cvt_units(m_css_borders.radius.bottom_left_x,			m_font_size);
    doc->cvt_units(m_css_borders.radius.bottom_left_y,			m_font_size);
    doc->cvt_units(m_css_borders.radius.bottom_right_x,			m_font_size);
    doc->cvt_units(m_css_borders.radius.bottom_right_y,			m_font_size);
    doc->cvt_units(m_css_borders.radius.top_left_x,				m_font_size);
    doc->cvt_units(m_css_borders.radius.top_left_y,				m_font_size);
    doc->cvt_units(m_css_borders.radius.top_right_x,			m_font_size);
    doc->cvt_units(m_css_borders.radius.top_right_y,			m_font_size);

    doc->cvt_units(m_css_text_indent,							m_font_size);

    css_length line_height;
    line_height.fromString(el->get_style_property(_t("line-height"),	true,	_t("normal")), _t("normal"));
    if(line_height.is_predefined())
    {
        m_line_height = m_font_metrics.height;
        m_lh_predefined = true;
    } else if(line_height.units() == css_units_none)
    {
        m_line_height = (int) (line_height.val() * (float) m_font_size);
        m_lh_predefined = false;
    } else
    {
        m_line_height =  doc->to_pixels(line_height, m_font_size, m_font_size);
        m_lh_predefined = false;
    }


    if(m_display == display_list_item)
    {
        const tchar_t* list_type = el->get_style_property(_t("list-style-type"), true, _t("disc"));
        m_list_style_type = (list_style_type) value_index(list_type, list_style_type_strings, list_style_type_disc);

        const tchar_t* list_pos = el->get_style_property(_t("list-style-position"), true, _t("outside"));
        m_list_style_position = (list_style_position) value_index(list_pos, list_style_position_strings, list_style_position_outside);

        const tchar_t* list_image = el->get_style_property(_t("list-style-image"), true, nullptr);
        if(list_image && list_image[0])
        {
            tstring url;
            css::parse_css_url(list_image, url);

            const tchar_t* list_image_baseurl = el->get_style_property(_t("list-style-image-baseurl"), true, nullptr);
            doc->container()->load_image(url.c_str(), list_image_baseurl, true);
        }

    }

    m_text_transform	= (text_transform)	value_index(el->get_style_property(_t("text-transform"), true,	_t("none")),	text_transform_strings,	text_transform_none);

    m_border_collapse = (border_collapse) value_index(el->get_style_property(_t("border-collapse"), true, _t("separate")), border_collapse_strings, border_collapse_separate);

    if(m_border_collapse == border_collapse_separate)
    {
        m_css_border_spacing_x.fromString(el->get_style_property(_t("-litehtml-border-spacing-x"), true, _t("0px")));
        m_css_border_spacing_y.fromString(el->get_style_property(_t("-litehtml-border-spacing-y"), true, _t("0px")));

        doc->cvt_units(m_css_border_spacing_x, m_font_size);
        doc->cvt_units(m_css_border_spacing_y, m_font_size);
    }

    parse_background(el, doc);
    parse_flex(el, doc);
}

void litehtml::css_properties::parse_font(const std::shared_ptr<element>& el, const std::shared_ptr<document>& doc)
{
    // initialize font size
    const tchar_t* str = el->get_style_property(_t("font-size"), false, nullptr);

    int parent_sz = 0;
    int doc_font_size = doc->container()->get_default_font_size();
    element::ptr el_parent = el->parent();
    if (el_parent)
    {
        parent_sz = el_parent->css().get_font_size();
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
        sz.fromString(str, font_size_strings, -1);
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
                    m_font_size = parent_sz;
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
                        m_font_size = parent_sz;
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
                m_font_size = doc->to_pixels(sz, parent_sz);
            }
        }
    }

    // initialize font
    const tchar_t* name			= el->get_style_property(_t("font-family"),		true,	_t("inherit"));
    const tchar_t* weight		= el->get_style_property(_t("font-weight"),		true,	_t("normal"));
    const tchar_t* style		= el->get_style_property(_t("font-style"),		true,	_t("normal"));
    const tchar_t* decoration	= el->get_style_property(_t("text-decoration"),	true,	_t("none"));

    m_font = doc->get_font(name, m_font_size, weight, style, decoration, &m_font_metrics);
}

void litehtml::css_properties::parse_background(const std::shared_ptr<element>& el, const std::shared_ptr<document>& doc)
{
    // parse background-color
    m_bg.m_color		= el->get_color(_t("background-color"), false, web_color(0, 0, 0, 0));

    // parse background-position
    const tchar_t* str = el->get_style_property(_t("background-position"), false, _t("0% 0%"));
    if(str)
    {
        string_vector res;
        split_string(str, res, _t(" \t"));
        if(!res.empty())
        {
            if(res.size() == 1)
            {
                if( value_in_list(res[0], _t("left;right;center")) )
                {
                    m_bg.m_position.x.fromString(res[0], _t("left;right;center"));
                    m_bg.m_position.y.set_value(50, css_units_percentage);
                } else if( value_in_list(res[0], _t("top;bottom;center")) )
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
                if(value_in_list(res[0], _t("left;right")))
                {
                    m_bg.m_position.x.fromString(res[0], _t("left;right;center"));
                    m_bg.m_position.y.fromString(res[1], _t("top;bottom;center"));
                } else if(value_in_list(res[0], _t("top;bottom")))
                {
                    m_bg.m_position.x.fromString(res[1], _t("left;right;center"));
                    m_bg.m_position.y.fromString(res[0], _t("top;bottom;center"));
                } else if(value_in_list(res[1], _t("left;right")))
                {
                    m_bg.m_position.x.fromString(res[1], _t("left;right;center"));
                    m_bg.m_position.y.fromString(res[0], _t("top;bottom;center"));
                }else if(value_in_list(res[1], _t("top;bottom")))
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

    str = el->get_style_property(_t("background-size"), false, _t("auto"));
    if(str)
    {
        string_vector res;
        split_string(str, res, _t(" \t"));
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

    doc->cvt_units(m_bg.m_position.x,		m_font_size);
    doc->cvt_units(m_bg.m_position.y,		m_font_size);
    doc->cvt_units(m_bg.m_position.width,	m_font_size);
    doc->cvt_units(m_bg.m_position.height,	m_font_size);

    // parse background_attachment
    m_bg.m_attachment = (background_attachment) value_index(
            el->get_style_property(_t("background-attachment"), false, _t("scroll")),
            background_attachment_strings,
            background_attachment_scroll);

    // parse background_attachment
    m_bg.m_repeat = (background_repeat) value_index(
            el->get_style_property(_t("background-repeat"), false, _t("repeat")),
            background_repeat_strings,
            background_repeat_repeat);

    // parse background_clip
    m_bg.m_clip = (background_box) value_index(
            el->get_style_property(_t("background-clip"), false, _t("border-box")),
            background_box_strings,
            background_box_border);

    // parse background_origin
    m_bg.m_origin = (background_box) value_index(
            el->get_style_property(_t("background-origin"), false, _t("padding-box")),
            background_box_strings,
            background_box_content);

    // parse background-image
    css::parse_css_url(el->get_style_property(_t("background-image"), false, _t("")), m_bg.m_image);
    m_bg.m_baseurl = el->get_style_property(_t("background-image-baseurl"), false, _t(""));

    if(!m_bg.m_image.empty())
    {
        doc->container()->load_image(m_bg.m_image.c_str(), m_bg.m_baseurl.empty() ? nullptr : m_bg.m_baseurl.c_str(), true);
    }
}

void litehtml::css_properties::parse_flex(const std::shared_ptr<element>& el, const std::shared_ptr<document>& doc)
{
    if(m_display == display_flex)
    {
        m_flex_direction = (flex_direction) value_index(el->get_style_property(_t("flex-direction"), false, _t("row")), flex_direction_strings, flex_direction_row);
        m_flex_wrap = (flex_wrap) value_index(el->get_style_property(_t("flex-wrap"), false, _t("nowrap")), flex_wrap_strings, flex_wrap_nowrap);

        m_flex_justify_content = (flex_justify_content) value_index(el->get_style_property(_t("justify-content"), false, _t("flex-start")), flex_justify_content_strings, flex_justify_content_flex_start);
        m_flex_align_items = (flex_align_items) value_index(el->get_style_property(_t("align-items"), false, _t("stretch")), flex_align_items_strings, flex_align_items_stretch);
        m_flex_align_content = (flex_align_content) value_index(el->get_style_property(_t("align-content"), false, _t("stretch")), flex_align_content_strings, flex_align_content_stretch);
    }
    auto parent = el->parent();
    if(parent && parent->css().m_display == display_flex)
    {
        m_flex_grow = (float) t_strtod(el->get_style_property(_t("flex-grow"), false, _t("0")), nullptr);
        m_flex_shrink = (float) t_strtod(el->get_style_property(_t("flex-shrink"), false, _t("1")), nullptr);
        m_flex_align_self = (flex_align_self) value_index(el->get_style_property(_t("align-self"), false, _t("auto")), flex_align_self_strings, flex_align_self_auto);
        m_flex_basis.fromString(el->get_style_property(_t("flex-shrink"), false, _t("auto")));
        doc->cvt_units(m_flex_basis,	m_font_size);
        if(m_display == display_inline || m_display == display_inline_block)
        {
            m_display = display_block;
        } else if(m_display == display_inline_table)
        {
            m_display = display_table;
        } else if(m_display == display_inline_flex)
        {
            m_display = display_flex;
        }
    }
}

std::vector<std::tuple<litehtml::tstring, litehtml::tstring>> litehtml::css_properties::dump_get_attrs()
{
    std::vector<std::tuple<litehtml::tstring, litehtml::tstring>> ret;

    ret.emplace_back(std::make_tuple(_t("display"), index_value(m_display, style_display_strings)));
    ret.emplace_back(std::make_tuple(_t("el_position"), index_value(m_el_position, element_position_strings)));
    ret.emplace_back(std::make_tuple(_t("text_align"), index_value(m_text_align, text_align_strings)));
    ret.emplace_back(std::make_tuple(_t("font_size"), t_to_string(m_font_size)));
    ret.emplace_back(std::make_tuple(_t("overflow"), index_value(m_overflow, overflow_strings)));
    ret.emplace_back(std::make_tuple(_t("white_space"), index_value(m_white_space, white_space_strings)));
    ret.emplace_back(std::make_tuple(_t("visibility"), index_value(m_visibility, visibility_strings)));
    ret.emplace_back(std::make_tuple(_t("box_sizing"), index_value(m_box_sizing, box_sizing_strings)));
    ret.emplace_back(std::make_tuple(_t("z_index"), t_to_string(m_z_index)));
    ret.emplace_back(std::make_tuple(_t("vertical_align"), index_value(m_vertical_align, vertical_align_strings)));
    ret.emplace_back(std::make_tuple(_t("float"), index_value(m_float, element_float_strings)));
    ret.emplace_back(std::make_tuple(_t("clear"), index_value(m_clear, element_clear_strings)));
    ret.emplace_back(std::make_tuple(_t("margins"), m_css_margins.to_string()));
    ret.emplace_back(std::make_tuple(_t("padding"), m_css_padding.to_string()));
    ret.emplace_back(std::make_tuple(_t("borders"), m_css_borders.to_string()));
    ret.emplace_back(std::make_tuple(_t("width"), m_css_width.to_string()));
    ret.emplace_back(std::make_tuple(_t("height"), m_css_height.to_string()));
    ret.emplace_back(std::make_tuple(_t("min_width"), m_css_min_width.to_string()));
    ret.emplace_back(std::make_tuple(_t("min_height"), m_css_min_width.to_string()));
    ret.emplace_back(std::make_tuple(_t("max_width"), m_css_max_width.to_string()));
    ret.emplace_back(std::make_tuple(_t("max_height"), m_css_max_width.to_string()));
    ret.emplace_back(std::make_tuple(_t("offsets"), m_css_offsets.to_string()));
    ret.emplace_back(std::make_tuple(_t("text_indent"), m_css_text_indent.to_string()));
    ret.emplace_back(std::make_tuple(_t("line_height"), t_to_string(m_line_height)));
    ret.emplace_back(std::make_tuple(_t("list_style_type"), index_value(m_list_style_type, list_style_type_strings)));
    ret.emplace_back(std::make_tuple(_t("list_style_position"), index_value(m_list_style_position, list_style_position_strings)));
    ret.emplace_back(std::make_tuple(_t("border_spacing_x"), m_css_border_spacing_x.to_string()));
    ret.emplace_back(std::make_tuple(_t("border_spacing_y"), m_css_border_spacing_y.to_string()));

    return ret;
}
