#include "test_container.h"
#include "Font.h"
#define CANVAS_ITY_IMPLEMENTATION
#include "canvas_ity.hpp"
string readfile(string filename);

//
//  canvas_ity adapters
//

void set_color(canvas& cvs, brush_type type, color c)
{
	cvs.set_color(type, c.r / 255.f, c.g / 255.f, c.b / 255.f, c.a / 255.f);
}

void fill_rect(canvas& cvs, rect r, color color)
{
	set_color(cvs, fill_style, color);
	cvs.fill_rectangle((float)r.x, (float)r.y, (float)r.width, (float)r.height);
}

void clip_rect(canvas& cvs, rect r)
{
	cvs.begin_path();
	cvs.rectangle((float)r.x, (float)r.y, (float)r.width, (float)r.height);
	cvs.clip();
}

void draw_image(canvas& cvs, int x, int y, const Bitmap& bmp)
{
	cvs.draw_image((byte*)bmp.data.data(), bmp.width, bmp.height, bmp.width * 4, (float)x, (float)y, (float)bmp.width, (float)bmp.height);
}

// endpoint is not drawn, like in GDI
void draw_line(canvas& cvs, int x0, int y0, int x1, int y1, color color)
{
	if (x0 != x1 && y0 != y1) return; // only horz and vert lines supported

	if (x0 == x1) // vert line
	{
		if (y0 > y1) swap(y0, y1);
		fill_rect(cvs, {x0, y0, 1, y1 - y0}, color);
	}
	else if (y0 == y1) // horz line
	{
		if (x0 > x1) swap(x0, x1);
		fill_rect(cvs, {x0, y0, x1 - x0, 1}, color);
	}
}


//
//  test_container implementation
//

// note: font is selected only by size, name and style are not used
uint_ptr test_container::create_font(const char* /*faceName*/, int size, int /*weight*/, font_style /*italic*/, unsigned int /*decoration*/, font_metrics* fm)
{
	Font* font = new Font(size);

	if (fm)
	{
		fm->ascent   = font->ascent;
		fm->descent  = font->descent;
		fm->height   = font->height;
		fm->x_height = font->x_height;
	}
	return (uint_ptr)font;
}

int test_container::text_width(const char* text, uint_ptr hFont)
{
	Font* font = (Font*)hFont;
	return (int)strlen(text) * font->width;
}

void test_container::draw_text(uint_ptr hdc, const char* text, uint_ptr hFont, web_color color, const position& pos)
{
	auto cvs = (canvas*)hdc;
	Font* font = (Font*)hFont;
	utf8_to_utf32 utf32(text);

	int x = pos.x;
	for (const char32_t* p = utf32; *p; p++)
	{
		Bitmap glyph = font->get_glyph(*p, color);
		::draw_image(*cvs, x, pos.y, glyph);
		x += glyph.width;
	}
}

int test_container::pt_to_px(int pt) const { return pt * 96 / 72; }
int test_container::get_default_font_size() const { return 16; }
const char* test_container::get_default_font_name() const { return ""; }

void test_container::draw_solid_fill(uint_ptr hdc, const background_layer& layer, const web_color& color)
{
	auto cvs = (canvas*)hdc;
	fill_rect(*cvs, layer.border_box, color);
}

void test_container::draw_borders(uint_ptr hdc, const borders& borders, const position& pos, bool /*root*/)
{
	auto& cvs = *(canvas*)hdc;

	// left border
	for (int x = 0; x < borders.left.width; x++)
		draw_line(cvs,
			pos.left() + x, pos.top(), 
			pos.left() + x, pos.bottom(), borders.left.color);

	// right border
	for (int x = 0; x < borders.right.width; x++)
		draw_line(cvs,
			pos.right() - x - 1, pos.top(),
			pos.right() - x - 1, pos.bottom(), borders.right.color);

	// top border
	for (int y = 0; y < borders.top.width; y++)
		draw_line(cvs,
			pos.left(),  pos.top() + y,
			pos.right(), pos.top() + y, borders.top.color);

	// bottom border
	for (int y = 0; y < borders.bottom.width; y++)
		draw_line(cvs,
			pos.left(),  pos.bottom() - y - 1,
			pos.right(), pos.bottom() - y - 1, borders.bottom.color);
}

void test_container::draw_list_marker(uint_ptr hdc, const list_marker& marker)
{
	auto cvs = (canvas*)hdc;
	fill_rect(*cvs, marker.pos, marker.color);
}

string getdir(string filename)
{
	auto i = filename.find_last_of("\\/");
	return filename.substr(0, i);
}

string test_container::make_url(const char* src, const char* baseurl)
{
	return (baseurl && *baseurl ? getdir(baseurl) : basedir) + "/" + src;
}

void test_container::import_css(string& text, const string& url, string& baseurl)
{
	baseurl = make_url(url.c_str(), baseurl.c_str());
	text = readfile(baseurl);
}

void test_container::get_client_rect(position& client) const
{
	client = {0, 0, width, height};
}

void test_container::get_media_features(media_features& media) const
{
	position client;
	get_client_rect(client);
	media.type        = media_type_screen;
	media.width       = client.width;
	media.height      = client.height;
	media.color       = 8; // same as Chrome/Firefox
	media.monochrome  = 0; // same as Chrome/Firefox
	media.color_index = 0; // same as Chrome/Firefox
	media.resolution  = 96; // same as Chrome/Firefox
}

void test_container::load_image(const char* src, const char* baseurl, bool /*redraw_on_ready*/)
{
	string url = make_url(src, baseurl);
	images[url] = Bitmap(url);
}

void test_container::get_image_size(const char* src, const char* baseurl, size& sz)
{
	string url = make_url(src, baseurl);
	auto& img = images[url];
	sz = {img.width, img.height};
}

void test_container::draw_image(uint_ptr hdc, const background_layer& bg, const string& src, const string& base_url)
{
	auto& cvs = *(canvas*)hdc;
	string url = make_url(src.c_str(), base_url.c_str());
	auto& img = images[url];
	if (!img) return;
	int x = bg.origin_box.x;
	int y = bg.origin_box.y;
	cvs.save();
	clip_rect(cvs, bg.clip_box);

	switch (bg.repeat)
	{
	case background_repeat_no_repeat:
		::draw_image(cvs, x, y, img);
		break;

	case background_repeat_repeat_x:
		while (x > bg.clip_box.left()) x -= img.width;
		for (; x < bg.clip_box.right(); x += img.width)
			::draw_image(cvs, x, y, img);
		break;

	case background_repeat_repeat_y:
		while (y > bg.clip_box.top()) y -= img.height;
		for (; y < bg.clip_box.bottom(); y += img.height)
			::draw_image(cvs, x, y, img);
		break;

	case background_repeat_repeat:
		while (x > bg.clip_box.left()) x -= img.width;
		while (y > bg.clip_box.top()) y -= img.height;
		for (; x < bg.clip_box.right(); x += img.width)
			for (int _y = y; _y < bg.clip_box.bottom(); _y += img.height)
				::draw_image(cvs, x, _y, img);
		break;
	}
	cvs.restore();
}
