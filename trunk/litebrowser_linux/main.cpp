/*
 * main.cpp
 *
 *  Created on: Jul 31, 2013
 *      Author: tordex
 */
#include "browser_wnd.h"
#include <gtkmm/application.h>
#include <gtkmm/window.h>
#include <litehtml.h>
#include <fstream>
#include <string>
#include <cerrno>
#include <clocale>

std::string get_file_contents(const char *filename)
{
  std::ifstream in(filename, std::ios::in | std::ios::binary);
  if (in)
  {
    std::string contents;
    in.seekg(0, std::ios::end);
    contents.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(&contents[0], contents.size());
    in.close();
    return(contents);
  }
  throw(errno);
}

int main (int argc, char *argv[])
{
	Glib::RefPtr<Gtk::Application> app = Gtk::Application::create(argc, argv, "litehtml.browser");

	litehtml::tstring css = get_file_contents("/home/tordex/projects/litehtml/include/master.css");

	litehtml::context html_context;
	html_context.load_master_stylesheet(css.c_str());

	browser_window win(&html_context);

	return app->run(win);
}

