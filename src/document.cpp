#include "html.h"
#include "document.h"
#include "stylesheet.h"
#include "html_tag.h"
#include "el_text.h"
#include "el_para.h"
#include "el_space.h"
#include "el_body.h"
#include "el_image.h"
#include "el_table.h"
#include "el_td.h"
#include "el_link.h"
#include "el_title.h"
#include "el_style.h"
#include "el_script.h"
#include "el_comment.h"
#include "el_cdata.h"
#include "el_base.h"
#include "el_anchor.h"
#include "el_break.h"
#include "el_div.h"
#include "el_font.h"
#include "el_tr.h"
#include <math.h>
#include <stdio.h>
#include <algorithm>
#include "gumbo/gumbo.h"
#include "utf8_strings.h"

litehtml::document::document(litehtml::document_container* objContainer, litehtml::context* ctx)
{
	m_container	= objContainer;
	m_context	= ctx;
}

litehtml::document::~document()
{
	m_over_element = 0;
	if(m_container)
	{
		for(fonts_map::iterator f = m_fonts.begin(); f != m_fonts.end(); f++)
		{
			m_container->delete_font(f->second.font);
		}
	}
}

litehtml::document::ptr litehtml::document::createFromString( const tchar_t* str, litehtml::document_container* objPainter, litehtml::context* ctx, litehtml::css* user_styles)
{
	return createFromUTF8(litehtml_to_utf8(str), objPainter, ctx, user_styles);
}

litehtml::document::ptr litehtml::document::createFromUTF8(const char* str, litehtml::document_container* objPainter, litehtml::context* ctx, litehtml::css* user_styles)
{
	// Create litehtml::document
	litehtml::document::ptr doc = new litehtml::document(objPainter, ctx);

    doc->m_root = set_inner_html( doc, str );

	return doc;
}

litehtml::element::ptr litehtml::document::set_inner_html( litehtml::document::ptr & document, const char* text, litehtml::css* user_styles )
{
    GumboOutput
        * output = gumbo_parse((const char*) text);
    elements_vector
        root_elements;
    litehtml::element::ptr
        root_element;

    document->create_node(output->root, root_elements);

    // Destroy GumboOutput
    gumbo_destroy_output(&kGumboDefaultOptions, output);

    if ( !root_elements.empty() )
    {
        root_element = root_elements.back();
    }

    // Let's process created elements tree
    if ( root_element )
    {
        document->container()->get_media_features(document->m_media);

        // apply master CSS
        root_element->apply_stylesheet( document->m_context->master_css() );

        // parse elements attributes
        root_element->parse_attributes();

        // parse style sheets linked in document
        media_query_list::ptr media;
        for (css_text::vector::iterator css = document->m_css.begin(); css != document->m_css.end(); css++)
        {
            if (!css->media.empty())
            {
                media = media_query_list::create_from_string(css->media, document);
            }
            else
            {
                media = 0;
            }
            document->m_styles.parse_stylesheet(css->text.c_str(), css->baseurl.c_str(), document, media);
        }
        // Sort css selectors using CSS rules.
        document->m_styles.sort_selectors();

		// get current media features
		if (!document->m_media_lists.empty())
		{
			document->update_media_lists(document->m_media);
		}

        // Apply parsed styles.
        root_element->apply_stylesheet(document->m_styles);

        // Apply user styles if any
        if (user_styles)
        {
            root_element->apply_stylesheet(*user_styles);
        }

        // Parse applied styles in the elements
        root_element->parse_styles();

        // Now the m_tabular_elements is filled with tabular elements.
        // We have to check the tabular elements for missing table elements
        // and create the anonymous boxes in visual table layout
        document->fix_tables_layout();

        // Finally initialize elements
        root_element->init();
    }

    return root_element;
}

litehtml::uint_ptr litehtml::document::add_font( const tchar_t* name, int size, const tchar_t* weight, const tchar_t* style, const tchar_t* decoration, font_metrics* fm )
{
	uint_ptr ret = 0;

	if( !name || (name && !t_strcasecmp(name, _t("inherit"))) )
	{
		name = m_container->get_default_font_name();
	}

	if(!size)
	{
		size = container()->get_default_font_size();
	}

	tchar_t strSize[20];
	t_itoa(size, strSize, 20, 10);

	tstring key = name;
	key += _t(":");
	key += strSize;
	key += _t(":");
	key += weight;
	key += _t(":");
	key += style;
	key += _t(":");
	key += decoration;

	if(m_fonts.find(key) == m_fonts.end())
	{
		font_style fs = (font_style) value_index(style, font_style_strings, fontStyleNormal);
		int	fw = value_index(weight, font_weight_strings, -1);
		if(fw >= 0)
		{
			switch(fw)
			{
			case litehtml::fontWeightBold:
				fw = 700;
				break;
			case litehtml::fontWeightBolder:
				fw = 600;
				break;
			case litehtml::fontWeightLighter:
				fw = 300;
				break;
			default:
				fw = 400;
				break;
			}
		} else
		{
			fw = t_atoi(weight);
			if(fw < 100)
			{
				fw = 400;
			}
		}

		unsigned int decor = 0;

		if(decoration)
		{
			std::vector<tstring> tokens;
			split_string(decoration, tokens, _t(" "));
			for(std::vector<tstring>::iterator i = tokens.begin(); i != tokens.end(); i++)
			{
				if(!t_strcasecmp(i->c_str(), _t("underline")))
				{
					decor |= font_decoration_underline;
				} else if(!t_strcasecmp(i->c_str(), _t("line-through")))
				{
					decor |= font_decoration_linethrough;
				} else if(!t_strcasecmp(i->c_str(), _t("overline")))
				{
					decor |= font_decoration_overline;
				}
			}
		}

		font_item fi= {0};

		fi.font = m_container->create_font(name, size, fw, fs, decor, &fi.metrics);
		m_fonts[key] = fi;
		ret = fi.font;
		if(fm)
		{
			*fm = fi.metrics;
		}
	}
	return ret;
}

litehtml::uint_ptr litehtml::document::get_font( const tchar_t* name, int size, const tchar_t* weight, const tchar_t* style, const tchar_t* decoration, font_metrics* fm )
{
	if( !name || (name && !t_strcasecmp(name, _t("inherit"))) )
	{
		name = m_container->get_default_font_name();
	}

	if(!size)
	{
		size = container()->get_default_font_size();
	}

	tchar_t strSize[20];
	t_itoa(size, strSize, 20, 10);

	tstring key = name;
	key += _t(":");
	key += strSize;
	key += _t(":");
	key += weight;
	key += _t(":");
	key += style;
	key += _t(":");
	key += decoration;

	fonts_map::iterator el = m_fonts.find(key);

	if(el != m_fonts.end())
	{
		if(fm)
		{
			*fm = el->second.metrics;
		}
		return el->second.font;
	}
	return add_font(name, size, weight, style, decoration, fm);
}

int litehtml::document::render( int max_width, render_type rt )
{
	int ret = 0;
	if(m_root)
	{
		if(rt == render_fixed_only)
		{
			m_fixed_boxes.clear();
			m_root->render_positioned(rt);
		} else
		{
			ret = m_root->render(0, 0, max_width);
			if(m_root->fetch_positioned())
			{
				m_fixed_boxes.clear();
				m_root->render_positioned(rt);
			}
			m_size.width	= 0;
			m_size.height	= 0;
			m_root->calc_document_size(m_size);
		}
	}
	return ret;
}

void litehtml::document::draw( uint_ptr hdc, int x, int y, const position* clip )
{
	if(m_root)
	{
		m_root->draw(hdc, x, y, clip);
		m_root->draw_stacking_context(hdc, x, y, clip, true);
	}
}

int litehtml::document::cvt_units( const tchar_t* str, int fontSize, bool* is_percent/*= 0*/ ) const
{
	if(!str)	return 0;

	css_length val;
	val.fromString(str);
	if(is_percent && val.units() == css_units_percentage && !val.is_predefined())
	{
		*is_percent = true;
	}
	return cvt_units(val, fontSize);
}

int litehtml::document::cvt_units( css_length& val, int fontSize, int size ) const
{
	if(val.is_predefined())
	{
		return 0;
	}
	int ret = 0;
	switch(val.units())
	{
	case css_units_percentage:
		ret = val.calc_percent(size);
		break;
	case css_units_em:
		ret = round_f(val.val() * fontSize);
		val.set_value((float) ret, css_units_px);
		break;
	case css_units_pt:
		ret = m_container->pt_to_px((int) val.val());
		val.set_value((float) ret, css_units_px);
		break;
	case css_units_in:
		ret = m_container->pt_to_px((int) (val.val() * 72));
		val.set_value((float) ret, css_units_px);
		break;
	case css_units_cm:
		ret = m_container->pt_to_px((int) (val.val() * 0.3937 * 72));
		val.set_value((float) ret, css_units_px);
		break;
	case css_units_mm:
		ret = m_container->pt_to_px((int) (val.val() * 0.3937 * 72) / 10);
		val.set_value((float) ret, css_units_px);
		break;
	case css_units_vw:
		ret = (int)((double)m_media.width * (double)val.val() / 100.0);
		break;
	case css_units_vh:
		ret = (int)((double)m_media.height * (double)val.val() / 100.0);
		break;
	case css_units_vmin:
		ret = (int)((double)std::min(m_media.height, m_media.width) * (double)val.val() / 100.0);
		break;
	case css_units_vmax:
		ret = (int)((double)std::max(m_media.height, m_media.width) * (double)val.val() / 100.0);
		break;
	default:
		ret = (int) val.val();
		break;
	}
	return ret;
}

int litehtml::document::width() const
{
	return m_size.width;
}

int litehtml::document::height() const
{
	return m_size.height;
}

void litehtml::document::add_stylesheet( const tchar_t* str, const tchar_t* baseurl, const tchar_t* media )
{
	if(str && str[0])
	{
		m_css.push_back(css_text(str, baseurl, media));
	}
}

bool litehtml::document::on_mouse_over( int x, int y, int client_x, int client_y, position::vector& redraw_boxes )
{
	if(!m_root)
	{
		return false;
	}

	element::ptr over_el = m_root->get_element_by_point(x, y, client_x, client_y);

	bool state_was_changed = false;

	if(over_el != m_over_element)
	{
		if(m_over_element)
		{
			if(m_over_element->on_mouse_leave())
			{
				state_was_changed = true;
			}
		}
		m_over_element = over_el;
	}

	const tchar_t* cursor = 0;

	if(m_over_element)
	{
		if(m_over_element->on_mouse_over())
		{
			state_was_changed = true;
		}
		cursor = m_over_element->get_cursor();
	}

	m_container->set_cursor(cursor ? cursor : _t("auto"));

	if(state_was_changed)
	{
		return m_root->find_styles_changes(redraw_boxes, 0, 0);
	}
	return false;
}

bool litehtml::document::on_mouse_leave( position::vector& redraw_boxes )
{
	if(!m_root)
	{
		return false;
	}
	if(m_over_element)
	{
		if(m_over_element->on_mouse_leave())
		{
			return m_root->find_styles_changes(redraw_boxes, 0, 0);
		}
	}
	return false;
}

bool litehtml::document::on_lbutton_down( int x, int y, int client_x, int client_y, position::vector& redraw_boxes )
{
	if(!m_root)
	{
		return false;
	}

	element::ptr over_el = m_root->get_element_by_point(x, y, client_x, client_y);

	bool state_was_changed = false;

	if(over_el != m_over_element)
	{
		if(m_over_element)
		{
			if(m_over_element->on_mouse_leave())
			{
				state_was_changed = true;
			}
		}
		m_over_element = over_el;
		if(m_over_element)
		{
			if(m_over_element->on_mouse_over())
			{
				state_was_changed = true;
			}
		}
	}

	const tchar_t* cursor = 0;

	if(m_over_element)
	{
		if(m_over_element->on_lbutton_down())
		{
			state_was_changed = true;
		}
		cursor = m_over_element->get_cursor();
	}

	m_container->set_cursor(cursor ? cursor : _t("auto"));

	if(state_was_changed)
	{
		return m_root->find_styles_changes(redraw_boxes, 0, 0);
	}

	return false;
}

bool litehtml::document::on_lbutton_up( int x, int y, int client_x, int client_y, position::vector& redraw_boxes )
{
	if(!m_root)
	{
		return false;
	}
	if(m_over_element)
	{
		if(m_over_element->on_lbutton_up())
		{
			return m_root->find_styles_changes(redraw_boxes, 0, 0);
		}
	}
	return false;
}

litehtml::element::ptr litehtml::document::create_element(const tchar_t* tag_name, const string_map& attributes)
{
	element::ptr newTag = NULL;
	if(m_container)
	{
		newTag = m_container->create_element(tag_name, attributes, this);
	}
	if(!newTag)
	{
		if(!t_strcmp(tag_name, _t("br")))
		{
			newTag = new litehtml::el_break(this);
		} else if(!t_strcmp(tag_name, _t("p")))
		{
			newTag = new litehtml::el_para(this);
		} else if(!t_strcmp(tag_name, _t("img")))
		{
			newTag = new litehtml::el_image(this);
		} else if(!t_strcmp(tag_name, _t("table")))
		{
			newTag = new litehtml::el_table(this);
		} else if(!t_strcmp(tag_name, _t("td")) || !t_strcmp(tag_name, _t("th")))
		{
			newTag = new litehtml::el_td(this);
		} else if(!t_strcmp(tag_name, _t("link")))
		{
			newTag = new litehtml::el_link(this);
		} else if(!t_strcmp(tag_name, _t("title")))
		{
			newTag = new litehtml::el_title(this);
		} else if(!t_strcmp(tag_name, _t("a")))
		{
			newTag = new litehtml::el_anchor(this);
		} else if(!t_strcmp(tag_name, _t("tr")))
		{
			newTag = new litehtml::el_tr(this);
		} else if(!t_strcmp(tag_name, _t("style")))
		{
			newTag = new litehtml::el_style(this);
		} else if(!t_strcmp(tag_name, _t("base")))
		{
			newTag = new litehtml::el_base(this);
		} else if(!t_strcmp(tag_name, _t("body")))
		{
			newTag = new litehtml::el_body(this);
		} else if(!t_strcmp(tag_name, _t("div")))
		{
			newTag = new litehtml::el_div(this);
		} else if(!t_strcmp(tag_name, _t("script")))
		{
			newTag = new litehtml::el_script(this);
		} else if(!t_strcmp(tag_name, _t("font")))
		{
			newTag = new litehtml::el_font(this);
		} else
		{
			newTag = new litehtml::html_tag(this);
		}
	}

	if(newTag)
	{
		newTag->set_tagName(tag_name);
		for (string_map::const_iterator iter = attributes.begin(); iter != attributes.end(); iter++)
		{
			newTag->set_attr(iter->first.c_str(), iter->second.c_str());
		}
	}

	return newTag;
}

void litehtml::document::get_fixed_boxes( position::vector& fixed_boxes )
{
	fixed_boxes = m_fixed_boxes;
}

void litehtml::document::add_fixed_box( const position& pos )
{
	m_fixed_boxes.push_back(pos);
}

bool litehtml::document::media_changed()
{
	if(!m_media_lists.empty())
	{
		container()->get_media_features(m_media);
		if (update_media_lists(m_media))
		{
			m_root->refresh_styles();
			m_root->parse_styles();
			return true;
		}
	}
	return false;
}

bool litehtml::document::update_media_lists(const media_features& features)
{
	bool update_styles = false;
	for(media_query_list::vector::iterator iter = m_media_lists.begin(); iter != m_media_lists.end(); iter++)
	{
		if((*iter)->apply_media_features(features))
		{
			update_styles = true;
		}
	}
	return update_styles;
}

void litehtml::document::add_media_list( media_query_list::ptr list )
{
	if(list)
	{
		if(std::find(m_media_lists.begin(), m_media_lists.end(), list) == m_media_lists.end())
		{
			m_media_lists.push_back(list);
		}
	}
}

void litehtml::document::create_node(GumboNode* node, elements_vector& elements)
{
	switch (node->type)
	{
	case GUMBO_NODE_ELEMENT:
		{
			string_map attrs;
			GumboAttribute* attr;
			for (unsigned int i = 0; i < node->v.element.attributes.length; i++)
			{
				attr = (GumboAttribute*)node->v.element.attributes.data[i];
				attrs[tstring(litehtml_from_utf8(attr->name))] = litehtml_from_utf8(attr->value);
			}


			element::ptr ret;
			const char* tag = gumbo_normalized_tagname(node->v.element.tag);
			if (tag[0])
			{
				ret = create_element(litehtml_from_utf8(tag), attrs);
			}
			else
			{
				if (node->v.element.original_tag.data && node->v.element.original_tag.length)
				{
					gumbo_tag_from_original_text( & node->v.element.original_tag );
					std::string strA;
					strA.append(node->v.element.original_tag.data, node->v.element.original_tag.length);
					ret = create_element(litehtml_from_utf8(strA.c_str()), attrs);
				}
			}
			if (ret)
			{
				elements_vector child;
				for (unsigned int i = 0; i < node->v.element.children.length; i++)
				{
					child.clear();
					create_node(static_cast<GumboNode*> (node->v.element.children.data[i]), child);
					std::for_each(child.begin(), child.end(),
						[&ret](element::ptr& el)
						{
							ret->appendChild(el);
						}
					);
				}
				elements.push_back(ret);
			}
		}
		break;
	case GUMBO_NODE_TEXT:
		{
			std::wstring str;
			std::wstring str_in = (const wchar_t*) (utf8_to_wchar(node->v.text.text));
			ucode_t c;
			for (size_t i = 0; i < str_in.length(); i++)
			{
				c = (ucode_t) str_in[i];
				if (c <= ' ' && (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f'))
				{
					if (!str.empty())
					{
						elements.push_back(new el_text(litehtml_from_wchar(str.c_str()), this));
						str.clear();
					}
					str += c;
					elements.push_back(new el_space(litehtml_from_wchar(str.c_str()), this));
					str.clear();
				}
				// CJK character range
				else if (c >= 0x4E00 && c <= 0x9FCC)
				{
					if (!str.empty())
					{
						elements.push_back(new el_text(litehtml_from_wchar(str.c_str()), this));
						str.clear();
					}
					str += c;
					elements.push_back(new el_text(litehtml_from_wchar(str.c_str()), this));
					str.clear();
				}
				else
				{
					str += c;
				}
			}
			if (!str.empty())
			{
				elements.push_back(new el_text(litehtml_from_wchar(str.c_str()), this));
			}
		}
		break;
	case GUMBO_NODE_CDATA:
		{
			element::ptr ret = new el_cdata(this);
			ret->set_data(litehtml_from_utf8(node->v.text.text));
			elements.push_back(ret);
		}
		break;
	case GUMBO_NODE_COMMENT:
		{
			element::ptr ret = new el_comment(this);
			ret->set_data(litehtml_from_utf8(node->v.text.text));
			elements.push_back(ret);
		}
		break;
	case GUMBO_NODE_WHITESPACE:
		{
			tstring str = litehtml_from_utf8(node->v.text.text);
			for (size_t i = 0; i < str.length(); i++)
			{
				elements.push_back(new el_space(str.substr(i, 1).c_str(), this));
			}
		}
		break;
	default:
		break;
	}
}

void litehtml::document::fix_tables_layout()
{
	size_t i = 0;
	while (i < m_tabular_elements.size())
	{
		element::ptr el_ptr = m_tabular_elements[i];

		switch (el_ptr->get_display())
		{
		case display_inline_table:
		case display_table:
			fix_table_children(el_ptr, display_table_row_group, _t("table-row-group"));
			break;
		case display_table_footer_group:
		case display_table_row_group:
		case display_table_header_group:
			fix_table_parent(el_ptr, display_table, _t("table"));
			fix_table_children(el_ptr, display_table_row, _t("table-row"));
			break;
		case display_table_row:
			fix_table_parent(el_ptr, display_table_row_group, _t("table-row-group"));
			fix_table_children(el_ptr, display_table_cell, _t("table-cell"));
			break;
		case display_table_cell:
			fix_table_parent(el_ptr, display_table_row, _t("table-row"));
			break;
		// TODO: make table layout fix for table-caption, table-column etc. elements
		case display_table_caption:
		case display_table_column:
		case display_table_column_group:
		default:
			break;
		}
		i++;
	}
}

void litehtml::document::fix_table_children(element::ptr el_ptr, style_display disp, const tchar_t* disp_str)
{
	elements_vector tmp;
	elements_vector::iterator first_iter = el_ptr->m_children.begin();
	elements_vector::iterator cur_iter = el_ptr->m_children.begin();

	auto flush_elements = [&]()
	{
		element::ptr annon_tag = new html_tag(this);
		style::ptr st = new style;
		st->add_property(_t("display"), disp_str, 0, false);
		annon_tag->add_style(st);
		annon_tag->parent(el_ptr);
		annon_tag->parse_styles();
		std::for_each(tmp.begin(), tmp.end(),
			[&annon_tag](element::ptr& el)
			{
				annon_tag->appendChild(el);
			}
		);
		first_iter = el_ptr->m_children.insert(first_iter, annon_tag);
		cur_iter = first_iter + 1;
		while (cur_iter != el_ptr->m_children.end() && (*cur_iter)->parent() != el_ptr)
		{
			cur_iter = el_ptr->m_children.erase(cur_iter);
		}
		first_iter = cur_iter;
		tmp.clear();
	};

	while (cur_iter != el_ptr->m_children.end())
	{
		if ((*cur_iter)->get_display() != disp)
		{
			if (!(*cur_iter)->is_white_space() || ((*cur_iter)->is_white_space() && !tmp.empty()))
			{
				if (tmp.empty())
				{
					first_iter = cur_iter;
				}
				tmp.push_back((*cur_iter));
			}
			cur_iter++;
		}
		else if (!tmp.empty())
		{
			flush_elements();
		}
		else
		{
			cur_iter++;
		}
	}
	if (!tmp.empty())
	{
		flush_elements();
	}
}

void litehtml::document::fix_table_parent(element::ptr el_ptr, style_display disp, const tchar_t* disp_str)
{
	element::ptr parent = el_ptr->parent();

	if (parent->get_display() != disp)
	{
		elements_vector::iterator this_element = std::find_if(parent->m_children.begin(), parent->m_children.end(),
			[&](element::ptr& el)->bool
			{
				if (el == el_ptr)
				{
					return true;
				}
				return false;
			}
		);
		if (this_element != parent->m_children.end())
		{
			style_display el_disp = el_ptr->get_display();
			elements_vector::iterator first = this_element;
			elements_vector::iterator last = this_element;
			elements_vector::iterator cur = this_element;

			// find first element with same display
			while (true)
			{
				if (cur == parent->m_children.begin()) break;
				cur--;
				if ((*cur)->is_white_space() || (*cur)->get_display() == el_disp)
				{
					first = cur;
				}
				else
				{
					break;
				}
			}

			// find last element with same display
			cur = this_element;
			while (true)
			{
				cur++;
				if (cur == parent->m_children.end()) break;

				if ((*cur)->is_white_space() || (*cur)->get_display() == el_disp)
				{
					last = cur;
				}
				else
				{
					break;
				}
			}

			// extract elements with the same display and wrap them with anonymous object
			element::ptr annon_tag = new html_tag(this);
			style::ptr st = new style;
			st->add_property(_t("display"), disp_str, 0, false);
			annon_tag->add_style(st);
			annon_tag->parent(parent);
			annon_tag->parse_styles();
			std::for_each(first, last + 1,
				[&annon_tag](element::ptr& el)
				{
					annon_tag->appendChild(el);
				}
			);
			first = parent->m_children.erase(first, last + 1);
			parent->m_children.insert(first, annon_tag);
		}
	}
}
