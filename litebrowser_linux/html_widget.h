/*
 * html_widget.h
 *
 *  Created on: Aug 1, 2013
 *      Author: tordex
 */

#ifndef HTML_WIDGET_H_
#define HTML_WIDGET_H_

#include <gtkmm/drawingarea.h>
#include "cairo_container_linux.h"


class html_widget : public Gtk::DrawingArea,
					public cairo_container_linux
{
	litehtml::document::ptr		m_html;
	litehtml::context*			m_html_context;
public:
	html_widget(litehtml::context* html_context);
	virtual ~html_widget();

protected:
	virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr);

	virtual void get_client_rect(litehtml::position& client);
	virtual	void on_anchor_click(const litehtml::tchar_t* url, litehtml::element::ptr el);
	virtual	void set_cursor(const litehtml::tchar_t* cursor);
	virtual void import_css(litehtml::tstring& text, const litehtml::tstring& url, litehtml::tstring& baseurl, const litehtml::string_vector& media);
	virtual	void set_caption(const litehtml::tchar_t* caption);
	virtual	void set_base_url(const litehtml::tchar_t* base_url);
	virtual	void link(litehtml::document* doc, litehtml::element::ptr el);
};


#endif /* HTML_WIDGET_H_ */
