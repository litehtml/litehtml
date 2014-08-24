#pragma once
#include <windows.h>

namespace simpledib
{
	class dib
	{
		HBITMAP		m_bmp;
		HBITMAP		m_oldBmp;
		HDC			m_hdc;
		LPRGBQUAD	m_bits;
		BOOL		m_ownData;
		int			m_width;
		int			m_height;

		HDC			m_hTargetDC;
		POINT		m_oldViewPort;
		BOOL		m_restore_view_port;
		RECT		m_rcTarget;
	public:
		dib();
		~dib();

		int			width()		const	{ return m_width;	} 
		int			height()	const	{ return m_height;	} 
		HDC			hdc()		const	{ return m_hdc;		}
		HBITMAP		bmp()		const	{ return m_bmp;		}
		LPRGBQUAD	bits()		const	{ return m_bits;	}
		
		bool		create(int width, int height, bool topdowndib = false);
		bool		create(HDC hdc, HBITMAP bmp, LPRGBQUAD bits, int width, int height);

		void		clear();
		void		destroy(bool del_bmp = true);
		void		draw(HDC hdc, int x, int y);
		void		draw(HDC hdc, LPRECT rcDraw);
		HDC			beginPaint(HDC hdc, LPRECT rcDraw);
		void		endPaint(bool copy = false);
		HBITMAP		detach_bitmap();

		operator HDC()	{ return m_hdc; }
	};

}