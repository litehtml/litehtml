#include <windows.h>
#include <gdiplus.h>
#include "gdiplus_container.h"
#pragma comment(lib, "gdiplus.lib")

gdiplus_container::gdiplus_container()
{
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);
}

gdiplus_container::~gdiplus_container()
{
	clear_images();
	Gdiplus::GdiplusShutdown(m_gdiplusToken);
}

void gdiplus_container::draw_ellipse( HDC hdc, int x, int y, int width, int height, const litehtml::web_color& color, int line_width )
{
	Gdiplus::Graphics graphics(hdc);
	Gdiplus::LinearGradientBrush* brush = NULL;

	graphics.SetCompositingQuality(Gdiplus::CompositingQualityHighQuality);
	graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

	Gdiplus::Pen pen( Gdiplus::Color(color.alpha, color.red, color.green, color.blue) );
	graphics.DrawEllipse(&pen, x, y, width, height);
}

void gdiplus_container::fill_ellipse( HDC hdc, int x, int y, int width, int height, const litehtml::web_color& color )
{
	Gdiplus::Graphics graphics(hdc);

	graphics.SetCompositingQuality(Gdiplus::CompositingQualityHighQuality);
	graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

	Gdiplus::SolidBrush brush( Gdiplus::Color(color.alpha, color.red, color.green, color.blue) );
	graphics.FillEllipse(&brush, x, y, width, height);
}

void gdiplus_container::fill_rect( HDC hdc, int x, int y, int width, int height, const litehtml::web_color& color )
{
	Gdiplus::Graphics graphics(hdc);

	Gdiplus::SolidBrush brush( Gdiplus::Color(color.alpha, color.red, color.green, color.blue) );
	graphics.FillRectangle(&brush, x, y, width, height);
}

void gdiplus_container::get_img_size( uint_ptr img, litehtml::size& sz )
{
	Gdiplus::Bitmap* bmp = (Gdiplus::Bitmap*) img;
	if(bmp)
	{
		sz.width  = bmp->GetWidth();
		sz.height = bmp->GetHeight();
	}
}

void gdiplus_container::free_image( uint_ptr img )
{
	Gdiplus::Bitmap* bmp = (Gdiplus::Bitmap*) img;
	delete bmp;
}

void gdiplus_container::draw_img_bg( HDC hdc, uint_ptr img, const litehtml::background_paint& bg )
{
	Gdiplus::Bitmap* bgbmp = (Gdiplus::Bitmap*) img;

	Gdiplus::Graphics graphics(hdc);
	graphics.SetInterpolationMode(Gdiplus::InterpolationModeNearestNeighbor);
	graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);

	Gdiplus::Region reg(Gdiplus::Rect(bg.border_box.left(), bg.border_box.top(), bg.border_box.width, bg.border_box.height));
	graphics.SetClip(&reg);

	Gdiplus::Bitmap* scaled_img = nullptr;
	if (bg.image_size.width != bgbmp->GetWidth() || bg.image_size.height != bgbmp->GetHeight())
	{
		scaled_img = new Gdiplus::Bitmap(bg.image_size.width, bg.image_size.height);
		Gdiplus::Graphics gr(scaled_img);
		gr.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
		gr.DrawImage(bgbmp, 0, 0, bg.image_size.width, bg.image_size.height);
		bgbmp = scaled_img;
	}

	switch(bg.repeat)
	{
	case litehtml::background_repeat_no_repeat:
		{
			graphics.DrawImage(bgbmp, bg.position_x, bg.position_y, bgbmp->GetWidth(), bgbmp->GetHeight());
		}
		break;
	case litehtml::background_repeat_repeat_x:
		{
			Gdiplus::CachedBitmap bmp(bgbmp, &graphics);
			int x = bg.position_x;
			while(x > bg.clip_box.left()) x -= bgbmp->GetWidth();
			for(; x < bg.clip_box.right(); x += bgbmp->GetWidth())
			{
				graphics.DrawCachedBitmap(&bmp, x, bg.position_y);
			}
		}
		break;
	case litehtml::background_repeat_repeat_y:
		{
			Gdiplus::CachedBitmap bmp(bgbmp, &graphics);
			int y = bg.position_y;
			while(y > bg.clip_box.top()) y -= bgbmp->GetHeight();
			for(; y < bg.clip_box.bottom(); y += bgbmp->GetHeight())
			{
				graphics.DrawCachedBitmap(&bmp, bg.position_x, y);
			}
		}
		break;
	case litehtml::background_repeat_repeat:
		{
			Gdiplus::CachedBitmap bmp(bgbmp, &graphics);
			int x = bg.position_x;
			while(x > bg.clip_box.left()) x -= bgbmp->GetWidth();
			int y0 = bg.position_y;
			while(y0 > bg.clip_box.top()) y0 -= bgbmp->GetHeight();

			for(; x < bg.clip_box.right(); x += bgbmp->GetWidth())
			{
				for(int y = y0; y < bg.clip_box.bottom(); y += bgbmp->GetHeight())
				{
					graphics.DrawCachedBitmap(&bmp, x, y);
				}
			}
		}
		break;
	}

	delete scaled_img;
}

void gdiplus_container::draw_borders( uint_ptr hdc, const litehtml::borders& borders, const litehtml::position& draw_pos, bool root )
{
	apply_clip((HDC) hdc);

	// draw left border
	if(borders.left.width != 0 && borders.left.style > litehtml::border_style_hidden)
	{
		HPEN pen = CreatePen(PS_SOLID, 1, RGB(borders.left.color.red, borders.left.color.green, borders.left.color.blue));
		HPEN oldPen = (HPEN) SelectObject((HDC) hdc, pen);
		for(int x = 0; x < borders.left.width; x++)
		{
			MoveToEx((HDC) hdc, draw_pos.left() + x, draw_pos.top(), NULL);
			LineTo((HDC) hdc, draw_pos.left() + x, draw_pos.bottom());
		}
		SelectObject((HDC) hdc, oldPen);
		DeleteObject(pen);
	}
	// draw right border
	if(borders.right.width != 0 && borders.right.style > litehtml::border_style_hidden)
	{
		HPEN pen = CreatePen(PS_SOLID, 1, RGB(borders.right.color.red, borders.right.color.green, borders.right.color.blue));
		HPEN oldPen = (HPEN) SelectObject((HDC) hdc, pen);
		for(int x = 0; x < borders.right.width; x++)
		{
			MoveToEx((HDC) hdc, draw_pos.right() - x - 1, draw_pos.top(), NULL);
			LineTo((HDC) hdc, draw_pos.right() - x - 1, draw_pos.bottom());
		}
		SelectObject((HDC) hdc, oldPen);
		DeleteObject(pen);
	}
	// draw top border
	if(borders.top.width != 0 && borders.top.style > litehtml::border_style_hidden)
	{
		HPEN pen = CreatePen(PS_SOLID, 1, RGB(borders.top.color.red, borders.top.color.green, borders.top.color.blue));
		HPEN oldPen = (HPEN) SelectObject((HDC) hdc, pen);
		for(int y = 0; y < borders.top.width; y++)
		{
			MoveToEx((HDC) hdc, draw_pos.left(), draw_pos.top() + y, NULL);
			LineTo((HDC) hdc, draw_pos.right(), draw_pos.top() + y);
		}
		SelectObject((HDC) hdc, oldPen);
		DeleteObject(pen);
	}
	// draw bottom border
	if(borders.bottom.width != 0 && borders.bottom.style > litehtml::border_style_hidden)
	{
		HPEN pen = CreatePen(PS_SOLID, 1, RGB(borders.bottom.color.red, borders.bottom.color.green, borders.bottom.color.blue));
		HPEN oldPen = (HPEN) SelectObject((HDC) hdc, pen);
		for(int y = 0; y < borders.bottom.width; y++)
		{
			MoveToEx((HDC) hdc, draw_pos.left(), draw_pos.bottom() - y - 1, NULL);
			LineTo((HDC) hdc, draw_pos.right(), draw_pos.bottom() - y - 1);
		}
		SelectObject((HDC) hdc, oldPen);
		DeleteObject(pen);
	}

	release_clip((HDC) hdc);
}
