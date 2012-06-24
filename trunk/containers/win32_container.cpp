#include "..\include/litehtml.h"
#include "..\litehtml\tokenizer.h"
#include "win32_container.h"

using namespace Gdiplus;

litehtml::uint_ptr litehtml::win32_container::create_font( const wchar_t* faceName, int size, int weight, font_style italic, unsigned int decoration )
{
	litehtml::string_vector fonts;
	tokenize(faceName, fonts, L",");
	litehtml::trim(fonts[0]);

	LOGFONT lf;
	ZeroMemory(&lf, sizeof(lf));
	wcscpy_s(lf.lfFaceName, LF_FACESIZE, fonts[0].c_str());

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

	return (uint_ptr) hFont;
}

void litehtml::win32_container::delete_font( uint_ptr hFont )
{
	DeleteObject((HFONT) hFont);
}

int litehtml::win32_container::line_height( uint_ptr hdc, uint_ptr hFont )
{
	HFONT oldFont = (HFONT) SelectObject((HDC) hdc, (HFONT) hFont);
	TEXTMETRIC tm;
	GetTextMetrics((HDC) hdc, &tm);
	SelectObject((HDC) hdc, oldFont);
	return (int) tm.tmHeight;
}

int litehtml::win32_container::text_width( uint_ptr hdc, const wchar_t* text, uint_ptr hFont )
{
	HFONT oldFont = (HFONT) SelectObject((HDC) hdc, (HFONT) hFont);

	SIZE sz = {0, 0};

	GetTextExtentPoint32((HDC) hdc, text, lstrlen(text), &sz);

	SelectObject((HDC) hdc, oldFont);

	return (int) sz.cx;
}

void litehtml::win32_container::draw_text( uint_ptr hdc, const wchar_t* text, uint_ptr hFont, litehtml::web_color color, const litehtml::position& pos )
{
	HFONT oldFont = (HFONT) SelectObject((HDC) hdc, (HFONT) hFont);

	SetBkMode((HDC) hdc, TRANSPARENT);

	SetTextColor((HDC) hdc, RGB(color.red, color.green, color.blue));

	RECT rcText = { pos.left(), pos.top(), pos.right(), pos.bottom() };
	DrawText((HDC) hdc, text, -1, &rcText, DT_SINGLELINE | DT_NOPREFIX | DT_BOTTOM | DT_NOCLIP);

	SelectObject((HDC) hdc, oldFont);
}

void litehtml::win32_container::fill_rect( uint_ptr hdc, const litehtml::position& pos, litehtml::web_color color )
{
	HBRUSH br = CreateSolidBrush(RGB(color.red, color.green, color.blue));
	RECT rcFill = { pos.left(), pos.top(), pos.right(), pos.bottom() };
	FillRect((HDC) hdc, &rcFill, br);
	DeleteObject(br);
}

litehtml::uint_ptr litehtml::win32_container::get_temp_dc()
{
	return (litehtml::uint_ptr) GetDC(NULL);
}

void litehtml::win32_container::release_temp_dc( uint_ptr hdc )
{
	ReleaseDC(NULL, (HDC) hdc);
}

int litehtml::win32_container::pt_to_px( int pt )
{
	HDC dc = GetDC(NULL);
	int ret = MulDiv(pt, GetDeviceCaps(dc, LOGPIXELSY), 72);
	ReleaseDC(NULL, dc);
	return ret;
}

int litehtml::win32_container::get_text_base_line( uint_ptr hdc, uint_ptr hFont )
{
	HDC dc = (HDC) hdc;
	if(!dc)
	{
		dc = GetDC(NULL);
	}
	HFONT oldFont = (HFONT) SelectObject(dc, (HFONT) hFont);
	TEXTMETRIC tm;
	GetTextMetrics(dc, &tm);
	SelectObject(dc, oldFont);
	if(!hdc)
	{
		ReleaseDC(NULL, dc);
	}
	return (int) tm.tmDescent;
}

void litehtml::win32_container::draw_list_marker( uint_ptr hdc, list_style_type marker_type, int x, int y, int height, const web_color& color )
{
	Graphics graphics((HDC) hdc);
	LinearGradientBrush* brush = NULL;

	int top_margin = height / 3;

	Rect rc(x, y + top_margin, 
		height - top_margin * 2, 
		height - top_margin * 2);

	switch(marker_type)
	{
	case list_style_type_circle:
		{
			graphics.SetCompositingQuality(CompositingQualityHighQuality);
			graphics.SetSmoothingMode(SmoothingModeAntiAlias);
			Pen pen( Color(color.alpha, color.red, color.green, color.blue) );
			graphics.DrawEllipse(&pen, rc.X, rc.Y, rc.Width, rc.Height);
		}
		break;
	case list_style_type_disc:
		{
			graphics.SetCompositingQuality(CompositingQualityHighQuality);
			graphics.SetSmoothingMode(SmoothingModeAntiAlias);
			SolidBrush brush( Color(color.alpha, color.red, color.green, color.blue) );
			graphics.FillEllipse(&brush, rc.X, rc.Y, rc.Width, rc.Height);
		}
		break;
	case list_style_type_square:
		{
			SolidBrush brush( Color(color.alpha, color.red, color.green, color.blue) );
			graphics.FillRectangle(&brush, rc.X, rc.Y, rc.Width, rc.Height);
		}
		break;
	}
}

void litehtml::win32_container::load_image( const wchar_t* src, const wchar_t* baseurl )
{
	std::wstring url;
	make_url(src, baseurl, url);
	if(m_images.find(url.c_str()) == m_images.end())
	{
		Bitmap* img = get_image(url.c_str());
		if(img)
		{ 
			m_images[url.c_str()] = img;
		}
	}
}

void litehtml::win32_container::get_image_size( const wchar_t* src, const wchar_t* baseurl, litehtml::size& sz )
{
	std::wstring url;
	make_url(src, baseurl, url);

	images_map::iterator img = m_images.find(url.c_str());
	if(img != m_images.end())
	{
		sz.width	= (int) img->second->GetWidth();
		sz.height	= (int) img->second->GetHeight();
	}
}

void litehtml::win32_container::draw_image( uint_ptr hdc, const wchar_t* src, const wchar_t* baseurl, const litehtml::position& pos )
{
	std::wstring url;
	make_url(src, baseurl, url);
	images_map::iterator img = m_images.find(url.c_str());
	if(img != m_images.end())
	{
		Graphics graphics((HDC) hdc);
		graphics.SetInterpolationMode(InterpolationModeNearestNeighbor);
		graphics.SetPixelOffsetMode(PixelOffsetModeHalf);
		graphics.DrawImage(img->second, pos.x, pos.y, pos.width, pos.height);

		img->second->GetWidth();
	}
}

void litehtml::win32_container::clear_images()
{
	for(images_map::iterator i = m_images.begin(); i != m_images.end(); i++)
	{
		if(i->second)
		{
			delete i->second;
		}
	}
	m_images.clear();
}

int litehtml::win32_container::get_default_font_size()
{
	return 16;
}

void litehtml::win32_container::draw_background( uint_ptr hdc, const wchar_t* image, const wchar_t* baseurl, const litehtml::position& draw_pos, const litehtml::css_position& bg_pos, litehtml::background_repeat repeat, litehtml::background_attachment attachment )
{
	std::wstring url;
	make_url(image, baseurl, url);

	images_map::iterator img = m_images.find(url.c_str());
	if(img != m_images.end())
	{
		int img_width	= img->second->GetWidth();
		int img_height	= img->second->GetHeight();

		Graphics graphics((HDC) hdc);
		graphics.SetInterpolationMode(InterpolationModeNearestNeighbor);
		graphics.SetPixelOffsetMode(PixelOffsetModeHalf);

		Region reg(Rect(draw_pos.left(), draw_pos.top(), draw_pos.width, draw_pos.height));
		graphics.SetClip(&reg);

		litehtml::position pos = draw_pos;

		if(bg_pos.x.units() != css_units_percentage)
		{
			pos.x += (int) bg_pos.x.val();
		} else
		{
			pos.x += (int) ((float) (draw_pos.width - img_width) * bg_pos.x.val() / 100.0);
		}

		if(bg_pos.y.units() != css_units_percentage)
		{
			pos.y += (int) bg_pos.y.val();
		} else
		{
			pos.y += (int) ( (float) (draw_pos.height - img_height) * bg_pos.y.val() / 100.0);
		}

		switch(repeat)
		{
		case background_repeat_no_repeat:
			{
				graphics.DrawImage(img->second, pos.x, pos.y, img->second->GetWidth(), img->second->GetHeight());
			}
			break;
		case background_repeat_repeat_x:
			{
				Gdiplus::CachedBitmap bmp(img->second, &graphics);
				for(int x = pos.left(); x < pos.right(); x += img->second->GetWidth())
				{
					graphics.DrawCachedBitmap(&bmp, x, pos.top());
				}
			}
			break;
		case background_repeat_repeat_y:
			{
				Gdiplus::CachedBitmap bmp(img->second, &graphics);
				for(int y = pos.top(); y < pos.bottom(); y += img->second->GetHeight())
				{
					graphics.DrawCachedBitmap(&bmp, pos.left(), y);
				}
			}
			break;
		case background_repeat_repeat:
			{
				Gdiplus::CachedBitmap bmp(img->second, &graphics);
				if(img->second->GetHeight() >= 0)
				{
					for(int x = pos.left(); x < pos.right(); x += img->second->GetWidth())
					{
						for(int y = pos.top(); y < pos.bottom(); y += img->second->GetHeight())
						{
							graphics.DrawCachedBitmap(&bmp, x, y);
						}
					}
				}
			}
			break;
		}
	}
}

void litehtml::win32_container::draw_borders( uint_ptr hdc, const css_borders& borders, const litehtml::position& draw_pos )
{
	// draw left border
	if(borders.left.width.val() != 0 && borders.left.style > border_style_hidden)
	{
		HPEN pen = CreatePen(PS_SOLID, 1, RGB(borders.left.color.red, borders.left.color.green, borders.left.color.blue));
		HPEN oldPen = (HPEN) SelectObject((HDC) hdc, pen);
		for(int x = 0; x < borders.left.width.val(); x++)
		{
			MoveToEx((HDC) hdc, draw_pos.left() + x, draw_pos.top(), NULL);
			LineTo((HDC) hdc, draw_pos.left() + x, draw_pos.bottom());
		}
		SelectObject((HDC) hdc, oldPen);
		DeleteObject(pen);
	}
	// draw right border
	if(borders.right.width.val() != 0 && borders.right.style > border_style_hidden)
	{
		HPEN pen = CreatePen(PS_SOLID, 1, RGB(borders.right.color.red, borders.right.color.green, borders.right.color.blue));
		HPEN oldPen = (HPEN) SelectObject((HDC) hdc, pen);
		for(int x = 0; x < borders.right.width.val(); x++)
		{
			MoveToEx((HDC) hdc, draw_pos.right() - x - 1, draw_pos.top(), NULL);
			LineTo((HDC) hdc, draw_pos.right() - x - 1, draw_pos.bottom());
		}
		SelectObject((HDC) hdc, oldPen);
		DeleteObject(pen);
	}
	// draw top border
	if(borders.top.width.val() != 0 && borders.top.style > border_style_hidden)
	{
		HPEN pen = CreatePen(PS_SOLID, 1, RGB(borders.top.color.red, borders.top.color.green, borders.top.color.blue));
		HPEN oldPen = (HPEN) SelectObject((HDC) hdc, pen);
		for(int y = 0; y < borders.top.width.val(); y++)
		{
			MoveToEx((HDC) hdc, draw_pos.left(), draw_pos.top() + y, NULL);
			LineTo((HDC) hdc, draw_pos.right(), draw_pos.top() + y);
		}
		SelectObject((HDC) hdc, oldPen);
		DeleteObject(pen);
	}
	// draw bottom border
	if(borders.bottom.width.val() != 0 && borders.bottom.style > border_style_hidden)
	{
		HPEN pen = CreatePen(PS_SOLID, 1, RGB(borders.bottom.color.red, borders.bottom.color.green, borders.bottom.color.blue));
		HPEN oldPen = (HPEN) SelectObject((HDC) hdc, pen);
		for(int y = 0; y < borders.bottom.width.val(); y++)
		{
			MoveToEx((HDC) hdc, draw_pos.left(), draw_pos.bottom() - y - 1, NULL);
			LineTo((HDC) hdc, draw_pos.right(), draw_pos.bottom() - y - 1);
		}
		SelectObject((HDC) hdc, oldPen);
		DeleteObject(pen);
	}
}
