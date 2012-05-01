#include "globals.h"
#include "memdc.h"

CMemDC::CMemDC()
{
	m_hBmp		= NULL;
	m_hOldBmp	= NULL;
	m_hMemDC	= NULL;
	m_alpha		= 255;
}

CMemDC::~CMemDC()
{
	if(m_hBmp) DeleteObject(m_hBmp);
}

HDC	CMemDC::beginPaint(HDC targetDC, LPRECT rcDraw)
{
	m_hTargetDC = targetDC;
	m_rcTarget	= *rcDraw;
	m_hMemDC	= CreateCompatibleDC(targetDC);
	m_hBmp		= CreateCompatibleBitmap(targetDC, rcDraw->right - rcDraw->left, rcDraw->bottom - rcDraw->top);
	m_hOldBmp	= (HBITMAP) SelectObject(m_hMemDC, m_hBmp);
	SetWindowOrgEx(m_hMemDC, rcDraw->left, rcDraw->top, &m_oldViewPort);

	return m_hMemDC;
}

void CMemDC::endPaint()
{
	BOOL draw = TRUE;

	SetWindowOrgEx(m_hMemDC, m_oldViewPort.x, m_oldViewPort.y, NULL);

/*
	if(m_alpha != 255)
	{
		BLENDFUNCTION bf;
		bf.BlendOp = AC_SRC_OVER;
		bf.BlendFlags = 0;
		bf.AlphaFormat = AC_SRC_ALPHA;
		bf.SourceConstantAlpha = m_alpha;

		if(AlphaBlend(m_hTargetDC, m_rcTarget.left, m_rcTarget.top,
			m_rcTarget.right - m_rcTarget.left,
			m_rcTarget.bottom - m_rcTarget.top, m_hMemDC,
			0, 0,
			m_rcTarget.right - m_rcTarget.left,
			m_rcTarget.bottom - m_rcTarget.top,
			bf))
		{
			draw = FALSE;
		}
	}
*/
	if(draw)
	{
		BitBlt(m_hTargetDC, m_rcTarget.left, m_rcTarget.top,
			m_rcTarget.right - m_rcTarget.left,
			m_rcTarget.bottom - m_rcTarget.top, m_hMemDC, 
			m_rcTarget.left - m_rcTarget.left,
			m_rcTarget.top - m_rcTarget.top, SRCCOPY);
	}

	SelectObject(m_hMemDC, m_hOldBmp);
	DeleteDC(m_hMemDC);
	DeleteObject(m_hBmp);

	m_hBmp		= NULL;
	m_hMemDC	= NULL;
}

void CMemDC::setAlpha( BYTE alpha, LPRECT rcAlpha )
{
	m_alpha = alpha;
}
