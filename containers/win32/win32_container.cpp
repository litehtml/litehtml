#include "win32_container.h"

win32_container::win32_container()
{
	m_hClipRgn = NULL;
	m_tmp_hdc = GetDC(NULL);
	InitializeCriticalSection(&m_img_sync);

	EnumFonts(m_tmp_hdc, NULL, EnumFontsProc, (LPARAM)this);
	m_installed_fonts.insert(L"monospace");
	m_installed_fonts.insert(L"serif");
	m_installed_fonts.insert(L"sans-serif");
	m_installed_fonts.insert(L"fantasy");
	m_installed_fonts.insert(L"cursive");
}

win32_container::~win32_container()
{
	clear_images();
	DeleteCriticalSection(&m_img_sync);
	if(m_hClipRgn)
	{
		DeleteObject(m_hClipRgn);
	}
	ReleaseDC(NULL, m_tmp_hdc);
}

int CALLBACK win32_container::EnumFontsProc(const LOGFONT* lplf, const TEXTMETRIC* lptm, DWORD dwType, LPARAM lpData)
{
	win32_container* container = (win32_container*)lpData;
	container->m_installed_fonts.insert(lplf->lfFaceName);
	return 1;
}

static LPCWSTR get_exact_font_name(LPCWSTR facename)
{
	if      (!lstrcmpi(facename, L"monospace"))		return L"Courier New";
	else if (!lstrcmpi(facename, L"serif"))			return L"Times New Roman";
	else if (!lstrcmpi(facename, L"sans-serif"))	return L"Arial";
	else if (!lstrcmpi(facename, L"fantasy"))		return L"Impact";
	else if (!lstrcmpi(facename, L"cursive"))		return L"Comic Sans MS";
	else											return facename;
}

static void trim_quotes(litehtml::tstring& str)
{
	if (str.front() == L'"' || str.front() == L'\'')
		str.erase(0, 1);

	if (str.back() == L'"' || str.back() == L'\'')
		str.erase(str.length() - 1, 1);
}

litehtml::uint_ptr win32_container::create_font( const litehtml::tchar_t* font_list, int size, int weight, litehtml::font_style italic, unsigned int decoration, litehtml::font_metrics* fm )
{
	std::wstring font_name;
	litehtml::string_vector fonts;
	litehtml::split_string(font_list, fonts, _t(","));
	bool found = false;
	for (auto& name : fonts)
	{
		litehtml::trim(name);
		trim_quotes(name);
		std::wstring wname = litehtml_to_wchar(name.c_str());
		if (m_installed_fonts.count(wname))
		{
			font_name = wname;
			found = true;
			break;
		}
	}
	if (!found) font_name = litehtml_to_wchar(get_default_font_name());
	font_name = get_exact_font_name(font_name.c_str());

	LOGFONT lf = {};
	wcscpy_s(lf.lfFaceName, LF_FACESIZE, font_name.c_str());

	lf.lfHeight			= -size;
	lf.lfWeight			= weight;
	lf.lfItalic			= (italic == litehtml::fontStyleItalic) ? TRUE : FALSE;
	lf.lfCharSet		= DEFAULT_CHARSET;
	lf.lfOutPrecision	= OUT_DEFAULT_PRECIS;
	lf.lfClipPrecision	= CLIP_DEFAULT_PRECIS;
	lf.lfQuality		= DEFAULT_QUALITY;
	lf.lfStrikeOut		= (decoration & litehtml::font_decoration_linethrough) ? TRUE : FALSE;
	lf.lfUnderline		= (decoration & litehtml::font_decoration_underline) ? TRUE : FALSE;
	HFONT hFont = CreateFontIndirect(&lf);

	if (fm)
	{
		SelectObject(m_tmp_hdc, hFont);
		TEXTMETRIC tm = {};
		GetTextMetrics(m_tmp_hdc, &tm);
		fm->ascent = tm.tmAscent;
		fm->descent = tm.tmDescent;
		fm->height = tm.tmHeight;
		fm->x_height = tm.tmHeight / 2;   // this is an estimate; call GetGlyphOutline to get the real value
		fm->draw_spaces = italic || decoration;
	}

	return (uint_ptr) hFont;
}

void win32_container::delete_font( uint_ptr hFont )
{
	DeleteObject((HFONT) hFont);
}

const litehtml::tchar_t* win32_container::get_default_font_name() const
{
	return _t("Times New Roman");
}

int win32_container::get_default_font_size() const
{
	return 16;
}

int win32_container::text_width( const litehtml::tchar_t* text, uint_ptr hFont )
{
	SIZE size = {};
	SelectObject(m_tmp_hdc, (HFONT)hFont);
	GetTextExtentPoint32(m_tmp_hdc, litehtml_to_wchar(text), (int)t_strlen(text), &size);
	return size.cx;
}

void win32_container::draw_text( uint_ptr hdc, const litehtml::tchar_t* text, uint_ptr hFont, litehtml::web_color color, const litehtml::position& pos )
{
	apply_clip((HDC) hdc);

	HFONT oldFont = (HFONT) SelectObject((HDC) hdc, (HFONT) hFont);

	SetBkMode((HDC) hdc, TRANSPARENT);

	SetTextColor((HDC) hdc, RGB(color.red, color.green, color.blue));

	RECT rcText = { pos.left(), pos.top(), pos.right(), pos.bottom() };
	DrawText((HDC) hdc, litehtml_to_wchar(text), -1, &rcText, DT_SINGLELINE | DT_NOPREFIX | DT_BOTTOM | DT_NOCLIP);

	SelectObject((HDC) hdc, oldFont);

	release_clip((HDC) hdc);
}

int win32_container::pt_to_px( int pt ) const
{
	return MulDiv(pt, GetDeviceCaps(m_tmp_hdc, LOGPIXELSY), 72);
}

void win32_container::draw_list_marker(uint_ptr hdc, const litehtml::list_marker& marker)
{
	apply_clip((HDC)hdc);

	int top_margin = marker.pos.height / 3;
	if (top_margin < 4)
		top_margin = 0;

	int draw_x = marker.pos.x;
	int draw_y = marker.pos.y + top_margin;
	int draw_width = marker.pos.height - top_margin * 2;
	int draw_height = marker.pos.height - top_margin * 2;

	switch (marker.marker_type)
	{
	case litehtml::list_style_type_circle:
		{
			draw_ellipse((HDC)hdc, draw_x, draw_y, draw_width, draw_height, marker.color, 1);
		}
		break;
	case litehtml::list_style_type_disc:
		{
			fill_ellipse((HDC)hdc, draw_x, draw_y, draw_width, draw_height, marker.color);
		}
		break;
	case litehtml::list_style_type_square:
		{
			fill_rect((HDC)hdc, draw_x, draw_y, draw_width, draw_height, marker.color);
		}
		break;
	}
	release_clip((HDC)hdc);
}

void win32_container::make_url_utf8(const char* url, const char* basepath, std::wstring& out)
{
	make_url(litehtml::utf8_to_wchar(url), litehtml::utf8_to_wchar(basepath), out);
}

void win32_container::load_image( const litehtml::tchar_t* src, const litehtml::tchar_t* baseurl, bool redraw_on_ready )
{
	std::wstring url;
	t_make_url(src, baseurl, url);
	
	lock_images_cache();
	if (m_images.count(url) == 0)
	{
		unlock_images_cache();
		uint_ptr img = get_image(url.c_str(), redraw_on_ready);
		add_image(url.c_str(), img);
	}
	else
	{
		unlock_images_cache();
	}
}

void win32_container::add_image(LPCWSTR url, uint_ptr img)
{
	lock_images_cache();
	m_images[url] = img;
	unlock_images_cache();
}

void win32_container::get_image_size( const litehtml::tchar_t* src, const litehtml::tchar_t* baseurl, litehtml::size& sz )
{
	std::wstring url;
	t_make_url(src, baseurl, url);

	sz.width  = 0;
	sz.height = 0;

	lock_images_cache();
	images_map::iterator img = m_images.find(url);
	if(img != m_images.end() && img->second)
	{
		get_img_size(img->second, sz);
	}
	unlock_images_cache();
}

void win32_container::clear_images()
{
	lock_images_cache();
	for(auto& img : m_images)
	{
		if(img.second)
		{
			free_image(img.second);
		}
	}
	m_images.clear();
	unlock_images_cache();
}

void win32_container::lock_images_cache()
{
	EnterCriticalSection(&m_img_sync);
}

void win32_container::unlock_images_cache()
{
	LeaveCriticalSection(&m_img_sync);
}

static void FillSolidRect(HDC hdc, LPCRECT lpRect, COLORREF clr)
{
	COLORREF clrOld = SetBkColor(hdc, clr);
	ExtTextOut(hdc, 0, 0, ETO_OPAQUE, lpRect, NULL, 0, NULL);
	SetBkColor(hdc, clrOld);
}

void win32_container::draw_background( uint_ptr _hdc, const litehtml::background_paint& bg )
{
	HDC hdc = (HDC)_hdc;
	apply_clip(hdc);

	RECT rect = { bg.border_box.left(), bg.border_box.top(), bg.border_box.right(), bg.border_box.bottom() };
	COLORREF color = RGB(bg.color.red, bg.color.green, bg.color.blue);
	// alpha channel for background color is not supported; alpha below some threshold is considered transparent, above it - opaque
	if (bg.color.alpha > 30)
	{
		FillSolidRect(hdc, &rect, color);
	}

	std::wstring url;
	t_make_url(bg.image.c_str(), bg.baseurl.c_str(), url);

	lock_images_cache();
	images_map::iterator img = m_images.find(url);
	if(img != m_images.end() && img->second)
	{
		draw_img_bg(hdc, img->second, bg);
	}
	unlock_images_cache();

	release_clip(hdc);
}

void win32_container::set_clip( const litehtml::position& pos, const litehtml::border_radiuses& bdr_radius, bool valid_x, bool valid_y )
{
	litehtml::position clip_pos = pos;
	litehtml::position client_pos;
	get_client_rect(client_pos);
	if(!valid_x)
	{
		clip_pos.x		= client_pos.x;
		clip_pos.width	= client_pos.width;
	}
	if(!valid_y)
	{
		clip_pos.y		= client_pos.y;
		clip_pos.height	= client_pos.height;
	}
	m_clips.push_back(clip_pos);
}

void win32_container::del_clip()
{
	if(!m_clips.empty())
	{
		m_clips.pop_back();
	}
}

void win32_container::apply_clip(HDC hdc)
{
	if(m_hClipRgn)
	{
		DeleteObject(m_hClipRgn);
		m_hClipRgn = NULL;
	}

	if(!m_clips.empty())
	{
		POINT ptView = {0, 0};
		GetWindowOrgEx(hdc, &ptView);

		litehtml::position clip_pos = m_clips.back();
		m_hClipRgn = CreateRectRgn(clip_pos.left() - ptView.x, clip_pos.top() - ptView.y, clip_pos.right() - ptView.x, clip_pos.bottom() - ptView.y);
		SelectClipRgn(hdc, m_hClipRgn);
	}
}

void win32_container::release_clip(HDC hdc)
{
	SelectClipRgn(hdc, NULL);

	if(m_hClipRgn)
	{
		DeleteObject(m_hClipRgn);
		m_hClipRgn = NULL;
	}
}

litehtml::element::ptr win32_container::create_element(const litehtml::tchar_t* tag_name, const litehtml::string_map& attributes, const litehtml::document::ptr& doc)
{
	return 0;
}

void win32_container::get_media_features(litehtml::media_features& media)  const
{
	litehtml::position client;
	get_client_rect(client);

	media.type = litehtml::media_type_screen;
	media.width = client.width;
	media.height = client.height;
	media.color = 8;
	media.monochrome = 0;
	media.color_index = 256;
	media.resolution = GetDeviceCaps(m_tmp_hdc, LOGPIXELSX);
	media.device_width = GetDeviceCaps(m_tmp_hdc, HORZRES);
	media.device_height = GetDeviceCaps(m_tmp_hdc, VERTRES);
}

void win32_container::get_language(litehtml::tstring& language, litehtml::tstring& culture) const
{
	language = _t("en");
	culture = _t("");
}

void win32_container::transform_text(litehtml::tstring& text, litehtml::text_transform tt)
{
	if (text.empty()) return;

	LPWSTR txt = _wcsdup(litehtml_to_wchar(text.c_str()));
	switch (tt)
	{
	case litehtml::text_transform_capitalize:
		CharUpperBuff(txt, 1);
		break;
	case litehtml::text_transform_uppercase:
		CharUpperBuff(txt, lstrlen(txt));
		break;
	case litehtml::text_transform_lowercase:
		CharLowerBuff(txt, lstrlen(txt));
		break;
	}
	text = litehtml_from_wchar(txt);
	free(txt);
}

void win32_container::link(const litehtml::document::ptr& doc, const litehtml::element::ptr& el)
{
}

litehtml::tstring win32_container::resolve_color(const litehtml::tstring& color) const
{
	struct custom_color
	{
		litehtml::tchar_t*	name;
		int					color_index;
	};

	static custom_color colors[] = {
		{ _t("ActiveBorder"),          COLOR_ACTIVEBORDER},
		{ _t("ActiveCaption"),         COLOR_ACTIVECAPTION},
		{ _t("AppWorkspace"),          COLOR_APPWORKSPACE },
		{ _t("Background"),            COLOR_BACKGROUND },
		{ _t("ButtonFace"),            COLOR_BTNFACE },
		{ _t("ButtonHighlight"),       COLOR_BTNHIGHLIGHT },
		{ _t("ButtonShadow"),          COLOR_BTNSHADOW },
		{ _t("ButtonText"),            COLOR_BTNTEXT },
		{ _t("CaptionText"),           COLOR_CAPTIONTEXT },
		{ _t("GrayText"),              COLOR_GRAYTEXT },
		{ _t("Highlight"),             COLOR_HIGHLIGHT },
		{ _t("HighlightText"),         COLOR_HIGHLIGHTTEXT },
		{ _t("InactiveBorder"),        COLOR_INACTIVEBORDER },
		{ _t("InactiveCaption"),       COLOR_INACTIVECAPTION },
		{ _t("InactiveCaptionText"),   COLOR_INACTIVECAPTIONTEXT },
		{ _t("InfoBackground"),        COLOR_INFOBK },
		{ _t("InfoText"),              COLOR_INFOTEXT },
		{ _t("Menu"),                  COLOR_MENU },
		{ _t("MenuText"),              COLOR_MENUTEXT },
		{ _t("Scrollbar"),             COLOR_SCROLLBAR },
		{ _t("ThreeDDarkShadow"),      COLOR_3DDKSHADOW },
		{ _t("ThreeDFace"),            COLOR_3DFACE },
		{ _t("ThreeDHighlight"),       COLOR_3DHILIGHT },
		{ _t("ThreeDLightShadow"),     COLOR_3DLIGHT },
		{ _t("ThreeDShadow"),          COLOR_3DSHADOW },
		{ _t("Window"),                COLOR_WINDOW },
		{ _t("WindowFrame"),           COLOR_WINDOWFRAME },
		{ _t("WindowText"),            COLOR_WINDOWTEXT }
	};

	for (auto& clr : colors)
	{
		if (!litehtml::t_strcasecmp(color.c_str(), clr.name))
		{
			litehtml::tchar_t  str_clr[20];
			DWORD rgb_color = GetSysColor(clr.color_index);
			t_snprintf(str_clr, 20, _t("#%02X%02X%02X"), GetRValue(rgb_color), GetGValue(rgb_color), GetBValue(rgb_color));
			return std::move(litehtml::tstring(str_clr));
		}
	}
	return std::move(litehtml::tstring());
}
