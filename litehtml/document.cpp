#include "html.h"
#include "document.h"
#include "elements.h"
#include "tokenizer.h"
#include "stylesheet.h"
#include "el_table.h"
#include "el_td.h"
#include "el_link.h"
#include "el_title.h"

const wchar_t* g_empty_tags[] =
{
	L"base",
	L"meta",
	L"link",
	L"br",
	L"hr",
	L"input",
	L"button",
	L"img",
	0
};

litehtml::document::document(litehtml::document_container* objContainer)
{
	m_root		= 0;
	m_container	= objContainer;
	m_font_name	= L"Times New Roman";
	m_font_size	= 16;
}

litehtml::document::~document()
{
	if(m_root)
	{
		m_root->release();
		m_root = 0;
	}
}

litehtml::document::ptr litehtml::document::createFromString( const wchar_t* str, litehtml::document_container* objPainter, const wchar_t* stylesheet, const wchar_t* cssbaseurl )
{
	litehtml::document::ptr doc = new litehtml::document(objPainter);
	str_istream si(str);
	litehtml::scanner sc(si);

	if(stylesheet)
	{
		parse_stylesheet(stylesheet, doc->m_styles, cssbaseurl);
	}

	element::ptr parent = NULL;

	int t = 0;
	while((t = sc.get_token()) != litehtml::scanner::TT_EOF)
	{
		if(	t == litehtml::scanner::TT_TAG_START || 
			t == litehtml::scanner::TT_WORD || 
			t == litehtml::scanner::TT_SPACE ||
			t == litehtml::scanner::TT_TAG_END)
		{
			if(!parent)
			{
				parent = doc->add_root();
			} else
			{
				for(int i=0; g_empty_tags[i]; i++)
				{
					if(!_wcsicmp(parent->get_tagName(), g_empty_tags[i]))
					{
						element::ptr newTag = parent->parentElement();
						parent = newTag;
						break;
					}
				}
			}
		}

		switch(t)
		{
		case litehtml::scanner::TT_ERROR:
			break;
		case litehtml::scanner::TT_TAG_START:
			{
				// add "body" element into "html" if current tag is not "head" or "body"
				if(!parent->parentElement() && _wcsicmp(sc.get_tag_name(), L"html") && _wcsicmp(sc.get_tag_name(), L"head") && _wcsicmp(sc.get_tag_name(), L"body") )
				{
					element::ptr newTag = doc->add_body();
					parent = newTag;
				}

				element::ptr newTag = NULL;
				if(!_wcsicmp(sc.get_tag_name(), L"html"))
				{
					newTag = NULL;
				} else if(!_wcsicmp(sc.get_tag_name(), L"p"))
				{
					newTag = new litehtml::el_para(doc);
				} else if(!_wcsicmp(sc.get_tag_name(), L"img"))
				{
					newTag = new litehtml::el_image(doc);
				} else if(!_wcsicmp(sc.get_tag_name(), L"html"))
				{
					newTag = NULL;
				} else if(!_wcsicmp(sc.get_tag_name(), L"table"))
				{
					newTag = new litehtml::el_table(doc);
				} else if(!_wcsicmp(sc.get_tag_name(), L"td"))
				{
					newTag = new litehtml::el_td(doc);
				} else if(!_wcsicmp(sc.get_tag_name(), L"link"))
				{
					newTag = new litehtml::el_link(doc);
				} else if(!_wcsicmp(sc.get_tag_name(), L"title"))
				{
					newTag = new litehtml::el_title(doc);
				} else
				{
					newTag = new litehtml::element(doc);
				}

				if(newTag)
				{
					newTag->set_tagName(sc.get_tag_name());
					if(parent->appendChild(newTag))
					{
						parent = newTag;
					}
				}
			}
			break;
		case litehtml::scanner::TT_TAG_END:
			if(!_wcsicmp(parent->get_tagName(), sc.get_tag_name()))
			{
				litehtml::element::ptr newTag = parent->parentElement();
				parent = newTag;
			}
			break;
		case litehtml::scanner::TT_ATTR:
			parent->set_attr(sc.get_attr_name(), sc.get_value());
			break;
		case litehtml::scanner::TT_WORD: 
			{
				element::ptr el = new litehtml::el_text(sc.get_value(), doc);
				parent->appendChild(el);
			}
			break;
		case litehtml::scanner::TT_SPACE:
			if(parent)
			{
				element::ptr el = new litehtml::el_space(doc);
				parent->appendChild(el);
			}
			break;
		}
	}

	if(doc->m_root)
	{
		doc->m_root->finish();

		for(style_sheet::vector::const_iterator i = master_stylesheet.begin(); i != master_stylesheet.end(); i++)
		{
			doc->m_root->apply_stylesheet(*i);
		}

		for(style_sheet::vector::const_iterator i = doc->m_styles.begin(); i != doc->m_styles.end(); i++)
		{
			doc->m_root->apply_stylesheet(*i);
		}
		doc->m_root->parse_styles();
	}

	return doc;
}

litehtml::uint_ptr litehtml::document::add_font( const wchar_t* name, const wchar_t* size, const wchar_t* weight, const wchar_t* style, const wchar_t* decoration )
{
	uint_ptr ret = 0;

	if(!name || name && !_wcsicmp(name, L"inherit"))
	{
		name = m_font_name.c_str();
	}

	if(!size)
	{
		size = L"medium";
	}

	std::wstring key = name;
	key += L":";
	key += size;
	key += L":";
	key += weight;
	key += L":";
	key += style;
	key += L":";
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
				fw = FW_BOLD;
				break;
			case litehtml::fontWeightBolder:
				fw = FW_SEMIBOLD;
				break;
			case litehtml::fontWeightLighter:
				fw = FW_LIGHT;
				break;
			default:
				fw = FW_NORMAL;
				break;
			}
		} else
		{
			fw = _wtoi(weight);
			if(fw < 100)
			{
				fw = FW_NORMAL;
			}
		}

		unsigned int decor = 0;

		if(decoration)
		{
			std::vector<std::wstring> tokens;
			tokenize(decoration, tokens, L" ");
			for(std::vector<std::wstring>::iterator i = tokens.begin(); i != tokens.end(); i++)
			{
				if(!_wcsicmp(i->c_str(), L"underline"))
				{
					decor |= font_decoration_underline;
				} else if(!_wcsicmp(i->c_str(), L"line-through"))
				{
					decor |= font_decoration_linethrough;
				} else if(!_wcsicmp(i->c_str(), L"overline"))
				{
					decor |= font_decoration_overline;
				}
			}
		}

		ret = m_container->create_font(name, cvt_font_size(size), fw, fs, decor);
		m_fonts[key] = ret;
	}
	return ret;
}

litehtml::uint_ptr litehtml::document::get_font( const wchar_t* name, const wchar_t* size, const wchar_t* weight, const wchar_t* style, const wchar_t* decoration )
{
	if(!name || name && !_wcsicmp(name, L"inherit"))
	{
		name = m_font_name.c_str();
	}

	if(!size)
	{
		size = L"medium";
	}

	std::wstring key = name;
	key += L":";
	key += size;
	key += L":";
	key += weight;
	key += L":";
	key += style;
	key += L":";
	key += decoration;

	fonts_map::iterator el = m_fonts.find(key);

	if(el != m_fonts.end())
	{
		return el->second;
	}
	return add_font(name, size, weight, style, decoration);
}

litehtml::element* litehtml::document::add_root()
{
	if(!m_root)
	{
		m_root = new element(this);
		m_root->addRef();
		m_root->set_tagName(L"html");
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
	el->set_tagName(L"body");
	m_root->appendChild(el);
	return el;
}

void litehtml::document::render( uint_ptr hdc, int max_width )
{
	if(m_root)
	{
		m_root->render(hdc, 0, 0, max_width);
	}
}

void litehtml::document::draw( uint_ptr hdc, int x, int y, position* clip )
{
	if(m_root)
	{
		m_root->draw(hdc, x, y, clip);
	}
}

int litehtml::document::cvt_units( const wchar_t* str, int fontSize, bool* is_percent/*= 0*/ ) const
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

int litehtml::document::cvt_units( css_length& val, int fontSize ) const
{
	if(val.is_predefined() || !val.is_predefined() && val.units() == css_units_percentage)
	{
		return 0;
	}
	int ret = 0;
	switch(val.units())
	{
	case css_units_px:
		ret = (int) val.val();
		break;
	case css_units_em:
		ret = (int) (val.val() * fontSize);
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
	}
	return ret;
}

int litehtml::document::cvt_font_size( const wchar_t* size ) const
{
	int sz = value_index(size, font_size_strings, -1);
	if(sz >= 0)
	{
		switch(sz)
		{
		case fontSize_xx_small:
			sz = m_font_size * 3 / 5;
			break;
		case fontSize_x_small:
			sz = m_font_size * 3 / 4;
			break;
		case fontSize_small:
			sz = m_font_size * 8 / 9;
			break;
		case fontSize_large:
			sz = m_font_size * 6 / 5;
			break;
		case fontSize_x_large:
			sz = m_font_size * 3 / 2;
			break;
		case fontSize_xx_large:
			sz = m_font_size * 2;
			break;
		default:
			sz = m_font_size;
			break;
		}
	} else
	{
		sz = cvt_units(size, m_font_size);
	}
	return sz;
}

int litehtml::document::width() const
{
	return m_root ? m_root->width() : 0;
}

int litehtml::document::height() const
{
	return m_root ? m_root->height() : 0;
}

void litehtml::document::add_stylesheet( const wchar_t* str, const wchar_t* baseurl )
{
	parse_stylesheet(str, m_styles, baseurl);
}
