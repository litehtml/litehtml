/*
 * broser_wnd.cpp
 *
 *  Created on: Aug 1, 2013
 *      Author: tordex
 */
#include "browser_wnd.h"
#include <gtkmm.h>

browser_window::browser_window(litehtml::context* html_context) : m_html(html_context)
{
	set_title("litehtml");

	add(m_html);
	m_html.show();
}

browser_window::~browser_window()
{

}

