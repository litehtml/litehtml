/*
 * html_widget.cpp
 *
 *  Created on: Aug 1, 2013
 *      Author: tordex
 */
#include "html_widget.h"
#include <cairomm/context.h>


html_widget::html_widget(litehtml::context* html_context)
{
	m_html_context = html_context;
	m_html = litehtml::document::createFromString("<h1>.:: Привет litehtml ::.</h1><p>&#163; fast and lightweight HTML/CSS open source rendering engine</p>", this, m_html_context);
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

	m_html->render(pos.width);
	m_html->draw((litehtml::uint_ptr) cr->cobj(), 0, 0, &pos);

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

}

void html_widget::set_caption(const litehtml::tchar_t* caption)
{
	get_parent_window()->set_title(caption);
}

void html_widget::set_base_url(const litehtml::tchar_t* base_url)
{

}

void html_widget::link(litehtml::document* doc, litehtml::element::ptr el)
{

}

