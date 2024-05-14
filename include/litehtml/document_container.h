#ifndef LH_DOCUMENT_CONTAINER_H
#define LH_DOCUMENT_CONTAINER_H

#include "os_types.h"
#include "types.h"
#include "web_color.h"
#include "background.h"
#include "borders.h"
#include "element.h"
#include <memory>
#include <functional>

namespace litehtml
{
	struct list_marker
	{
		string			image;
		const char*		baseurl;
		list_style_type	marker_type;
		web_color		color;
		position		pos;
		int				index;
		uint_ptr		font;
	};
	
	enum mouse_event
	{
		mouse_event_enter,
		mouse_event_leave,
	};

	// call back interface to draw text, images and other elements
	class document_container
	{
	public:
		virtual litehtml::uint_ptr	create_font(const char* faceName, int size, int weight, litehtml::font_style italic, unsigned int decoration, litehtml::font_metrics* fm) = 0;
		virtual void				delete_font(litehtml::uint_ptr hFont) = 0;
		virtual int					text_width(const char* text, litehtml::uint_ptr hFont) = 0;
		virtual void				draw_text(litehtml::uint_ptr hdc, const char* text, litehtml::uint_ptr hFont, litehtml::web_color color, const litehtml::position& pos) = 0;
		virtual int					pt_to_px(int pt) const = 0;
		virtual int					get_default_font_size() const = 0;
		virtual const char*			get_default_font_name() const = 0;
		virtual void				draw_list_marker(litehtml::uint_ptr hdc, const litehtml::list_marker& marker) = 0;
		virtual void				load_image(const char* src, const char* baseurl, bool redraw_on_ready) = 0;
		virtual void				get_image_size(const char* src, const char* baseurl, litehtml::size& sz) = 0;
		virtual void				draw_image(litehtml::uint_ptr hdc, const background_layer& layer, const std::string& url, const std::string& base_url) = 0;
		virtual void				draw_solid_fill(litehtml::uint_ptr hdc, const background_layer& layer, const web_color& color) = 0;
		virtual void				draw_linear_gradient(litehtml::uint_ptr hdc, const background_layer& layer, const background_layer::linear_gradient& gradient) = 0;
		virtual void				draw_radial_gradient(litehtml::uint_ptr hdc, const background_layer& layer, const background_layer::radial_gradient& gradient) = 0;
		virtual void				draw_conic_gradient(litehtml::uint_ptr hdc, const background_layer& layer, const background_layer::conic_gradient& gradient) = 0;
		virtual void				draw_borders(litehtml::uint_ptr hdc, const litehtml::borders& borders, const litehtml::position& draw_pos, bool root) = 0;

		virtual	void				set_caption(const char* caption) = 0;
		virtual	void				set_base_url(const char* base_url) = 0;
		virtual void				link(const std::shared_ptr<litehtml::document>& doc, const litehtml::element::ptr& el) = 0;
		virtual void				on_anchor_click(const char* url, const litehtml::element::ptr& el) = 0;
		virtual void				on_mouse_event(const litehtml::element::ptr& el, litehtml::mouse_event event) = 0;
		virtual	void				set_cursor(const char* cursor) = 0;
		virtual	void				transform_text(litehtml::string& text, litehtml::text_transform tt) = 0;
		virtual void				import_css(litehtml::string& text, const litehtml::string& url, litehtml::string& baseurl) = 0;
		virtual void				set_clip(const litehtml::position& pos, const litehtml::border_radiuses& bdr_radius) = 0;
		virtual void				del_clip() = 0;
		virtual void				get_client_rect(litehtml::position& client) const = 0;
		virtual litehtml::element::ptr	create_element( const char* tag_name,
														const litehtml::string_map& attributes,
														const std::shared_ptr<litehtml::document>& doc) = 0;

		virtual void				get_media_features(litehtml::media_features& media) const = 0;
		virtual void				get_language(litehtml::string& language, litehtml::string& culture) const = 0;
		virtual litehtml::string	resolve_color(const litehtml::string& /*color*/) const { return litehtml::string(); }
		virtual void				split_text(const char* text, const std::function<void(const char*)>& on_word, const std::function<void(const char*)>& on_space);

	protected:
		virtual ~document_container() = default;
	};
}

#endif  // LH_DOCUMENT_CONTAINER_H
