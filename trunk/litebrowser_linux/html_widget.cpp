#include "globals.h"
#include "html_widget.h"


html_widget::html_widget(litehtml::context* html_context)
{
	m_rendered_width	= 0;
	m_html_context 		= html_context;
	m_html 				= NULL;
}

html_widget::~html_widget()
{

}

bool html_widget::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
	Gtk::Allocation allocation = get_allocation();

	litehtml::position pos;

	pos.width 	= allocation.get_width();
	pos.height 	= allocation.get_height();
	pos.x 		= 0;
	pos.y 		= 0;

	cr->rectangle(0, 0, allocation.get_width(), allocation.get_height());
	cr->set_source_rgb(1, 1, 1);
	cr->fill();

	if(m_html)
	{
		m_html->draw((litehtml::uint_ptr) cr->cobj(), 0, 0, &pos);
	}

	return true;
}

void html_widget::get_client_rect(litehtml::position& client)
{
	Gtk::Allocation allocation = get_allocation();
	client.width = allocation.get_width();
	client.height = allocation.get_height();
	client.x = 0;
	client.y = 0;
}


void html_widget::on_anchor_click(const litehtml::tchar_t* url, litehtml::element::ptr el)
{

}

void html_widget::set_cursor(const litehtml::tchar_t* cursor)
{

}

void html_widget::import_css(litehtml::tstring& text, const litehtml::tstring& url, litehtml::tstring& baseurl, const litehtml::string_vector& media)
{
	if(media.empty() || std::find(media.begin(), media.end(), std::string("all")) != media.end() || std::find(media.begin(), media.end(), std::string("screen")) != media.end())
	{
		std::string css_url;
		make_url(url.c_str(), baseurl.c_str(), css_url);
		load_text_file(css_url, text);
		if(!text.empty())
		{
			baseurl = css_url;
		}
	}
}

void html_widget::set_caption(const litehtml::tchar_t* caption)
{
	if(get_parent_window())
	{
		get_parent_window()->set_title(caption);
	}
}

void html_widget::set_base_url(const litehtml::tchar_t* base_url)
{
	if(base_url)
	{
		m_base_url = urljoin(m_url, std::string(base_url));
	} else
	{
		m_base_url = m_url;
	}
}

void html_widget::link(litehtml::document* doc, litehtml::element::ptr el)
{
	const litehtml::tchar_t* rel = el->get_attr("rel");
	if(rel && !strcmp(rel, "stylesheet"))
	{
		const litehtml::tchar_t* media = el->get_attr("media", "screen");
		if(media && (strstr(media, "screen") || strstr(media, "all")))
		{
			const litehtml::tchar_t* href = el->get_attr("href");
			if(href && href[0])
			{
				std::string url;
				make_url(href, NULL, url);
				std::string css;
				load_text_file(url, css);
				if(!css.empty())
				{
					doc->add_stylesheet(css.c_str(), url.c_str());
				}
			}
		}
	}
}

Glib::RefPtr<Gdk::Pixbuf> html_widget::get_image(const litehtml::tchar_t* url, bool redraw_on_ready)
{
	Glib::RefPtr< Gio::InputStream > stream = load_file(url);
	Glib::RefPtr<Gdk::Pixbuf> ptr = Gdk::Pixbuf::create_from_stream(stream);
	return ptr;
}

void html_widget::open_page(const litehtml::tstring& url)
{
	m_url 		= url;
	m_base_url	= url;

	std::string html;
	load_text_file(url, html);
	m_html = litehtml::document::createFromString(html.c_str(), this, m_html_context);
	if(m_html)
	{
		m_rendered_width = get_allocation().get_width();
		m_html->render(m_rendered_width);
		set_size_request(m_html->width(), m_html->height());
	}

    Glib::RefPtr<Gdk::Window> win = get_window();
    if(win)
    {
        win->invalidate(false);
    }

}

void html_widget::make_url(const litehtml::tchar_t* url, const litehtml::tchar_t* basepath, litehtml::tstring& out)
{
	if(!basepath || (basepath && !basepath[0]))
	{
		if(!m_base_url.empty())
		{
			out = urljoin(m_base_url, std::string(url));
		} else
		{
			out = url;
		}
	} else
	{
		out = urljoin(std::string(basepath), std::string(url));
	}
}

void html_widget::get_preferred_width_vfunc(int& minimum_width, int& natural_width) const
{
	minimum_width = 0;
	if(m_html)
	{
		natural_width = m_html->width();
	} else
	{
		natural_width = 0;
	}
}

void html_widget::get_preferred_height_vfunc(int& minimum_height, int& natural_height) const
{
	minimum_height = 0;
	if(m_html)
	{
		natural_height = m_html->height();
	} else
	{
		natural_height = 0;
	}
}

void html_widget::on_size_allocate(Gtk::Allocation& allocation)
{
	Gtk::DrawingArea::on_size_allocate(allocation);
	if(m_html && m_rendered_width != allocation.get_width())
	{
		m_rendered_width = allocation.get_width();
		m_html->render(m_rendered_width);
	}
}

