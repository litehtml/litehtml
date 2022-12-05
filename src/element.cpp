#include "html.h"
#include "element.h"
#include "document.h"
#include "render_item.h"
#include "el_before_after.h"

#define LITEHTML_EMPTY_FUNC			{}
#define LITEHTML_RETURN_FUNC(ret)	{return ret;}

litehtml::element::element(const document::ptr& doc) : m_doc(doc)
{
}

litehtml::position litehtml::element::get_placement() const
{
	position pos;
	bool is_first = true;
	for(const auto& ri_el : m_renders)
	{
		auto ri = ri_el.lock();
		if(ri)
		{
			position ri_pos = ri_el.lock()->get_placement();
			if(is_first)
			{
				is_first = false;
				pos = ri_pos;
			} else
			{
				if(pos.x < ri_pos.x)
				{
					pos.x = ri_pos.x;
				}
				if(pos.y < ri_pos.y)
				{
					pos.y = ri_pos.y;
				}
			}
		}
	}
	return pos;
}

bool litehtml::element::is_inline_box() const
{
	if(	css().get_display() == display_inline ||
		   css().get_display() == display_inline_table ||
		   css().get_display() == display_inline_block ||
		   css().get_display() == display_inline_text ||
		   css().get_display() == display_inline_flex)
	{
		return true;
	}
	return false;
}

bool litehtml::element::is_ancestor(const ptr &el) const
{
	element::ptr el_parent = parent();
	while(el_parent && el_parent != el)
	{
		el_parent = el_parent->parent();
	}
	if(el_parent)
	{
		return true;
	}
	return false;
}

bool litehtml::element::is_table_skip() const
{
	return is_space() || is_comment() || css().get_display() == display_none;
}

litehtml::string litehtml::element::dump_get_name()
{
	return "element";
}

std::vector<std::tuple<litehtml::string, litehtml::string>> litehtml::element::dump_get_attrs()
{
	return m_css.dump_get_attrs();
}

void litehtml::element::dump(litehtml::dumper& cout)
{
	cout.begin_node(dump_get_name());

	auto attrs = dump_get_attrs();
	if(!attrs.empty())
	{
		cout.begin_attrs_group("attributes");
		for (const auto &attr: attrs)
		{
			cout.add_attr(std::get<0>(attr), std::get<1>(attr));
		}
		cout.end_attrs_group();
	}

	if(!m_children.empty())
	{
		cout.begin_attrs_group("children");
		for (const auto &el: m_children)
		{
			el->dump(cout);
		}
		cout.end_attrs_group();
	}

	cout.end_node();
}

std::shared_ptr<litehtml::render_item> litehtml::element::create_render_item(const std::shared_ptr<render_item>& parent_ri)
{
	std::shared_ptr<litehtml::render_item> ret;

	if(css().get_display() == display_table_column ||
	   css().get_display() == display_table_column_group ||
	   css().get_display() == display_table_footer_group ||
	   css().get_display() == display_table_header_group ||
	   css().get_display() == display_table_row ||
	   css().get_display() == display_table_row_group)
	{
		ret = std::make_shared<render_item_table_part>(shared_from_this());
	} else if(css().get_display() == display_block ||
				css().get_display() == display_table_cell ||
				css().get_display() == display_table_caption ||
				css().get_display() == display_list_item ||
				css().get_display() == display_inline_block)
	{
		ret = std::make_shared<render_item_block>(shared_from_this());
	} else if(css().get_display() == display_table || css().get_display() == display_inline_table)
	{
		ret = std::make_shared<render_item_table>(shared_from_this());
	} else if(css().get_display() == display_inline || css().get_display() == display_inline_text)
	{
		ret = std::make_shared<render_item_inline>(shared_from_this());
	} else if(css().get_display() == display_flex || css().get_display() == display_inline_flex)
	{
		ret = std::make_shared<render_item_flex>(shared_from_this());
	}
	if(ret)
	{
		if (css().get_display() == display_table ||
			css().get_display() == display_inline_table ||
			css().get_display() == display_table_caption ||
			css().get_display() == display_table_cell ||
			css().get_display() == display_table_column ||
			css().get_display() == display_table_column_group ||
			css().get_display() == display_table_footer_group ||
			css().get_display() == display_table_header_group ||
			css().get_display() == display_table_row ||
			css().get_display() == display_table_row_group)
		{
			get_document()->add_tabular(ret);
		}

		ret->parent(parent_ri);
		for(const auto& el : m_children)
		{
			auto ri = el->create_render_item(ret);
			if(ri)
			{
				ret->add_child(ri);
			}
		}
	}
	return ret;
}

bool litehtml::element::requires_styles_update()
{
	for (const auto& used_style : m_used_styles)
	{
		if(used_style->m_selector->is_media_valid())
		{
			int res = select(*(used_style->m_selector), true);
			if( (res == select_no_match && used_style->m_used) || (res == select_match && !used_style->m_used) )
			{
				return true;
			}
		}
	}
	return false;
}

void litehtml::element::add_render(const std::shared_ptr<render_item>& ri)
{
	m_renders.push_back(ri);
}

bool litehtml::element::find_styles_changes( position::vector& redraw_boxes)
{
	if(css().get_display() == display_inline_text)
	{
		return false;
	}

	bool ret = false;

	if(requires_styles_update())
	{
		auto fetch_boxes = [&](const std::shared_ptr<element>& el)
			{
				for(const auto& weak_ri : el->m_renders)
				{
					auto ri = weak_ri.lock();
					if(ri)
					{
						position::vector boxes;
						ri->get_rendering_boxes(boxes);
						for (auto &box: boxes)
						{
							redraw_boxes.push_back(box);
						}
					}
				}
			};
		fetch_boxes(shared_from_this());
		for (auto& el : m_children)
		{
			fetch_boxes(el);
		}

		refresh_styles();
		compute_styles();
		ret = true;
	}
	for (auto& el : m_children)
	{
		if(el->find_styles_changes(redraw_boxes))
		{
			ret = true;
		}
	}
	return ret;
}

litehtml::element::ptr litehtml::element::_add_before_after(int type, const style& style)
{
	if(style.get_property(_content_).m_type != prop_type_invalid)
	{
		element::ptr el;
		if(type == 0)
		{
			el = std::make_shared<el_before>(get_document());
			m_children.insert(m_children.begin(), el);
		} else
		{
			el = std::make_shared<el_after>(get_document());
			m_children.insert(m_children.end(), el);
		}
		el->parent(shared_from_this());
		return el;
	}
	return nullptr;
}


const litehtml::background* litehtml::element::get_background(bool own_only)		LITEHTML_RETURN_FUNC(nullptr)
void litehtml::element::add_style( const style& style)	        					LITEHTML_EMPTY_FUNC
void litehtml::element::select_all(const css_selector& selector, litehtml::elements_vector& res)	LITEHTML_EMPTY_FUNC
litehtml::elements_vector litehtml::element::select_all(const litehtml::css_selector& selector)	 LITEHTML_RETURN_FUNC(litehtml::elements_vector())
litehtml::elements_vector litehtml::element::select_all(const litehtml::string& selector)			 LITEHTML_RETURN_FUNC(litehtml::elements_vector())
litehtml::element::ptr litehtml::element::select_one( const css_selector& selector ) LITEHTML_RETURN_FUNC(nullptr)
litehtml::element::ptr litehtml::element::select_one( const string& selector )		LITEHTML_RETURN_FUNC(nullptr)
litehtml::element::ptr litehtml::element::find_adjacent_sibling(const element::ptr& el, const css_selector& selector, bool apply_pseudo /*= true*/, bool* is_pseudo /*= 0*/) LITEHTML_RETURN_FUNC(nullptr)
litehtml::element::ptr litehtml::element::find_sibling(const element::ptr& el, const css_selector& selector, bool apply_pseudo /*= true*/, bool* is_pseudo /*= 0*/) LITEHTML_RETURN_FUNC(nullptr)
bool litehtml::element::is_nth_last_child(const element::ptr& el, int num, int off, bool of_type) const		LITEHTML_RETURN_FUNC(false)
bool litehtml::element::is_nth_child(const element::ptr&, int num, int off, bool of_type) const		LITEHTML_RETURN_FUNC(false)
bool litehtml::element::is_only_child(const element::ptr& el, bool of_type)	 const	LITEHTML_RETURN_FUNC(false)
litehtml::element::ptr litehtml::element::get_child( int idx ) const				LITEHTML_RETURN_FUNC(nullptr)
size_t litehtml::element::get_children_count() const								LITEHTML_RETURN_FUNC(0)
void litehtml::element::update_floats(int dy, const ptr &parent)					LITEHTML_EMPTY_FUNC
bool litehtml::element::is_floats_holder() const									LITEHTML_RETURN_FUNC(false)
void litehtml::element::get_content_size( size& sz, int max_width )					LITEHTML_EMPTY_FUNC
bool litehtml::element::appendChild(const ptr &el)									LITEHTML_RETURN_FUNC(false)
bool litehtml::element::removeChild(const ptr &el)									LITEHTML_RETURN_FUNC(false)
void litehtml::element::clearRecursive()											LITEHTML_EMPTY_FUNC
litehtml::string_id litehtml::element::id() const									LITEHTML_RETURN_FUNC(empty_id)
litehtml::string_id litehtml::element::tag() const									LITEHTML_RETURN_FUNC(empty_id)
const char* litehtml::element::get_tagName() const									LITEHTML_RETURN_FUNC("")
void litehtml::element::set_tagName( const char* tag )								LITEHTML_EMPTY_FUNC
void litehtml::element::set_data( const char* data )								LITEHTML_EMPTY_FUNC
void litehtml::element::set_attr( const char* name, const char* val )				LITEHTML_EMPTY_FUNC
void litehtml::element::apply_stylesheet( const litehtml::css& stylesheet )			LITEHTML_EMPTY_FUNC
void litehtml::element::refresh_styles()											LITEHTML_EMPTY_FUNC
void litehtml::element::on_click()													LITEHTML_EMPTY_FUNC
void litehtml::element::compute_styles( bool recursive )							LITEHTML_EMPTY_FUNC
const char* litehtml::element::get_attr( const char* name, const char* def /*= 0*/ ) const LITEHTML_RETURN_FUNC(def)
bool litehtml::element::is_white_space() const										LITEHTML_RETURN_FUNC(false)
bool litehtml::element::is_space() const											LITEHTML_RETURN_FUNC(false)
bool litehtml::element::is_comment() const											LITEHTML_RETURN_FUNC(false)
bool litehtml::element::is_body() const												LITEHTML_RETURN_FUNC(false)
bool litehtml::element::is_break() const											LITEHTML_RETURN_FUNC(false)
bool litehtml::element::is_text() const												LITEHTML_RETURN_FUNC(false)

bool litehtml::element::on_mouse_over()												LITEHTML_RETURN_FUNC(false)
bool litehtml::element::on_mouse_leave()											LITEHTML_RETURN_FUNC(false)
bool litehtml::element::on_lbutton_down()											LITEHTML_RETURN_FUNC(false)
bool litehtml::element::on_lbutton_up()												LITEHTML_RETURN_FUNC(false)
bool litehtml::element::set_pseudo_class( string_id cls, bool add )					LITEHTML_RETURN_FUNC(false)
bool litehtml::element::set_class( const char* pclass, bool add )					LITEHTML_RETURN_FUNC(false)
bool litehtml::element::is_replaced() const											LITEHTML_RETURN_FUNC(false)
void litehtml::element::draw(uint_ptr hdc, int x, int y, const position *clip, const std::shared_ptr<render_item> &ri) LITEHTML_EMPTY_FUNC
void litehtml::element::draw_background(uint_ptr hdc, int x, int y, const position *clip, const std::shared_ptr<render_item> &ri) LITEHTML_EMPTY_FUNC
int					    litehtml::element::get_enum_property(string_id name, bool inherited, int defval, uint_ptr css_properties_member_offset) const LITEHTML_RETURN_FUNC(0)
litehtml::css_length	litehtml::element::get_length_property(string_id name, bool inherited, css_length defval, uint_ptr css_properties_member_offset) const LITEHTML_RETURN_FUNC(0)
litehtml::web_color		litehtml::element::get_color_property(string_id name, bool inherited, web_color defval, uint_ptr css_properties_member_offset) const LITEHTML_RETURN_FUNC(web_color())
litehtml::string		litehtml::element::get_string_property(string_id name, bool inherited, const string& defval, uint_ptr css_properties_member_offset) const LITEHTML_RETURN_FUNC("")
float				    litehtml::element::get_number_property(string_id name, bool inherited, float defval, uint_ptr css_properties_member_offset) const LITEHTML_RETURN_FUNC(0)
litehtml::string		litehtml::element::get_custom_property(string_id name, const string& defval) const LITEHTML_RETURN_FUNC("")
void litehtml::element::get_text( string& text )									LITEHTML_EMPTY_FUNC
void litehtml::element::parse_attributes()											LITEHTML_EMPTY_FUNC
int litehtml::element::select(const string& selector)								LITEHTML_RETURN_FUNC(select_no_match)
int litehtml::element::select(const css_selector& selector, bool apply_pseudo)		LITEHTML_RETURN_FUNC(select_no_match)
int litehtml::element::select( const css_element_selector& selector, bool apply_pseudo /*= true*/ )	LITEHTML_RETURN_FUNC(select_no_match)
litehtml::element::ptr litehtml::element::find_ancestor(const css_selector& selector, bool apply_pseudo, bool* is_pseudo)	LITEHTML_RETURN_FUNC(nullptr)
