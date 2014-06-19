#include "html.h"
#include "document.h"
#include "elements.h"
#include "tokenizer.h"
#include "stylesheet.h"
#include "el_table.h"
#include "el_td.h"
#include "el_link.h"
#include "el_title.h"
#include "el_style.h"
#include "el_script.h"
#include "el_comment.h"
#include "el_base.h"
#include "el_anchor.h"
#include "el_break.h"
#include "el_div.h"
#include "el_font.h"
#include "el_tr.h"
#include <math.h>
#include <stdio.h>
#include <algorithm>

const litehtml::tchar_t* g_empty_tags[] =
{
	_t("base"),
	_t("meta"),
	_t("link"),
	_t("br"),
	_t("hr"),
	_t("input"),
	_t("button"),
	_t("img"),
	_t("param"),
	_t("col"),
	0
};

litehtml::tags_parse_data litehtml::document::m_tags_table[] =
{
	{ _t("li"),		_t("ul;ol"),			_t("body")		},
	{ _t("body"),	_t("html"),				_t("html")		},
	{ _t("head"),	_t("html"),				_t("html")		},
	{ _t("td"),		_t("tr"),				_t("table")		},
	{ _t("th"),		_t("tr"),				_t("table")		},
	{ _t("tr"),		_t("tbody;thead;tfoot"),_t("table")		},
	{ _t("tbody"),	_t("table"),			_t("table")		},
	{ _t("thead"),	_t("table"),			_t("table")		},
	{ _t("tfoot"),	_t("table"),			_t("table")		},
	{ 0,	0	},
};

litehtml::document::document(litehtml::document_container* objContainer, litehtml::context* ctx)
{
	m_container	= objContainer;
	m_context	= ctx;
}

litehtml::document::~document()
{
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
	litehtml::document::ptr doc = new litehtml::document(objPainter, ctx);
	str_istream si(str);
	litehtml::scanner sc(si);

	doc->begin_parse();

	int t = 0;
	tstring tmp_str;
	while((t = sc.get_token()) != litehtml::scanner::TT_EOF && !doc->m_parse_stack.empty())
	{
		switch(t)
		{
		case litehtml::scanner::TT_COMMENT_START:
			doc->parse_comment_start();
			break;
		case litehtml::scanner::TT_COMMENT_END:
			doc->parse_comment_end();
			break;
		case litehtml::scanner::TT_DATA:
			doc->parse_data(sc.get_value());
			break;
		case litehtml::scanner::TT_TAG_START:
			{
				tmp_str = sc.get_tag_name();
				litehtml::lcase(tmp_str);
				doc->parse_tag_start(tmp_str.c_str());
			}
			break;
		case litehtml::scanner::TT_TAG_END_EMPTY:
			{
				tmp_str = sc.get_tag_name();
				litehtml::lcase(tmp_str);
				if( value_in_list(tmp_str.c_str(), _t("area;base;br;col;command;embed;hr;img;input;keygen;link;meta;param;source;track;wbr")) )
				{
					doc->parse_tag_end(tmp_str.c_str());
				}
			}
			break;
		case litehtml::scanner::TT_TAG_END:
			{
				tmp_str = sc.get_tag_name();
				litehtml::lcase(tmp_str);
				doc->parse_tag_end(tmp_str.c_str());
			}
			break;
		case litehtml::scanner::TT_ATTR:
			{
				tmp_str = sc.get_attr_name();
				litehtml::lcase(tmp_str);
				doc->parse_attribute(tmp_str.c_str(), sc.get_value());
			}
			break;
		case litehtml::scanner::TT_WORD: 
			doc->parse_word(sc.get_value());
			break;
		case litehtml::scanner::TT_SPACE:
			doc->parse_space(sc.get_value());
			break;
		}
	}

	if(doc->m_root)
	{
		doc->m_root->apply_stylesheet(ctx->master_css());

		doc->m_root->parse_attributes();

		for(css_text::vector::iterator css = doc->m_css.begin(); css != doc->m_css.end(); css++)
		{
			doc->m_styles.parse_stylesheet(css->text.c_str(), css->baseurl.c_str(), doc->container());
		}
		doc->m_styles.sort_selectors();

		doc->m_root->apply_stylesheet(doc->m_styles);

		if(user_styles)
		{
			doc->m_root->apply_stylesheet(*user_styles);
		}

		doc->m_root->parse_styles();
	}

	return doc;
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
	t_snprintf(strSize, 20, _t("%d"), size);

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
			tokenize(decoration, tokens, _t(" "));
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
	t_snprintf(strSize, 20, _t("%d"), size);

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

litehtml::element* litehtml::document::add_root()
{
	if(!m_root)
	{
		m_root = new html_tag(this);
		m_root->addRef();
		m_root->set_tagName(_t("html"));
	}
	return m_root;
}

litehtml::element* litehtml::document::add_body()
{
	if(!m_root)
	{
		add_root();
	}
	element* el = new el_body(this);
	el->set_tagName(_t("body"));
	m_root->appendChild(el);
	return el;
}

int litehtml::document::render( int max_width )
{
	int ret = 0;
	if(m_root)
	{
		ret = m_root->render(0, 0, max_width);
		if(m_root->fetch_positioned())
		{
			m_root->render_absolutes();
		}
		m_size.width	= 0;
		m_size.height	= 0;
		m_root->calc_document_size(m_size);
	}
	return ret;
}

void litehtml::document::draw( uint_ptr hdc, int x, int y, const position* clip )
{
	if(m_root)
	{
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

void litehtml::document::add_stylesheet( const tchar_t* str, const tchar_t* baseurl )
{
	if(str && str[0])
	{
		m_css.push_back(css_text(str, baseurl));
	}
}

bool litehtml::document::on_mouse_over( int x, int y, position::vector& redraw_boxes )
{
	if(!m_root)
	{
		return false;
	}
	bool res = m_root->on_mouse_over(x, y);
	const tchar_t* cursor = m_root->get_cursor();
	m_container->set_cursor(cursor ? cursor : _t("auto"));
	if(res)
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
	if(m_root->on_mouse_leave())
	{
		return m_root->find_styles_changes(redraw_boxes, 0, 0);
	}
	return false;
}

bool litehtml::document::on_lbutton_down( int x, int y, position::vector& redraw_boxes )
{
	if(!m_root)
	{
		return false;
	}
	if(m_root->on_lbutton_down(x, y))
	{
		return m_root->find_styles_changes(redraw_boxes, 0, 0);
	}
	return false;
}

bool litehtml::document::on_lbutton_up( int x, int y, position::vector& redraw_boxes )
{
	if(!m_root)
	{
		return false;
	}
	bool ret_val = false;
	if(m_root->on_lbutton_up(x, y))
	{
		ret_val = m_root->find_styles_changes(redraw_boxes, 0, 0);
	}
	return ret_val;
}

litehtml::element::ptr litehtml::document::create_element( const tchar_t* tag_name )
{
	element::ptr newTag = NULL;
	if(m_container)
	{
		newTag = m_container->create_element(tag_name);
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
	}

	return newTag;
}

void litehtml::document::parse_tag_start( const tchar_t* tag_name )
{
	parse_pop_empty_element();

	// We add the html(root) element before parsing
	if(!t_strcmp(tag_name, _t("html")))
	{
		return;
	}

	element::ptr el = create_element(tag_name);
	if(el)
	{
		if(!t_strcmp(m_parse_stack.back()->get_tagName(), _t("html")))
		{
			// if last element is root we have to add head or body
			if(!value_in_list(tag_name, _t("head;body")))
			{
				parse_push_element(create_element(_t("body")));
			}
		}


		for(int i = 0; m_tags_table[i].tag; i++)
		{
			if(!t_strcmp(tag_name, m_tags_table[i].tag))
			{
				parse_pop_to_parent(m_tags_table[i].parents, m_tags_table[i].stop_parent);
				break;
			}
		}

		parse_push_element(el);
	}
}


void litehtml::document::parse_tag_end( const tchar_t* tag_name )
{
	if(!m_parse_stack.empty())
	{
		if(!t_strcmp(m_parse_stack.back()->get_tagName(), tag_name))
		{
			parse_pop_element();
		} else
		{
			const tchar_t* stop_tag = _t("");
			for(int i = 0; m_tags_table[i].tag; i++)
			{
				if(!t_strcmp(tag_name, m_tags_table[i].tag))
				{
					stop_tag = m_tags_table[i].stop_parent;
					break;
				}
			}
			parse_pop_element(tag_name, stop_tag);
		}
	}
}

void litehtml::document::begin_parse()
{
	m_root = create_element(_t("html"));
	m_parse_stack.push_back(m_root);
}

void litehtml::document::parse_push_element( element::ptr el )
{
	if(!m_parse_stack.empty())
	{
		m_parse_stack.back()->appendChild(el);
		m_parse_stack.push_back(el);
	}
}

void litehtml::document::parse_attribute( const tchar_t* attr_name, const tchar_t* attr_value )
{
	if(!m_parse_stack.empty())
	{
		m_parse_stack.back()->set_attr(attr_name, attr_value);
	}
}

void litehtml::document::parse_word( const tchar_t* val )
{
	if(!t_strcmp(m_parse_stack.back()->get_tagName(), _t("html")))
	{
		parse_push_element(create_element(_t("body")));
	}

	parse_pop_empty_element();

	if(!m_parse_stack.empty())
	{
		element::ptr el = new litehtml::el_text(val, this);
		m_parse_stack.back()->appendChild(el);
	}
}

void litehtml::document::parse_space(const tchar_t* val)
{
	parse_pop_empty_element();

	if(!m_parse_stack.empty())
	{
		element::ptr el = new litehtml::el_space(val, this);
		m_parse_stack.back()->appendChild(el);
	}
}

void litehtml::document::parse_comment_start()
{
	parse_pop_empty_element();
	parse_push_element(new litehtml::el_comment(this));
}

void litehtml::document::parse_comment_end()
{
	parse_pop_element();
}

void litehtml::document::parse_data( const tchar_t* val )
{
	if(!m_parse_stack.empty())
	{
		m_parse_stack.back()->set_data(val);
	}
}

bool litehtml::document::parse_pop_element()
{
	if(!m_parse_stack.empty())
	{
		m_parse_stack.pop_back();
		return true;
	}
	return false;
}

bool litehtml::document::parse_pop_element( const tchar_t* tag, const tchar_t* stop_tags )
{
	bool found = false;
	for(elements_vector::reverse_iterator iel = m_parse_stack.rbegin(); iel != m_parse_stack.rend(); iel++)
	{
		if(!t_strcmp( (*iel)->get_tagName(), tag ))
		{
			found = true;
			break;
		}
		if(stop_tags && value_in_list((*iel)->get_tagName(), stop_tags)) break;
	}

	if(!found) return false;

	while(found)
	{
		if(!t_strcmp( m_parse_stack.back()->get_tagName(), tag ))
		{
			found = false;
		}
		parse_pop_element();
	}
	return true;
}

void litehtml::document::parse_pop_empty_element()
{
	if(!m_parse_stack.empty())
	{
		bool is_empty_tag = false;
		for(int i=0; g_empty_tags[i]; i++)
		{
			if(!t_strcmp(m_parse_stack.back()->get_tagName(), g_empty_tags[i]))
			{
				is_empty_tag = true;
				break;
			}
		}
		if(is_empty_tag)
		{
			parse_pop_element();
		}
	}
}

void litehtml::document::parse_pop_to_parent( const tchar_t* parents, const tchar_t* stop_parent )
{
	elements_vector::size_type parent = 0;
	bool found = false;
	string_vector p;
	tokenize(parents, p, _t(";"));

	for(int i = (int) m_parse_stack.size() - 1; i >= 0 && !found; i--)
	{
		if(std::find(p.begin(), p.end(), m_parse_stack[i]->get_tagName()) != p.end())
		{
			found	= true;
			parent	= i;
		}
		if(!t_strcmp(stop_parent, m_parse_stack[i]->get_tagName()))
		{
			break;
		}
	}
	if(found)
	{
		m_parse_stack.erase(m_parse_stack.begin() + parent + 1, m_parse_stack.end());
	} else
	{
		parse_tag_start(p.front().c_str());
	}
}


