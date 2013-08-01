/*
 * browser_wnd.h
 *
 *  Created on: Aug 1, 2013
 *      Author: tordex
 */

#ifndef BROWSER_WND_H_
#define BROWSER_WND_H_

#include <gtkmm/window.h>
#include "html_widget.h"
#include <litehtml.h>

class browser_window : public Gtk::Window
{
public:
	browser_window(litehtml::context* html_context);
	virtual ~browser_window();

protected:
	html_widget		m_html;
};


#endif /* BROWSER_WND_H_ */
