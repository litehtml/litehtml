#pragma once

#include <fstream>
#include <string>
#include <cerrno>
#include <clocale>
#include <vector>
#include <iostream>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <cairo.h>
#include <gtkmm.h>
#include <gtkmm/application.h>
#include <gtkmm/window.h>
#include "../include/litehtml.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <fontconfig/fontconfig.h>
#include <cairo-ft.h>
#include <gdk/gdk.h>
#include <cairomm/context.h>
#include <curl/curl.h>
#include <Uri.h>

extern Glib::RefPtr< Gio::InputStream > load_file(const litehtml::tstring& url);
extern std::string urljoin(const std::string &base, const std::string &relative);
extern void load_text_file(const litehtml::tstring& url, litehtml::tstring& out);
