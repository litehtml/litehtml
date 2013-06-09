#pragma once
#include <mlang.h>

struct linked_font
{
	typedef std::vector<linked_font*>	vector;

	DWORD				code_pages;
	HFONT				hFont;
	cairo_font_face_t*	font_face;
};

struct text_chunk
{
	typedef std::vector<text_chunk*>	vector;

	char*			text;
	linked_font*	font;

	~text_chunk()
	{
		if(text)
		{
			delete text;
		}
	}
};

struct cairo_font_metrics
{
	int		height;
	int		ascent;
	int		descent;
	int		x_height;
};


class cairo_font
{
	HFONT				m_hFont;
	cairo_font_face_t*	m_font_face;
	IMLangFontLink2*	m_font_link;
	DWORD				m_font_code_pages;
	linked_font::vector	m_linked_fonts;
	int					m_size;
	BOOL				m_bUnderline;
	BOOL				m_bStrikeOut;
public:
	cairo_font(IMLangFontLink2* fl, HFONT hFont, int size);
	cairo_font(IMLangFontLink2* fl, LPCWSTR facename, int size, int weight, BOOL italic, BOOL strikeout, BOOL underline);

	void init();
	~cairo_font();

	void				show_text(cairo_t* cr, int x, int y, LPCWSTR str);
	int					text_width(cairo_t* cr, LPCWSTR str);
	void				get_metrics(cairo_t* cr, cairo_font_metrics* fm);
private:
	void				split_text(LPCWSTR str, text_chunk::vector& chunks);
	void				free_text_chunks(text_chunk::vector& chunks);
	cairo_font_face_t*	create_font_face(HFONT fnt);
	void				set_font(HFONT hFont);
	void				clear();
	int					text_width(cairo_t* cr, text_chunk::vector& chunks);
};