#include "dib.h"

simpledib::dib::dib()
{
	m_bmp		= NULL;
	m_oldBmp	= NULL;
	m_hdc		= NULL;
	m_bits		= NULL;
	m_ownData	= FALSE;
	m_width		= 0;
	m_height	= 0;
	m_hTargetDC	= NULL;
	m_restore_view_port = FALSE;
}

simpledib::dib::~dib()
{
	destroy();
}

void simpledib::dib::destroy(bool del_bmp)
{
	if(m_hdc && m_ownData)
	{
		SelectObject(m_hdc, m_oldBmp);
		if(del_bmp)
		{
			DeleteObject(m_bmp);
		}
		DeleteDC(m_hdc);
	} else if(m_restore_view_port && m_hdc)
	{
		SetWindowOrgEx(m_hdc, m_oldViewPort.x, m_oldViewPort.y, NULL);
	}

	m_bmp		= NULL;
	m_oldBmp	= NULL;
	m_hdc		= NULL;
	m_bits		= NULL;
	m_ownData	= FALSE;
	m_width		= 0;
	m_height	= 0;
}

bool simpledib::dib::create( int width, int height, bool topdowndib /*= false*/ )
{
	destroy();

	BITMAPINFO bmp_info; 
	bmp_info.bmiHeader.biSize			= sizeof(BITMAPINFOHEADER); 
	bmp_info.bmiHeader.biWidth			= width; 
	bmp_info.bmiHeader.biHeight			= height * (topdowndib ? -1 : 1); 
	bmp_info.bmiHeader.biPlanes			= 1; 
	bmp_info.bmiHeader.biBitCount		= 32; 
	bmp_info.bmiHeader.biCompression	= BI_RGB; 
	bmp_info.bmiHeader.biSizeImage		= 0; 
	bmp_info.bmiHeader.biXPelsPerMeter	= 0; 
	bmp_info.bmiHeader.biYPelsPerMeter	= 0; 
	bmp_info.bmiHeader.biClrUsed		= 0; 
	bmp_info.bmiHeader.biClrImportant	= 0; 

	m_hdc = CreateCompatibleDC(NULL); 

	m_bmp = ::CreateDIBSection( m_hdc, &bmp_info, DIB_RGB_COLORS, (LPVOID*) &m_bits, 0, 0 );
	if(m_bits)
	{
		m_oldBmp = (HBITMAP)::SelectObject(m_hdc, m_bmp);
	} else
	{
		DeleteDC(m_hdc);
		m_hdc = NULL;
	}

	if(m_hdc)
	{
		m_width		= width;
		m_height	= height;
		m_ownData	= TRUE;
		return true;
	}

	return false;
}

/*
bool simpledib::dib::create( HDC hdc, HBITMAP bmp, LPRGBQUAD bits, int width, int height, int shift_x, int shift_y )
{
	destroy();

	m_bmp		= bmp;
	m_hdc		= hdc;
	m_bits		= bits;
	m_width		= width;
	m_height	= height;
	m_ownData	= FALSE;

	SetWindowOrgEx(m_hdc, -shift_x, -shift_y, &m_oldViewPort);
	m_restore_view_port = TRUE;
	return true;
}

*/
bool simpledib::dib::create( HDC hdc, HBITMAP bmp, LPRGBQUAD bits, int width, int height )
{
	destroy();

	m_bmp				= bmp;
	m_hdc				= hdc;
	m_bits				= bits;
	m_width				= width;
	m_height			= height;
	m_ownData			= FALSE;
	m_restore_view_port = FALSE;
	return true;
}

void simpledib::dib::clear()
{
	if(m_bits)
	{
		ZeroMemory(m_bits, m_width * m_height * 4);
	}
}

void simpledib::dib::draw( HDC hdc, int x, int y )
{
	BLENDFUNCTION bf;
	bf.BlendOp = AC_SRC_OVER;
	bf.BlendFlags = 0;
	bf.AlphaFormat = AC_SRC_ALPHA;
	bf.SourceConstantAlpha = 255;

	AlphaBlend(hdc, x, y, m_width, m_height, m_hdc, 0, 0, m_width, m_height, bf);
}

void simpledib::dib::draw( HDC hdc, LPRECT rcDraw )
{
	BLENDFUNCTION bf;
	bf.BlendOp = AC_SRC_OVER;
	bf.BlendFlags = 0;
	bf.AlphaFormat = AC_SRC_ALPHA;
	bf.SourceConstantAlpha = 255;

	AlphaBlend(hdc, 
		rcDraw->left, rcDraw->top,
		rcDraw->right - rcDraw->left,
		rcDraw->bottom - rcDraw->top, m_hdc,

		rcDraw->left, rcDraw->top,
		rcDraw->right - rcDraw->left,
		rcDraw->bottom - rcDraw->top,
		bf);
}

HDC simpledib::dib::beginPaint( HDC hdc, LPRECT rcDraw )
{
	if(create(rcDraw->right - rcDraw->left, rcDraw->bottom - rcDraw->top, true))
	{
		m_hTargetDC = hdc;
		m_rcTarget	= *rcDraw;

		SetWindowOrgEx(m_hdc, rcDraw->left, rcDraw->top, &m_oldViewPort);

		return m_hdc;
	}
	return NULL;
}

void simpledib::dib::endPaint(bool copy)
{
	BOOL draw = TRUE;

	SetWindowOrgEx(m_hdc, m_oldViewPort.x, m_oldViewPort.y, NULL);

	if(!copy)
	{
		BLENDFUNCTION bf;
		bf.BlendOp = AC_SRC_OVER;
		bf.BlendFlags = 0;
		bf.AlphaFormat = AC_SRC_ALPHA;
		bf.SourceConstantAlpha = 255;

		AlphaBlend(m_hTargetDC, m_rcTarget.left, m_rcTarget.top,
			m_rcTarget.right - m_rcTarget.left,
			m_rcTarget.bottom - m_rcTarget.top, m_hdc,
			0, 0,
			m_rcTarget.right	- m_rcTarget.left,
			m_rcTarget.bottom	- m_rcTarget.top,
			bf);
	} else
	{
		BitBlt(m_hTargetDC, m_rcTarget.left, m_rcTarget.top,
			m_rcTarget.right - m_rcTarget.left,
			m_rcTarget.bottom - m_rcTarget.top, m_hdc, 0, 0, SRCCOPY);
	}

	m_hTargetDC	= NULL;
	destroy();
}

HBITMAP simpledib::dib::detach_bitmap()
{
	HBITMAP bmp = m_bmp;
	destroy(false);
	return bmp;
}
