#pragma once
#include "..\win32\win32_container.h"

class gdiplus_container : public win32_container
{
public:
	gdiplus_container();
	virtual ~gdiplus_container();

private:
	ULONG_PTR m_gdiplusToken;

protected:
	// win32_container members
	virtual void	draw_ellipse(HDC hdc, int x, int y, int width, int height, const litehtml::web_color& color, int line_width) override;
	virtual void	fill_ellipse(HDC hdc, int x, int y, int width, int height, const litehtml::web_color& color) override;
	virtual void	fill_rect(HDC hdc, int x, int y, int width, int height, const litehtml::web_color& color) override;
	virtual void	get_img_size(uint_ptr img, litehtml::size& sz) override;
	virtual void	free_image(uint_ptr img) override;
	virtual void	draw_img_bg(HDC hdc, uint_ptr img, const litehtml::background_paint& bg) override;
	// litehtml::document_container members
	virtual void	draw_borders(uint_ptr hdc, const litehtml::borders& borders, const litehtml::position& draw_pos, bool root) override;
};
