#include "py_container.h"

extern "C" uint_ptr container_create(int width, int height, PYCB py) {
    py_container *c = new py_container(width, height, py);
    return (uint_ptr)c;
}

extern "C" void container_delete(uint_ptr *ctr) {
    py_container *c = (py_container *)ctr;
    delete c;
}

extern "C" void container_render(uint_ptr *ctr, char *html) {
    py_container *c = (py_container *)ctr;
    position clip(0, 0, c->width, c->height);
    auto doc = document::createFromString(html, c);
    doc->render(c->width);
    doc->draw((uint_ptr)NULL, 0, 0, &clip);
}

struct CreateFont {
// in
    const char *faceName;
    int size;
    int weight;
    int italic;
    unsigned int decoration;
// out
    int ascent;
    int descent;
    int height;
    int x_height;
    int font;
};

// note: font is selected only by size, name and style are not used
uint_ptr py_container::create_font(const char* faceName, int size, int weight, font_style italic, unsigned int decoration, font_metrics* fm)
{
    struct CreateFont fi;
    fi.faceName = faceName;
    fi.size = size;
    fi.weight = weight;
    fi.italic = (int)italic;
    fi.decoration = decoration;
    (*py)("createFont", (void *)&fi);

    if (fm)
    {
        fm->ascent   = fi.ascent;
        fm->descent  = fi.descent;
        fm->height   = fi.height;
        fm->x_height = fi.x_height;
    }
    return (uint_ptr)fi.font;
}

struct TextWidth {
// in
    const char *text;
    int font;
// out
    int width;
};

int py_container::text_width(const char* text, uint_ptr hFont)
{
    struct TextWidth fi;
    fi.text = text;
    fi.font = (int)hFont;
    (*py)("textWidth", (void *)&fi);
    return fi.width;
}

#define C32(c, s) (((unsigned int)c)<<s)
#define RGBA(color) C32(color.red, 24) | C32(color.green, 16) | C32(color.blue, 8) | C32(color.alpha, 0)
struct DrawText {
// in
    int dc;
    const char *text;
    int font;
    unsigned int color;
    int x;
    int y;
};

void py_container::draw_text(uint_ptr hdc, const char* text, uint_ptr hFont, web_color color, const position& pos)
{
    struct DrawText fi;
    fi.dc = (int)hdc;
    fi.text = text;
    fi.font = (int)hFont;
    fi.color = RGBA(color);
    fi.x = pos.x;
    fi.y = pos.y;
    (*py)("drawText", (void *)&fi);
}

struct PT2PX {
    int pt;
};

int py_container::pt_to_px(int pt) const {
    struct PT2PX fi;
    fi.pt = pt;
    (*py)("pt2px", (void *)&fi);
    return fi.pt;
}

int py_container::get_default_font_size() const {
    return 16;
}

const char* py_container::get_default_font_name() const {
    return "";
}

struct DrawBackground {
// in
    int dc;
    int x;
    int y;
    int w;
    int h;
    unsigned int color;
// out
};

void py_container::draw_background(uint_ptr hdc, const background_paint& bg)
{
    struct DrawBackground fi;
    fi.dc = (int)hdc;
    fi.x = bg.border_box.x;
    fi.y = bg.border_box.y;
    fi.w = bg.border_box.width;
    fi.h = bg.border_box.height;
    fi.color = RGBA(bg.color);
    (*py)("drawBackground", (void *)&fi);
}

struct DrawBorders {
// in
    int dc;
    int left;
    int right;
    int top;
    int bottom;
    unsigned int colorLeft;
    unsigned int colorRight;
    unsigned int colorTop;
    unsigned int colorBottom;
    int widthLeft;
    int widthRight;
    int widthTop;
    int widthBottom;
// out
};
void py_container::draw_borders(uint_ptr hdc, const borders& borders, const position& pos, bool root)
{
    struct DrawBorders fi;
    fi.dc = (int)hdc;
    fi.left = pos.left();
    fi.right = pos.right();
    fi.top = pos.top();
    fi.bottom = pos.bottom();
    fi.colorLeft = RGBA(borders.left.color);
    fi.colorRight = RGBA(borders.right.color);
    fi.colorTop = RGBA(borders.top.color);
    fi.colorBottom = RGBA(borders.bottom.color);
    fi.widthLeft = borders.left.width;
    fi.widthRight = borders.right.width;
    fi.widthTop = borders.top.width;
    fi.widthBottom = borders.bottom.width;
    (*py)("drawBorders", (void *)&fi);
}

struct DrawMarker {
// in
    int dc;
    int x;
    int y;
    int w;
    int h;
    int mt;
    unsigned int color;
// out
};

void py_container::draw_list_marker(uint_ptr hdc, const list_marker& marker)
{
    struct DrawMarker fi;

    int top_margin = marker.pos.height / 3;
    if (top_margin < 4)
        top_margin = 0;

    int draw_x = marker.pos.x;
    int draw_y = marker.pos.y + top_margin;
    int draw_width = marker.pos.height - top_margin * 2;
    int draw_height = marker.pos.height - top_margin * 2;

    fi.dc = (int)hdc;
    fi.x = draw_x;
    fi.y = draw_y;
    fi.w = draw_width;
    fi.h = draw_height;
    fi.mt = (int)marker.marker_type;
    fi.color = RGBA(marker.color);
    (*py)("drawMarker", (void *)&fi);
}

void py_container::import_css(string& text, const string& url, string& baseurl)
{
    baseurl = url;
    text = "";
}

void py_container::get_client_rect(position& client) const
{
    client = position(0, 0, width, height);
}
