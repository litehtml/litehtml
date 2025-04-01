#include "html_widget.h"
#include <chrono>

html_widget::html_widget()
{
	add_css_class("litehtml");

	m_notifier = std::make_shared<html_widget_notifier>();
	m_notifier->connect_redraw(sigc::mem_fun(*this, &html_widget::on_redraw));
	m_notifier->connect_render(sigc::mem_fun(*this, &html_widget::render));
	m_notifier->connect_update_state([this]() { m_sig_update_state.emit(get_state()); });
	m_notifier->connect_on_page_loaded(sigc::mem_fun(*this, &html_widget::on_page_loaded));
	m_notifier->connect_on_set_caption(sigc::mem_fun(*this, &html_widget::set_caption));

	set_focusable(true);

	m_vadjustment = Gtk::Adjustment::create(0.0, 0.0, 0.0, 10);
	m_vadjustment->signal_value_changed().connect(sigc::mem_fun(*this, &html_widget::on_vadjustment_changed));
	m_hadjustment = Gtk::Adjustment::create(0.0, 0.0, 0.0, 10);
	m_hadjustment->signal_value_changed().connect(sigc::mem_fun(*this, &html_widget::on_hadjustment_changed));

	m_hadjustment->signal_changed().connect(sigc::mem_fun(*this, &html_widget::on_adjustments_changed));
	m_vadjustment->signal_changed().connect(sigc::mem_fun(*this, &html_widget::on_adjustments_changed));

	m_vscrollbar = Gtk::make_managed<Gtk::Scrollbar>(m_vadjustment, Gtk::Orientation::VERTICAL);
	m_vscrollbar->insert_at_end(*this);
	m_vscrollbar->set_visible(true);
	m_vscrollbar->add_css_class("right");
	m_vscrollbar->add_css_class("overlay-indicator");

	m_hscrollbar = Gtk::make_managed<Gtk::Scrollbar>(m_hadjustment, Gtk::Orientation::HORIZONTAL);
	m_hscrollbar->insert_at_end(*this);
	m_hscrollbar->set_visible(true);
	m_hscrollbar->add_css_class("bottom");
	m_hscrollbar->add_css_class("overlay-indicator");

	auto controller = Gtk::EventControllerScroll::create();
	controller->set_flags(Gtk::EventControllerScroll::Flags::BOTH_AXES);
	controller->signal_scroll().connect(sigc::mem_fun(*this, &html_widget::on_scroll), false);
	add_controller(controller);

	auto click_gesture = Gtk::GestureClick::create();
	click_gesture->set_button(GDK_BUTTON_PRIMARY);
	click_gesture->signal_pressed().connect(sigc::mem_fun(*this, &html_widget::on_button_press_event));
	click_gesture->signal_released().connect(sigc::mem_fun(*this, &html_widget::on_button_release_event));
	add_controller(click_gesture);

	auto move_gesture = Gtk::EventControllerMotion::create();
	move_gesture->signal_motion().connect(sigc::mem_fun(*this, &html_widget::on_mouse_move));
	move_gesture->signal_leave().connect([this]() { restart_scrollbar_timer(); });
	add_controller(move_gesture);

	auto key_controller = Gtk::EventControllerKey::create();
	key_controller->signal_key_pressed().connect(sigc::mem_fun(*this, &html_widget::on_key_pressed), false);
	add_controller(key_controller);
}

html_widget::~html_widget()
{
	m_notifier->disconnect();

	if (!gobj()) return;

	while (Widget* child = get_first_child())
	{
		child->unparent();
	}
}

double html_widget::get_dpi()
{
	// DPI is always 96 (default). Scaling will make things larger.
	return 96.0;
}

int html_widget::get_screen_width()
{
	auto display = Gdk::Display::get_default();
	if (display)
	{
		auto monitors = display->get_monitors();
		if (monitors->get_n_items() > 0)
		{
			auto monitor = monitors->get_typed_object<Gdk::Monitor>(0);
			Gdk::Rectangle rect;
			monitor->get_geometry(rect);
			return rect.get_width();
		}
	}
	return 800;
}

int html_widget::get_screen_height()
{
	auto display = Gdk::Display::get_default();
	if (display)
	{
		auto monitors = display->get_monitors();
		if (monitors->get_n_items() > 0)
		{
			auto monitor = monitors->get_typed_object<Gdk::Monitor>(0);
			Gdk::Rectangle rect;
			monitor->get_geometry(rect);
			return rect.get_height();
		}
	}
	return 800;
}

void html_widget::snapshot_vfunc(const Glib::RefPtr<Gtk::Snapshot>& snapshot)
{
    if (get_allocated_width() <= 0 || get_allocated_height() <= 0)
	{
        return;
    }

	{
		auto allocation = get_allocation();
		auto cr = snapshot->append_cairo(Gdk::Rectangle(0, 0, allocation.get_width(), allocation.get_height()));
		if(m_draw_buffer.get_cairo_surface())
		{
			cr->scale(1.0 / m_draw_buffer.get_scale_factor(), 1.0 / m_draw_buffer.get_scale_factor());
			cairo_set_source_surface(cr->cobj(), m_draw_buffer.get_cairo_surface(), 0, 0);
			cr->paint();
		}
	}

	snapshot_child(*m_vscrollbar, snapshot);
	snapshot_child(*m_hscrollbar, snapshot);
}

void html_widget::get_client_rect(litehtml::position& client) const
{
	client.x		= m_draw_buffer.get_left();
	client.y		= m_draw_buffer.get_top();
	client.width	= m_draw_buffer.get_width();
	client.height	= m_draw_buffer.get_height();
}

void html_widget::set_caption(const std::string& caption)
{
	auto root = get_root();
	if(root)
	{
		auto window = dynamic_cast<Gtk::Window*>(root);
		if (window)
		{
			window->set_title(caption.c_str());
		};
	}
}

cairo_surface_t *html_widget::load_image(const std::string &path)
{
	Glib::RefPtr<Gdk::Pixbuf> ptr;

	try
	{
		ptr = Gdk::Pixbuf::create_from_file(path);
	} catch (...) {	}

	if(!ptr) return nullptr;

	cairo_surface_t* ret = nullptr;

	if(ptr->get_has_alpha())
	{
		ret = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, ptr->get_width(), ptr->get_height());
	} else
	{
		ret = cairo_image_surface_create(CAIRO_FORMAT_RGB24, ptr->get_width(), ptr->get_height());
	}

	Cairo::RefPtr<Cairo::Surface> surface(new Cairo::Surface(ret, false));
	Cairo::RefPtr<Cairo::Context> ctx = Cairo::Context::create(surface);
	Gdk::Cairo::set_source_pixbuf(ctx, ptr, 0.0, 0.0);
	ctx->paint();

	return ret;

}

void html_widget::open_page(const litehtml::string& url, const litehtml::string& hash)
{
	{
		std::lock_guard<std::mutex> lock(m_page_mutex);
		if (m_current_page)
		{
			m_current_page->stop_loading();
		}
		m_next_page = std::make_shared<litebrowser::web_page>(this, m_notifier, 10);
		m_next_page->open(url, hash);
	}
	m_sig_set_address.emit(url);
	m_sig_update_state.emit(get_state());
}

void html_widget::scroll_to(int x, int y)
{
	m_hadjustment->set_value(x);
	m_vadjustment->set_value(y);
}

void html_widget::on_redraw()
{
	m_draw_buffer.redraw(current_page());
	queue_draw();
}

void html_widget::on_button_press_event(int /* n_press */, double x, double y)
{
	if(!has_focus())
	{
		grab_focus();
	}
	auto page = current_page();
	if (page)
	{
		page->on_lbutton_down(	(int) (x + m_draw_buffer.get_left()),
								(int) (y + m_draw_buffer.get_top()),
								(int) x, (int) y);
	}
}

void html_widget::on_button_release_event(int /* n_press */, double x, double y)
{
	auto page = current_page();
	if(page)
	{
		page->on_lbutton_up((int) (x + m_draw_buffer.get_left()),
							(int) (y + m_draw_buffer.get_top()),
							(int) x, (int) y);
	}
}

void html_widget::on_mouse_move(double x, double y)
{
	bool restart_timer = true;
	if(m_hscrollbar->is_visible())
	{
		if(y >= get_allocated_height() - 16)
		{
			m_hscrollbar->add_css_class("hovering");
			restart_timer = false;
		} else
		{
			m_hscrollbar->remove_css_class("hovering");
		}
	}
	if(m_vscrollbar->is_visible())
	{
		if(x >= get_allocated_width() - 16)
		{
			m_vscrollbar->add_css_class("hovering");
			restart_timer = false;
		} else
		{
			m_vscrollbar->remove_css_class("hovering");
		}
	}
	m_hscrollbar->set_opacity(1);
	m_vscrollbar->set_opacity(1);
	if(restart_timer)
	{
		restart_scrollbar_timer();
	} else
	{
		m_scrollbar_timer.disconnect();
	}

	std::shared_ptr<litebrowser::web_page> page = current_page();
	if(page)
	{
		page->on_mouse_over((int) (x + m_draw_buffer.get_left()),
							(int) (y + m_draw_buffer.get_top()),
							(int) x, (int) y);
	}
}

bool html_widget::on_key_pressed(guint keyval, guint /* keycode */, Gdk::ModifierType /* state */)
{
	switch (keyval)
	{
	case GDK_KEY_KP_Page_Down:
	case GDK_KEY_Page_Down:
		m_vadjustment->set_value(m_vadjustment->get_value() + m_vadjustment->get_page_size());
		break;
	case GDK_KEY_KP_Page_Up:
	case GDK_KEY_Page_Up:
		m_vadjustment->set_value(m_vadjustment->get_value() - m_vadjustment->get_page_size());
		break;
	case GDK_KEY_KP_Down:
	case GDK_KEY_Down:
		m_vadjustment->set_value(m_vadjustment->get_value() + m_vadjustment->get_step_increment());
		break;
	case GDK_KEY_KP_Up:
	case GDK_KEY_Up:
		m_vadjustment->set_value(m_vadjustment->get_value() - m_vadjustment->get_step_increment());
		break;
	case GDK_KEY_KP_Home:
	case GDK_KEY_Home:
		m_vadjustment->set_value(0);
		break;
	case GDK_KEY_KP_End:
	case GDK_KEY_End:
		m_vadjustment->set_value(m_vadjustment->get_upper());
		break;
	case GDK_KEY_KP_Left:
	case GDK_KEY_Left:
		m_hadjustment->set_value(m_hadjustment->get_value() - m_hadjustment->get_step_increment());
		break;
	case GDK_KEY_KP_Right:
	case GDK_KEY_Right:
		m_hadjustment->set_value(m_hadjustment->get_value() + m_hadjustment->get_step_increment());
		break;

	default:
		break;
	}

    return false;
}

void html_widget::update_cursor()
{
	std::shared_ptr<litebrowser::web_page> page = current_page();
	if(page)
	{
		if(page->get_cursor() == "auto")
		{
			set_cursor("default");
		} else
		{
			set_cursor(page->get_cursor());
		}
	} else
	{
		set_cursor("default");
	}
}

long html_widget::draw_measure(int number)
{
	std::shared_ptr<litebrowser::web_page> page = current_page();

	if(page)
	{
		litehtml::position view_port;
		get_client_rect(view_port);

		int width = view_port.width;
		int height = view_port.height;

		int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
		auto image = (unsigned char *) g_malloc(stride * height);

		cairo_surface_t *surface = cairo_image_surface_create_for_data(image, CAIRO_FORMAT_ARGB32, width, height,
																	   stride);
		cairo_t *cr = cairo_create(surface);

		litehtml::position pos;
		pos.width = width;
		pos.height = height;
		pos.x = 0;
		pos.y = 0;

		int x = view_port.x;
		int y = view_port.y;

		cairo_rectangle(cr, 0, 0, width, height);
		cairo_set_source_rgb(cr, 1, 1, 1);
		cairo_paint(cr);
		page->draw((litehtml::uint_ptr) cr, -x, -y, &pos);
		cairo_surface_write_to_png(surface, "/tmp/litebrowser.png");

		auto t1 = std::chrono::high_resolution_clock::now();
		for (int i = 0; i < number; i++)
		{
			page->draw((litehtml::uint_ptr) cr, -x, -y, &pos);
		}
		auto t2 = std::chrono::high_resolution_clock::now();

		cairo_destroy(cr);
		cairo_surface_destroy(surface);
		g_free(image);

		return (std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1)).count();
	}
	return 0;
}

long html_widget::render_measure(int number)
{
	std::shared_ptr<litebrowser::web_page> page = current_page();

	if(page)
	{
		auto t1 = std::chrono::high_resolution_clock::now();
		for (int i = 0; i < number; i++)
		{
			page->render(m_draw_buffer.get_width());
		}
		auto t2 = std::chrono::high_resolution_clock::now();
		return (std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1)).count();
	}
	return -1;
}

void html_widget::size_allocate_vfunc(int width, int height, int /* baseline */)
{
	allocate_scrollbars(width, height);
	std::shared_ptr<litebrowser::web_page> page = current_page();
	if(page)
	{
		if(m_rendered_width != width || m_rendered_height != height)
		{
			m_rendered_width = width;
			m_rendered_height = height;
			m_draw_buffer.on_size_allocate(page, width, height);
			page->media_changed();
			page->render(m_rendered_width);
			update_view_port(page);
			m_draw_buffer.redraw(page);
			queue_draw();
		}
	} else
	{
		m_draw_buffer.on_size_allocate(page, width, height);
	}

}

void html_widget::allocate_scrollbars(int width, int height)
{
	m_do_force_redraw_on_adjustment = false;
	m_hadjustment->set_page_size(width);
	m_vadjustment->set_page_size(height);
	if(m_vadjustment->get_value() > m_vadjustment->get_upper() - m_vadjustment->get_page_size())
	{
		m_vadjustment->set_value(m_vadjustment->get_upper() - m_vadjustment->get_page_size());
	}
	if(m_hadjustment->get_value() > m_hadjustment->get_upper() - m_hadjustment->get_page_size())
	{
		m_hadjustment->set_value(m_hadjustment->get_upper() - m_hadjustment->get_page_size());
	}
	m_do_force_redraw_on_adjustment = true;

	int minimum = 0, natural = 0, m_baseline = 0, n_baseline = 0;

	m_vscrollbar->measure(Gtk::Orientation::HORIZONTAL, -1, minimum, natural, m_baseline, n_baseline);
	Gtk::Allocation vscrollbar_allocation(width - natural, 0, natural, height);
	m_vscrollbar->size_allocate(vscrollbar_allocation, -1);

	minimum = 0, natural = 0, m_baseline = 0, n_baseline = 0;
	m_hscrollbar->measure(Gtk::Orientation::VERTICAL, -1, minimum, natural, m_baseline, n_baseline);
	Gtk::Allocation hscrollbar_allocation(0, height - natural, width, natural);
	m_hscrollbar->size_allocate(hscrollbar_allocation, -1);
}

void html_widget::on_vadjustment_changed()
{
	m_draw_buffer.on_scroll(	current_page(),
								(int) m_hadjustment->get_value(),
								(int) m_vadjustment->get_value());

	if(m_do_force_redraw_on_adjustment)
	{
		force_redraw();
	} else
	{
		queue_draw();
	}
}

void html_widget::on_hadjustment_changed()
{
	m_draw_buffer.on_scroll(	current_page(),
								(int) m_hadjustment->get_value(),
								(int) m_vadjustment->get_value());
	if(m_do_force_redraw_on_adjustment)
	{
		force_redraw();
	} else
	{
		queue_draw();
	}
}

void html_widget::on_adjustments_changed()
{
	m_hscrollbar->set_visible(m_hadjustment->get_upper() > 0);
	m_vscrollbar->set_visible(m_vadjustment->get_upper() > 0);
}

bool html_widget::on_scroll(double dx, double dy)
{
	m_vadjustment->set_value(m_vadjustment->get_value() + dy * 60);
	m_hadjustment->set_value(m_hadjustment->get_value() + dx * 60);
	return true;
}

bool html_widget::on_scrollbar_timeout()
{
	m_hscrollbar->remove_css_class("hovering");
	m_vscrollbar->remove_css_class("hovering");
	m_hscrollbar->set_opacity(0);
	m_vscrollbar->set_opacity(0);
	return false;
}

void html_widget::on_realize()
{
	Gtk::Widget::on_realize();

	auto native = get_native();
	if(native)
	{
		auto surface = native->get_surface();
		if(surface)
		{
			surface->property_scale().signal_changed().connect([this]()
			{
				auto native = get_native();
				if(native)
				{
					auto surface = native->get_surface();
					if(surface)
					{
						m_draw_buffer.set_scale_factor(current_page(), surface->get_scale());
						queue_draw();
					}
				}
			});
		}
	}
}

void html_widget::update_view_port(std::shared_ptr<litebrowser::web_page> page)
{
	if(page)
	{
		auto allocation = get_allocation();
		if(allocation.get_width() < page->width())
		{
			m_hadjustment->set_upper(page->width());
		} else
		{
			m_hadjustment->set_upper(0);
		}
		if(allocation.get_height() < page->height())
		{
			m_vadjustment->set_upper(page->height());
		} else
		{
			m_vadjustment->set_upper(0);
		}
	} else
	{
		m_hadjustment->set_upper(0);
		m_vadjustment->set_upper(0);
	}
}

void html_widget::restart_scrollbar_timer()
{
	if (m_scrollbar_timer)
	{
		m_scrollbar_timer.disconnect();
	}

	m_scrollbar_timer = Glib::signal_timeout().connect_seconds(
		sigc::mem_fun(*this, &html_widget::on_scrollbar_timeout), 2);
}

void html_widget::redraw_boxes(const litehtml::position::vector& boxes)
{
	if(boxes.empty()) return;

	Gdk::Rectangle rect(0, 0, 0, 0);
	bool is_first = true;
	for(const auto& pos : boxes)
	{
		if(is_first)
		{
			rect = Gdk::Rectangle(pos.x, pos.y, pos.width, pos.height);
			is_first = false;
		} else
		{
			rect.join(Gdk::Rectangle(pos.x, pos.y, pos.width, pos.height));
		}
	}

	if(!rect.has_zero_area())
	{
		m_draw_buffer.redraw_area(current_page(), rect.get_x(), rect.get_y(), rect.get_width(), rect.get_height());
		queue_draw();
	}
}

int html_widget::get_render_width()
{
	return m_draw_buffer.get_width();
}

void html_widget::on_page_loaded(uint64_t web_page_id)
{
	std::string url;
	{
		std::lock_guard<std::mutex> lock(m_page_mutex);
		if(m_next_page->id() != web_page_id) return;
		m_current_page = m_next_page;
		m_next_page = nullptr;
		url = m_current_page->url();
		update_view_port(m_current_page);
	}
	scroll_to(0, 0);
	on_redraw();
	m_sig_set_address.emit(url);
	m_sig_update_state.emit(get_state());
}

void html_widget::show_hash(const std::string &hash)
{
	std::shared_ptr<litebrowser::web_page> page = current_page();
	if(page)
	{
		page->show_hash(hash);
	}
}

void html_widget::open_url(const std::string &url)
{
	std::string hash;
	std::string s_url = url;

	m_sig_set_address.emit(url);

	std::string::size_type hash_pos = s_url.find_first_of(L'#');
	if(hash_pos != std::wstring::npos)
	{
		hash = s_url.substr(hash_pos + 1);
		s_url.erase(hash_pos);
	}

	bool open_hash_only = false;
	bool reload = false;

	auto current_url = m_history.current();
	hash_pos = current_url.find_first_of(L'#');
	if(hash_pos != std::wstring::npos)
	{
		current_url.erase(hash_pos);
	}

	if(!current_url.empty())
	{
		if(m_history.current() != url)
		{
			if (current_url == s_url)
			{
				open_hash_only = true;
			}
		} else
		{
			reload = true;
		}
	}
	if(!open_hash_only)
	{
		open_page(url, hash);
	} else
	{
		show_hash(hash);
	}
	if(!reload)
	{
		m_history.url_opened(url);
	}
	m_sig_update_state.emit(get_state());
}

void html_widget::render()
{
	std::shared_ptr<litebrowser::web_page> page = current_page();
	if(page)
	{
		page->render(m_draw_buffer.get_width());
		update_view_port(page);
		m_draw_buffer.redraw(page);
		queue_draw();
	}
}

bool html_widget::on_close()
{
	if(m_current_page)
	{
		m_current_page->stop_loading();
	}
	if(m_next_page)
	{
		m_next_page->stop_loading();
	}
	return false;
}

void html_widget::dump(litehtml::dumper &cout)
{
	std::shared_ptr<litebrowser::web_page> page = current_page();
	if(page)
	{
		page->dump(cout);
	}
}

void html_widget::go_forward()
{
	std::string url;
	if(m_history.forward(url))
	{
		open_url(url);
	}
}

void html_widget::go_back()
{
	std::string url;
	if(m_history.back(url))
	{
		open_url(url);
	}
}

uint32_t html_widget::get_state()
{
	uint32_t ret = 0;
	std::string url;
	if(m_history.back(url))
	{
		ret |= page_state_has_back;
	}
	if(m_history.forward(url))
	{
		ret |= page_state_has_forward;
	}
	{
		std::lock_guard<std::mutex> lock(m_page_mutex);
		if(m_next_page)
		{
			ret |= page_state_downloading;
		}
	}
	if(!(ret & page_state_downloading))
	{
		std::shared_ptr<litebrowser::web_page> page = current_page();
		if(page)
		{
			if(page->is_downloading())
			{
				ret |= page_state_downloading;
			}
		}
	}
	return ret;
}

void html_widget::stop_download()
{
	std::lock_guard<std::mutex> lock(m_page_mutex);
	if(m_next_page)
	{
		m_next_page->stop_loading();
	} else if (m_current_page)
	{
		m_current_page->stop_loading();
	}
}

void html_widget::reload()
{
	std::shared_ptr<litebrowser::web_page> page = current_page();
	if(page)
	{
		open_url(page->url());
	}
}

std::string html_widget::get_html_source()
{
	std::lock_guard<std::mutex> lock(m_page_mutex);
	if(m_current_page)
		return m_current_page->get_html_source();
	return {};
}
