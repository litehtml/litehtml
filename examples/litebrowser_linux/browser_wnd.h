#pragma once

#include "html_widget.h"

class browser_window : public Gtk::Window
{
public:
	browser_window(litehtml::context* html_context);
	virtual ~browser_window();

private:
    void on_go_clicked();
    bool on_address_key_press(GdkEventKey* event);

protected:
	html_widget			m_html;
	Gtk::Entry			m_address_bar;
	Gtk::Button			m_go_button;
	Gtk::VBox			m_vbox;
	Gtk::HBox			m_hbox;
	Gtk::ScrolledWindow	m_scrolled_wnd;
};

