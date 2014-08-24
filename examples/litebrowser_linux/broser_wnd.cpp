#include "globals.h"
#include "browser_wnd.h"
#include <gdk/gdkkeysyms.h>

browser_window::browser_window(litehtml::context* html_context) : m_html(html_context), m_go_button("_Go", true)
{
	set_title("litehtml");

	add(m_vbox);
	m_vbox.show();

	m_vbox.pack_start(m_hbox, Gtk::PACK_SHRINK);
	m_hbox.show();

	m_hbox.pack_start(m_address_bar, Gtk::PACK_EXPAND_WIDGET);
	m_address_bar.show();
	m_address_bar.set_text("http://www.litehtml.com/");

	m_address_bar.add_events(Gdk::KEY_PRESS_MASK);
	m_address_bar.signal_key_press_event().connect( sigc::mem_fun(*this, &browser_window::on_address_key_press), false );

	m_go_button.signal_clicked().connect( sigc::mem_fun(*this, &browser_window::on_go_clicked) );

	m_hbox.pack_start(m_go_button, Gtk::PACK_SHRINK);
	m_go_button.show();

	m_vbox.pack_start(m_scrolled_wnd, Gtk::PACK_EXPAND_WIDGET);
	m_scrolled_wnd.set_size_request(800, 600);
	m_scrolled_wnd.show();

	m_scrolled_wnd.add(m_html);
	m_html.show();
}

browser_window::~browser_window()
{

}

void browser_window::on_go_clicked()
{
	litehtml::tstring url = m_address_bar.get_text();
	m_html.open_page(url);
}

bool browser_window::on_address_key_press(GdkEventKey* event)
{
	if(event->keyval == GDK_KEY_Return)
	{
		m_address_bar.select_region(0, -1);
		on_go_clicked();
		return true;
	}

	return false;
}
