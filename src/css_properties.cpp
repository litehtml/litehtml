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

    m_el_position	= (element_position)	value_index(el->get_style_property(_position_,		false,	"static"),		element_position_strings,	element_position_fixed);
    m_text_align	= (text_align)			value_index(el->get_style_property(_text_align_,	true,	"left"),		text_align_strings,			text_align_left);
    m_overflow		= (overflow)			value_index(el->get_style_property(_overflow_,		false,	"visible"),		overflow_strings,			overflow_visible);
    m_white_space	= (white_space)			value_index(el->get_style_property(_white_space_,	true,	"normal"),		white_space_strings,		white_space_normal);
    m_display		= (style_display)		value_index(el->get_style_property(_display_,		false,	"inline"),		style_display_strings,		display_inline);
    m_visibility	= (visibility)			value_index(el->get_style_property(_visibility_,	true,	"visible"),		visibility_strings,			visibility_visible);
    m_box_sizing	= (box_sizing)			value_index(el->get_style_property(_box_sizing_,	false,	"content-box"),	box_sizing_strings,			box_sizing_content_box);

    if(m_el_position != element_position_static)
    {
        const char* val = el->get_style_property(_z_index_, false, nullptr);
        if(val)
        {
            m_z_index = atoi(val);
        }
    }

    const char* fl	= el->get_style_property(_float_, false,	"none");
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
        m_css_offsets.left.fromString(el->get_style_property(_left_, false, "auto"), "auto");
        m_css_offsets.right.fromString(el->get_style_property(_right_, false, "auto"), "auto");
        m_css_offsets.top.fromString(el->get_style_property(_top_, false, "auto"), "auto");
        m_css_offsets.bottom.fromString(el->get_style_property(_bottom_, false, "auto"), "auto");

        doc->cvt_units(m_css_offsets.left, m_font_size);
        doc->cvt_units(m_css_offsets.right, m_font_size);
        doc->cvt_units(m_css_offsets.top, m_font_size);
        doc->cvt_units(m_css_offsets.bottom, m_font_size);
    }

    const char* va	= el->get_style_property(_vertical_align_, true,	"baseline");
    m_vertical_align = (vertical_align) value_index(va, vertical_align_strings, va_baseline);

    m_clear = (element_clear) value_index(el->get_style_property(_clear_, false, "none"), element_clear_strings, clear_none);

    m_css_text_indent.fromString(	el->get_style_property(_text_indent_,	true,	"0"),	"0");

    m_css_width.fromString(			el->get_style_property(_width_,			false,	"auto"), "auto");
    m_css_height.fromString(		el->get_style_property(_height_,		false,	"auto"), "auto");

    doc->cvt_units(m_css_width, m_font_size);
    doc->cvt_units(m_css_height, m_font_size);

    m_css_min_width.fromString(		el->get_style_property(_min_width_,		false,	"0"));
    m_css_min_height.fromString(	el->get_style_property(_min_height_,	false,	"0"));

    m_css_max_width.fromString(		el->get_style_property(_max_width_,		false,	"none"),	"none");
    m_css_max_height.fromString(	el->get_style_property(_max_height_,	false,	"none"),	"none");

    doc->cvt_units(m_css_min_width, m_font_size);
    doc->cvt_units(m_css_min_height, m_font_size);

    m_css_margins.left.fromString(		el->get_style_property(_margin_left_,		false,	"0"), "auto");
    m_css_margins.right.fromString(		el->get_style_property(_margin_right_,		false,	"0"), "auto");
    m_css_margins.top.fromString(		el->get_style_property(_margin_top_,		false,	"0"), "auto");
    m_css_margins.bottom.fromString(	el->get_style_property(_margin_bottom_,		false,	"0"), "auto");

    doc->cvt_units(m_css_margins.left,	m_font_size);
    doc->cvt_units(m_css_margins.right,	m_font_size);
    doc->cvt_units(m_css_margins.top,		m_font_size);
    doc->cvt_units(m_css_margins.bottom,	m_font_size);

    m_css_padding.left.fromString(		el->get_style_property(_padding_left_,		false,	"0"), "");
    m_css_padding.right.fromString(		el->get_style_property(_padding_right_,		false,	"0"), "");
    m_css_padding.top.fromString(		el->get_style_property(_padding_top_,		false,	"0"), "");
    m_css_padding.bottom.fromString(	el->get_style_property(_padding_bottom_,	false,	"0"), "");

    doc->cvt_units(m_css_padding.left,	m_font_size);
    doc->cvt_units(m_css_padding.right,	m_font_size);
    doc->cvt_units(m_css_padding.top,		m_font_size);
    doc->cvt_units(m_css_padding.bottom,	m_font_size);

    m_css_borders.left.width.fromString(	el->get_style_property(_border_left_width_,		false,	"medium"), border_width_strings);
    m_css_borders.right.width.fromString(	el->get_style_property(_border_right_width_,	false,	"medium"), border_width_strings);
    m_css_borders.top.width.fromString(		el->get_style_property(_border_top_width_,		false,	"medium"), border_width_strings);
    m_css_borders.bottom.width.fromString(	el->get_style_property(_border_bottom_width_,	false,	"medium"), border_width_strings);

    doc->cvt_units(m_css_borders.left.width,	m_font_size);
    doc->cvt_units(m_css_borders.right.width,	m_font_size);
    doc->cvt_units(m_css_borders.top.width,		m_font_size);
    doc->cvt_units(m_css_borders.bottom.width,	m_font_size);

    m_css_borders.left.color = web_color::from_string(el->get_style_property(_border_left_color_,	false,	""), doc->container());
    m_css_borders.left.style = (border_style) value_index(el->get_style_property(_border_left_style_, false, "none"), border_style_strings, border_style_none);

    m_css_borders.right.color = web_color::from_string(el->get_style_property(_border_right_color_, false, ""), doc->container());
    m_css_borders.right.style = (border_style) value_index(el->get_style_property(_border_right_style_, false, "none"), border_style_strings, border_style_none);

    m_css_borders.top.color = web_color::from_string(el->get_style_property(_border_top_color_, false, ""), doc->container());
    m_css_borders.top.style = (border_style) value_index(el->get_style_property(_border_top_style_, false, "none"), border_style_strings, border_style_none);

    m_css_borders.bottom.color = web_color::from_string(el->get_style_property(_border_bottom_color_, false, ""), doc->container());
    m_css_borders.bottom.style = (border_style) value_index(el->get_style_property(_border_bottom_style_, false, "none"), border_style_strings, border_style_none);

    m_css_borders.radius.top_left_x.fromString(el->get_style_property(_border_top_left_radius_x_, false, "0"));
    m_css_borders.radius.top_left_y.fromString(el->get_style_property(_border_top_left_radius_y_, false, "0"));

    m_css_borders.radius.top_right_x.fromString(el->get_style_property(_border_top_right_radius_x_, false, "0"));
    m_css_borders.radius.top_right_y.fromString(el->get_style_property(_border_top_right_radius_y_, false, "0"));

    m_css_borders.radius.bottom_right_x.fromString(el->get_style_property(_border_bottom_right_radius_x_, false, "0"));
    m_css_borders.radius.bottom_right_y.fromString(el->get_style_property(_border_bottom_right_radius_y_, false, "0"));

    m_css_borders.radius.bottom_left_x.fromString(el->get_style_property(_border_bottom_left_radius_x_, false, "0"));
    m_css_borders.radius.bottom_left_y.fromString(el->get_style_property(_border_bottom_left_radius_y_, false, "0"));

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
    line_height.fromString(el->get_style_property(_line_height_,	true,	"normal"), "normal");
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
        const char* list_type = el->get_style_property(_list_style_type_, true, "disc");
        m_list_style_type = (list_style_type) value_index(list_type, list_style_type_strings, list_style_type_disc);

        const char* list_pos = el->get_style_property(_list_style_position_, true, "outside");
        m_list_style_position = (list_style_position) value_index(list_pos, list_style_position_strings, list_style_position_outside);

        const char* list_image = el->get_style_property(_list_style_image_, true, nullptr);
        if(list_image && list_image[0])
        {
            string url;
            css::parse_css_url(list_image, url);

            const char* list_image_baseurl = el->get_style_property(_list_style_image_baseurl_, true, nullptr);
            doc->container()->load_image(url.c_str(), list_image_baseurl, true);
        }

    }

    m_text_transform  = (text_transform) value_index(el->get_style_property(_text_transform_, true,	"none"),	text_transform_strings,	text_transform_none);

    m_border_collapse = (border_collapse) value_index(el->get_style_property(_border_collapse_, true, "separate"), border_collapse_strings, border_collapse_separate);

    if(m_border_collapse == border_collapse_separate)
    {
        m_css_border_spacing_x.fromString(el->get_style_property(__litehtml_border_spacing_x_, true, "0px"));
        m_css_border_spacing_y.fromString(el->get_style_property(__litehtml_border_spacing_y_, true, "0px"));

        doc->cvt_units(m_css_border_spacing_x, m_font_size);
        doc->cvt_units(m_css_border_spacing_y, m_font_size);
    }

    parse_background(el, doc);
    parse_flex(el, doc);
}

void litehtml::css_properties::parse_font(const std::shared_ptr<element>& el, const std::shared_ptr<document>& doc)
{
    // initialize font size
    const char* str = el->get_style_property(_font_size_, false, nullptr);

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
    const char* name		= el->get_style_property(_font_family_,		true,	"inherit");
    const char* weight		= el->get_style_property(_font_weight_,		true,	"normal");
    const char* style		= el->get_style_property(_font_style_,		true,	"normal");
    const char* decoration	= el->get_style_property(_text_decoration_,	true,	"none");

    m_font = doc->get_font(name, m_font_size, weight, style, decoration, &m_font_metrics);
}

void litehtml::css_properties::parse_background(const std::shared_ptr<element>& el, const std::shared_ptr<document>& doc)
{
    // parse background-color
    m_bg.m_color		= el->get_color(_background_color_, false, web_color(0, 0, 0, 0));

    // parse background-position
    const char* str = el->get_style_property(_background_position_, false, "0% 0%");
    if(str)
    {
        string_vector res;
        split_string(str, res, " \t");
        if(!res.empty())
        {
            if(res.size() == 1)
            {
                if( value_in_list(res[0], "left;right;center") )
                {
                    m_bg.m_position.x.fromString(res[0], "left;right;center");
                    m_bg.m_position.y.set_value(50, css_units_percentage);
                } else if( value_in_list(res[0], "top;bottom;center") )
                {
                    m_bg.m_position.y.fromString(res[0], "top;bottom;center");
                    m_bg.m_position.x.set_value(50, css_units_percentage);
                } else
                {
                    m_bg.m_position.x.fromString(res[0], "left;right;center");
                    m_bg.m_position.y.set_value(50, css_units_percentage);
                }
            } else
            {
                if(value_in_list(res[0], "left;right"))
                {
                    m_bg.m_position.x.fromString(res[0], "left;right;center");
                    m_bg.m_position.y.fromString(res[1], "top;bottom;center");
                } else if(value_in_list(res[0], "top;bottom"))
                {
                    m_bg.m_position.x.fromString(res[1], "left;right;center");
                    m_bg.m_position.y.fromString(res[0], "top;bottom;center");
                } else if(value_in_list(res[1], "left;right"))
                {
                    m_bg.m_position.x.fromString(res[1], "left;right;center");
                    m_bg.m_position.y.fromString(res[0], "top;bottom;center");
                }else if(value_in_list(res[1], "top;bottom"))
                {
                    m_bg.m_position.x.fromString(res[0], "left;right;center");
                    m_bg.m_position.y.fromString(res[1], "top;bottom;center");
                } else
                {
                    m_bg.m_position.x.fromString(res[0], "left;right;center");
                    m_bg.m_position.y.fromString(res[1], "top;bottom;center");
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

    str = el->get_style_property(_background_size_, false, "auto");
    if(str)
    {
        string_vector res;
        split_string(str, res, " \t");
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
            el->get_style_property(_background_attachment_, false, "scroll"),
            background_attachment_strings,
            background_attachment_scroll);

    // parse background_attachment
    m_bg.m_repeat = (background_repeat) value_index(
            el->get_style_property(_background_repeat_, false, "repeat"),
            background_repeat_strings,
            background_repeat_repeat);

    // parse background_clip
    m_bg.m_clip = (background_box) value_index(
            el->get_style_property(_background_clip_, false, "border-box"),
            background_box_strings,
            background_box_border);

    // parse background_origin
    m_bg.m_origin = (background_box) value_index(
            el->get_style_property(_background_origin_, false, "padding-box"),
            background_box_strings,
            background_box_content);

    // parse background-image
    css::parse_css_url(el->get_style_property(_background_image_, false, ""), m_bg.m_image);
    m_bg.m_baseurl = el->get_style_property(_background_image_baseurl_, false, "");

    if(!m_bg.m_image.empty())
    {
        doc->container()->load_image(m_bg.m_image.c_str(), m_bg.m_baseurl.empty() ? nullptr : m_bg.m_baseurl.c_str(), true);
    }
}

void litehtml::css_properties::parse_flex(const std::shared_ptr<element>& el, const std::shared_ptr<document>& doc)
{
    if(m_display == display_flex)
    {
        m_flex_direction = (flex_direction) value_index(el->get_style_property(_flex_direction_, false, "row"), flex_direction_strings, flex_direction_row);
        m_flex_wrap = (flex_wrap) value_index(el->get_style_property(_flex_wrap_, false, "nowrap"), flex_wrap_strings, flex_wrap_nowrap);

        m_flex_justify_content = (flex_justify_content) value_index(el->get_style_property(_justify_content_, false, "flex-start"), flex_justify_content_strings, flex_justify_content_flex_start);
        m_flex_align_items = (flex_align_items) value_index(el->get_style_property(_align_items_, false, "stretch"), flex_align_items_strings, flex_align_items_stretch);
        m_flex_align_content = (flex_align_content) value_index(el->get_style_property(_align_content_, false, "stretch"), flex_align_content_strings, flex_align_content_stretch);
    }
    auto parent = el->parent();
    if(parent && parent->css().m_display == display_flex)
    {
        m_flex_grow = (float) t_strtod(el->get_style_property(_flex_grow_, false, "0"), nullptr);
        m_flex_shrink = (float) t_strtod(el->get_style_property(_flex_shrink_, false, "1"), nullptr);
        m_flex_align_self = (flex_align_self) value_index(el->get_style_property(_align_self_, false, "auto"), flex_align_self_strings, flex_align_self_auto);
        m_flex_basis.fromString(el->get_style_property(_flex_shrink_, false, "auto"));
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

std::vector<std::tuple<litehtml::string, litehtml::string>> litehtml::css_properties::dump_get_attrs()
{
    std::vector<std::tuple<string, string>> ret;

    ret.emplace_back(std::make_tuple("display", index_value(m_display, style_display_strings)));
    ret.emplace_back(std::make_tuple("el_position", index_value(m_el_position, element_position_strings)));
    ret.emplace_back(std::make_tuple("text_align", index_value(m_text_align, text_align_strings)));
    ret.emplace_back(std::make_tuple("font_size", std::to_string(m_font_size)));
    ret.emplace_back(std::make_tuple("overflow", index_value(m_overflow, overflow_strings)));
    ret.emplace_back(std::make_tuple("white_space", index_value(m_white_space, white_space_strings)));
    ret.emplace_back(std::make_tuple("visibility", index_value(m_visibility, visibility_strings)));
    ret.emplace_back(std::make_tuple("box_sizing", index_value(m_box_sizing, box_sizing_strings)));
    ret.emplace_back(std::make_tuple("z_index", std::to_string(m_z_index)));
    ret.emplace_back(std::make_tuple("vertical_align", index_value(m_vertical_align, vertical_align_strings)));
    ret.emplace_back(std::make_tuple("float", index_value(m_float, element_float_strings)));
    ret.emplace_back(std::make_tuple("clear", index_value(m_clear, element_clear_strings)));
    ret.emplace_back(std::make_tuple("margins", m_css_margins.to_string()));
    ret.emplace_back(std::make_tuple("padding", m_css_padding.to_string()));
    ret.emplace_back(std::make_tuple("borders", m_css_borders.to_string()));
    ret.emplace_back(std::make_tuple("width", m_css_width.to_string()));
    ret.emplace_back(std::make_tuple("height", m_css_height.to_string()));
    ret.emplace_back(std::make_tuple("min_width", m_css_min_width.to_string()));
    ret.emplace_back(std::make_tuple("min_height", m_css_min_width.to_string()));
    ret.emplace_back(std::make_tuple("max_width", m_css_max_width.to_string()));
    ret.emplace_back(std::make_tuple("max_height", m_css_max_width.to_string()));
    ret.emplace_back(std::make_tuple("offsets", m_css_offsets.to_string()));
    ret.emplace_back(std::make_tuple("text_indent", m_css_text_indent.to_string()));
    ret.emplace_back(std::make_tuple("line_height", std::to_string(m_line_height)));
    ret.emplace_back(std::make_tuple("list_style_type", index_value(m_list_style_type, list_style_type_strings)));
    ret.emplace_back(std::make_tuple("list_style_position", index_value(m_list_style_position, list_style_position_strings)));
    ret.emplace_back(std::make_tuple("border_spacing_x", m_css_border_spacing_x.to_string()));
    ret.emplace_back(std::make_tuple("border_spacing_y", m_css_border_spacing_y.to_string()));

    return ret;
}
