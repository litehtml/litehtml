#include "html.h"
#include "el_image.h"
#include "render_image.h"

litehtml::el_image::el_image(const document::ptr& doc) : html_tag(doc)
{
	m_css.set_display(display_inline_block);
}

void litehtml::el_image::get_content_size( size& sz, int /*max_width*/ )
{
	get_document()->container()->get_image_size(m_src.c_str(), nullptr, sz);
}

bool litehtml::el_image::is_replaced() const
{
	return true;
}

void litehtml::el_image::parse_attributes()
{
	m_src = get_attr("src", "");

	const char* attr_height = get_attr("height");
	if(attr_height)
	{
		m_style.add_property(_height_, attr_height);
	}
	const char* attr_width = get_attr("width");
	if(attr_width)
	{
		m_style.add_property(_width_, attr_width);
	}
}

void litehtml::el_image::draw(uint_ptr hdc, int x, int y, const position *clip, const std::shared_ptr<render_item> &ri)
{
	html_tag::draw(hdc, x, y, clip, ri);
	position pos = ri->pos();
	pos.x += x;
	pos.y += y;

	// draw image as background
	if(pos.does_intersect(clip))
	{
		if (pos.width > 0 && pos.height > 0)
		{
			background_layer layer;
			layer.clip_box = pos;
			layer.origin_box = pos;
			layer.border_box = pos;
			layer.border_box += ri->get_paddings();
			layer.border_box += ri->get_borders();
			layer.repeat = background_repeat_no_repeat;
			layer.border_radius = css().get_borders().radius.calc_percents(layer.border_box.width, layer.border_box.height);
			get_document()->container()->draw_image(hdc, layer, m_src, {});
		}
	}
}

void litehtml::el_image::compute_styles(bool recursive)
{
	html_tag::compute_styles(recursive);

	if(!m_src.empty())
	{
		if(!css().get_height().is_predefined() && !css().get_width().is_predefined())
		{
			get_document()->container()->load_image(m_src.c_str(), nullptr, true);
		} else
		{
			get_document()->container()->load_image(m_src.c_str(), nullptr, false);
		}
	}
}

litehtml::string litehtml::el_image::dump_get_name()
{
    return "img src=\"" + m_src + "\"";
}

std::shared_ptr<litehtml::render_item> litehtml::el_image::create_render_item(const std::shared_ptr<render_item>& parent_ri)
{
    auto ret = std::make_shared<render_item_image>(shared_from_this());
    ret->parent(parent_ri);
    return ret;
}
