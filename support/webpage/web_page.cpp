#include <litehtml/url_path.h>
#include <litehtml/url.h>
#include "web_page.h"

void litebrowser::text_file::on_data(void* data, size_t len, size_t /*downloaded*/, size_t /*total*/)
{
	stream.write((const char*) data, (std::streamsize) len);
}

void litebrowser::text_file::on_page_downloaded(u_int32_t http_status,
												u_int32_t err_code,
												const std::string &/*err_text*/)
{
	if(err_code == 0 && (http_status == 200 || http_status == 0))
	{
		data_ready = true;
	} else
	{
		data_ready = false;
	}
	wait_mutex.unlock();
}

void litebrowser::web_page::open(const std::string &url, const std::string &hash)
{
	m_url = url;
	m_base_url = url;
	m_hash = hash;

	auto data = std::make_shared<text_file>();
	auto cb_on_data = [data](auto && PH1, auto && PH2, auto && PH3, auto && PH4) mutable { data->on_data(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2), std::forward<decltype(PH3)>(PH3), std::forward<decltype(PH4)>(PH4)); };
	auto cb_on_finish = std::bind(&web_page::on_page_downloaded, shared_from_this(), data, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
	http_request(m_url, cb_on_data, cb_on_finish);
}

void litebrowser::web_page::get_client_rect(litehtml::position& client) const
{
	m_html_host->get_client_rect(client);
}

void litebrowser::web_page::on_anchor_click(const char* url, const litehtml::element::ptr& /*el*/)
{
	if(url)
	{
		make_url(url, m_base_url.c_str(), m_clicked_url);
	}
}

void litebrowser::web_page::set_cursor(const char* cursor)
{
	if(cursor)
	{
		if(m_cursor != cursor)
		{
			m_cursor = cursor;
			m_html_host->update_cursor();
		}
	}
}

void litebrowser::web_page::import_css(litehtml::string& text, const litehtml::string& url, litehtml::string& baseurl)
{
	std::string css_url;
	make_url(url.c_str(), baseurl.c_str(), css_url);

	auto data = std::make_shared<text_file>();
	auto cb_on_data = std::bind(&text_file::on_data, data, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
	auto cb_on_finish = std::bind(&text_file::on_page_downloaded, data, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	http_request(css_url, cb_on_data, cb_on_finish);
	data->wait();
	text = data->str();
	if(!text.empty())
	{
		baseurl = css_url;
	}
}

void litebrowser::web_page::set_caption(const char* caption)
{
	m_notify->on_set_caption(caption);
}

void litebrowser::web_page::set_base_url(const char* base_url)
{
	if(base_url)
	{
		m_base_url = litehtml::resolve(litehtml::url(m_url), litehtml::url(base_url)).str();
	} else
	{
		m_base_url = m_url;
	}
}

cairo_surface_t* litebrowser::web_page::get_image(const std::string& url)
{
	return m_images.get_image(url);
}

void litebrowser::web_page::show_hash(const litehtml::string& hash)
{
	std::lock_guard<std::recursive_mutex> html_lock(m_html_mutex);
	if(hash.empty() || !m_html)
	{
		m_html_host->scroll_to(0, 0);
	} else
	{
		std::string selector = "#" + hash;
		litehtml::element::ptr el = m_html->root()->select_one(selector);
		if (!el)
		{
			selector = "[name=" + hash + "]";
			el = m_html->root()->select_one(selector);
		}
		if (el)
		{
			litehtml::position pos = el->get_placement();
			m_html_host->scroll_to(0, pos.top());
		}
	}
}

void litebrowser::web_page::make_url(const char* url, const char* basepath, litehtml::string& out)
{
	if(!basepath || !basepath[0])
	{
		if(!m_base_url.empty())
		{
			out = litehtml::resolve(litehtml::url(m_base_url), litehtml::url(url)).str();
		} else
		{
			out = url;
		}
	} else
	{
		out = litehtml::resolve(litehtml::url(basepath), litehtml::url(url)).str();
	}
}

void litebrowser::web_page::on_mouse_over(int x, int y, int client_x, int client_y)
{
	std::lock_guard<std::recursive_mutex> html_lock(m_html_mutex);
	if(m_html)
	{
		litehtml::position::vector redraw_boxes;
		if(m_html->on_mouse_over(x, y, client_x, client_y, redraw_boxes))
		{
			m_html_host->redraw_boxes(redraw_boxes);
		}
	}
}

void litebrowser::web_page::on_lbutton_down(int x, int y, int client_x, int client_y)
{
	std::lock_guard<std::recursive_mutex> html_lock(m_html_mutex);
	if(m_html)
	{
		litehtml::position::vector redraw_boxes;
		if(m_html->on_lbutton_down(x, y, client_x, client_y, redraw_boxes))
		{
			m_html_host->redraw_boxes(redraw_boxes);
		}
	}
}

void litebrowser::web_page::on_lbutton_up(int x, int y, int client_x, int client_y)
{
	if(m_html)
	{
		{
			std::lock_guard<std::recursive_mutex> html_lock(m_html_mutex);

			litehtml::position::vector redraw_boxes;
			m_clicked_url.clear();
			if (m_html->on_lbutton_up(x, y, client_x, client_y, redraw_boxes))
			{
				m_html_host->redraw_boxes(redraw_boxes);
			}
		}
		if(!m_clicked_url.empty())
		{
			m_html_host->open_url(m_clicked_url);
		}
	}
}

void litebrowser::web_page::on_page_downloaded(std::shared_ptr<text_file> data,
											   u_int32_t /*http_status*/,
											   u_int32_t err_code,
											   const std::string &err_text, const std::string& url)
{
	if(err_code == 0)
	{
		m_url = url;
		std::lock_guard<std::recursive_mutex> html_lock(m_html_mutex);
		data->set_ready();
		m_html_source = data->str();
	} else
	{
		std::lock_guard<std::recursive_mutex> html_lock(m_html_mutex);
		std::stringstream ss;
		ss << "<h1>Impossible to load page</h1>" << std::endl;
		ss << "<p>Error #" << err_code << ": " << err_text << "</p>" << std::endl;
		m_html_source = ss.str();
	}

	m_html = litehtml::document::createFromString(m_html_source, this);
	if (m_html)
	{
		std::lock_guard<std::recursive_mutex> html_lock(m_html_mutex);
		int render_width = m_html_host->get_render_width();
		m_html->render(render_width);
	}
	m_notify->on_page_loaded(id());
}

void litebrowser::web_page::on_image_downloaded(std::shared_ptr<image_file> data,
												u_int32_t http_status,
												u_int32_t err_code,
												const std::string &/*err_text*/,
												const std::string& /*url*/)
{
	data->close();
	if(!data->path().empty() && !err_code && (http_status == 200 || http_status == 0))
	{
		auto ptr = m_html_host->load_image(data->path());
		if(ptr)
		{
			m_images.add_image(data->url(), ptr);
			if(data->redraw_only())
			{
				m_notify->redraw();
			} else
			{
				m_notify->render();
			}
		}
	}
	unlink(data->path().c_str());
}

void litebrowser::web_page::load_image(const char *src, const char *baseurl, bool redraw_on_ready)
{
	std::string url;
	make_url(src, baseurl, url);

	if(m_images.reserve(url))
	{
		auto data = std::make_shared<image_file>(url, redraw_on_ready);
		auto cb_on_data = std::bind(&image_file::on_data, data, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
		auto cb_on_finish = std::bind(&web_page::on_image_downloaded, this, data, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
		http_request(url, cb_on_data, cb_on_finish);
	}
}

void litebrowser::web_page::http_request(const std::string &url,
										 const std::function<void(void *, size_t, size_t, size_t)> &cb_on_data,
										 const std::function<void(u_int32_t, u_int32_t, const std::string &, const std::string &)> &cb_on_finish)
{
	m_requests_pool.enqueue(url, cb_on_data, cb_on_finish);
}

void litebrowser::web_page::on_pool_update_state()
{
	m_notify->update_state();
}

double litebrowser::web_page::get_screen_dpi() const
{
	return m_html_host->get_dpi();
}

int litebrowser::web_page::get_screen_width() const
{
    return m_html_host->get_screen_width();
}

int litebrowser::web_page::get_screen_height() const
{
    return m_html_host->get_screen_height();
}

void litebrowser::web_page::on_mouse_event(const litehtml::element::ptr& /*el*/, litehtml::mouse_event /*event*/)
{

}

//////////////////////////////////////////////////////////

litebrowser::image_file::image_file(std::string url, bool redraw_on_ready) :
			m_url(std::move(url)),
			m_redraw_on_ready(redraw_on_ready)
{
}

void litebrowser::image_file::on_data(void *data, size_t len, size_t /*downloaded*/, size_t /*total*/)
{
	if(m_fd < 0)
	{
		char nameBuff[] = "/tmp/litebrowser-XXXXXX";
		m_fd = mkstemp(nameBuff);
		if(m_fd >= 0)
		{
			m_path = nameBuff;
		}
	}
	if(m_fd >= 0)
	{
		write(m_fd, data, len);
	}
}
