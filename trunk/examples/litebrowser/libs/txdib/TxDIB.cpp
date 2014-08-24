#include <Windows.h>
#include "TxDIB.h"
#include <emmintrin.h>
#include <mmintrin.h>
#include "..\freeimage\FreeImage.h"


CTxDIB::CTxDIB(void)
{
	m_maxAlpha	= 255;
	m_bits		= NULL;
	m_width		= 0;
	m_height	= 0;
}

CTxDIB::CTxDIB( CTxDIB& val )
{
	m_bits		= NULL;
	m_width		= 0;
	m_height	= 0;
	m_maxAlpha	= 255;
	_copy(val);
}

CTxDIB::CTxDIB( LPCWSTR fileName )
{
	m_bits		= NULL;
	m_width		= 0;
	m_height	= 0;
	m_maxAlpha	= 255;
	load(fileName);
}

CTxDIB::~CTxDIB(void)
{
	destroy();
}

BOOL CTxDIB::load( LPCWSTR fileName )
{
	BOOL ret = FALSE;

	FREE_IMAGE_FORMAT fif = FreeImage_GetFileTypeU(fileName);
	FIBITMAP* dib = FreeImage_LoadU(fif, fileName);
	ret = attach(dib);
	PreMultiplyWithAlpha();
	FreeImage_Unload(dib);

	return ret;
}

BOOL CTxDIB::load( HRSRC hRes, HMODULE hModule /*= NULL*/ )
{
	BOOL	ret		= FALSE;
	DWORD	rsize	= SizeofResource(hModule, hRes);
	HGLOBAL hMem	= LoadResource(hModule, hRes);
	if (hMem)
	{
		LPBYTE lpData = (LPBYTE) LockResource(hMem);
		ret = load(lpData, rsize);
	} 
	return ret;
}

BOOL CTxDIB::load( LPBYTE data, DWORD size )
{
	BOOL ret = FALSE;

	FIMEMORY* mem = FreeImage_OpenMemory(data, size);
	if(mem)
	{
		FREE_IMAGE_FORMAT fif = FreeImage_GetFileTypeFromMemory(mem, size);
		FIBITMAP* dib = FreeImage_LoadFromMemory(fif, mem);
		ret = attach(dib);
		PreMultiplyWithAlpha();
		FreeImage_Unload(dib);
		FreeImage_CloseMemory(mem);
	}
	
	return ret;
}

BOOL CTxDIB::destroy()
{
	if(m_bits)
	{
		free(m_bits);
		m_bits = NULL;
	}
	m_width		= 0;
	m_height	= 0;
	return TRUE;
}

BOOL CTxDIB::draw( HDC hdc, int x, int y, int cx /*= -1*/, long cy /*= -1*/ )
{
	if(!m_bits || !cx || !cy)
	{
		return FALSE;
	}

	if(cx > 0 && cx != m_width || cy > 0 && cy != m_height)
	{
		CTxDIB newdib;
		resample(cx, cy, &newdib);
		return newdib.draw(hdc, x, y);
	}

	HDC		memDC	= CreateCompatibleDC(hdc);
	HBITMAP bmp		= createBitmap(memDC);
	HBITMAP oldBmp	= (HBITMAP) SelectObject(memDC, bmp);

	BLENDFUNCTION bf;
	bf.BlendOp				= AC_SRC_OVER;
	bf.BlendFlags			= 0;
	bf.AlphaFormat			= AC_SRC_ALPHA;
	bf.SourceConstantAlpha	= m_maxAlpha;

	AlphaBlend(hdc, x, y,
		cx >= 0 ? cx : m_width,
		cy >= 0 ? cy : m_height,
		memDC,
		0, 0,
		m_width,
		m_height,
		bf);

	SelectObject(memDC, oldBmp);
	DeleteObject(bmp);
	DeleteDC(memDC);

	return TRUE;
}

void CTxDIB::setTransColor( COLORREF clr )
{
	RGBQUAD rgb;
	rgb.rgbRed		= GetBValue(clr);
	rgb.rgbGreen	= GetGValue(clr);
	rgb.rgbBlue		= GetBValue(clr);
	if(m_bits)
	{
		size_t cnt = m_width * m_height;
		for(size_t i = 0; i < cnt; i++)
		{
			if(m_bits[i].rgbRed == rgb.rgbRed && m_bits[i].rgbGreen == rgb.rgbGreen && m_bits[i].rgbBlue == rgb.rgbBlue)
			{
				m_bits[i].rgbReserved = 0;
				PreMulRGBA(m_bits[i]);
			}
		}
	}
}

void CTxDIB::resample( int newWidth, int newHeight, CTxDIB* dst )
{
	if(newHeight <= 0 || newWidth <= 0 || !isValid())
	{
		return;
	}
	if(newWidth < m_width && newHeight < m_height)
	{
		QIShrink(newWidth, newHeight, dst);
	} else
	{
		resample2(newWidth, newHeight, dst);
	}
}

RGBQUAD CTxDIB::GetPixelColorInterpolated( float x, float y )
{
	float sX, sY;         //source location

	WORD wt1, wt2, wd, wb, wc, wa;
	WORD wrr, wgg, wbb, waa;
	int xi, yi;

	float t1, t2, d, b, c, a;
	RGBQUAD rgb11,rgb21,rgb12,rgb22;

	RGBQUAD color;

	LPRGBQUAD pxptr;

	yi = (int) y; if (y < 0) yi--;
	xi = (int) x; if (x < 0) xi--;

	wt2 = (WORD) ((x - yi) * 256.0f);
	if (xi < -1 || xi >= m_width || yi < -1 || yi >= m_height) 
	{	
		//recalculate coordinates and use faster method later on
		OverflowCoordinates(sX, sY);
		xi = (int) sX; if (sX<0) xi--;   //sX and/or sY have changed ... recalculate xi and yi
		yi = (int) sY; if (sY<0) yi--;
		wt2 = (WORD) ((sY - yi) * 256.0f);
	}
	//get four neighbouring pixels
	if ((xi + 1) < m_width && xi >= 0 && (yi + 1) < m_height && yi >= 0) 
	{
		//all pixels are inside RGB24 image... optimize reading (and use fixed point arithmetic)
		wt1 = (WORD) ((sX - xi) * 256.0f);
		wd  = wt1 * wt2 >> 8;
		wb  = wt1 - wd;
		wc  = wt2 - wd;
		wa  = 256 - wt1 - wc;

		pxptr = m_bits + yi * m_width + xi;
		wbb = wa * pxptr->rgbBlue; 
		wgg = wa * pxptr->rgbGreen; 
		wrr = wa * pxptr->rgbRed;
		waa = wa * pxptr->rgbReserved;
		pxptr++;
		wbb += wb * pxptr->rgbBlue; 
		wgg += wb * pxptr->rgbGreen; 
		wrr += wb * pxptr->rgbRed;
		waa += wb * pxptr->rgbReserved;
		pxptr += m_width - 1;
		wbb += wc * pxptr->rgbBlue; 
		wgg += wc * pxptr->rgbGreen; 
		wrr += wc * pxptr->rgbRed;
		waa += wc * pxptr->rgbReserved;
		pxptr++;
		wbb += wd * pxptr->rgbBlue; 
		wgg += wd * pxptr->rgbGreen; 
		wrr += wd * pxptr->rgbRed;
		waa += wd * pxptr->rgbReserved;

		color.rgbRed		= (BYTE) (wrr >> 8); 
		color.rgbGreen		= (BYTE) (wgg >> 8); 
		color.rgbBlue		= (BYTE) (wbb >> 8);
		color.rgbReserved	= (BYTE) (waa >> 8);
	} else 
	{
		//default (slower) way to get pixels (not RGB24 or some pixels out of borders)
		t1 = sX  - xi;
		t2 = sY  - yi;
		d  = t1 * t2;
		b  = t1 - d;
		c  = t2 - d;
		a  = 1 - t1 - c;
		rgb11 = GetPixelColorWithOverflow(xi,		yi);
		rgb21 = GetPixelColorWithOverflow(xi + 1,	yi);
		rgb12 = GetPixelColorWithOverflow(xi,		yi + 1);
		rgb22 = GetPixelColorWithOverflow(xi+1,		yi + 1);
		//calculate linear interpolation
		color.rgbRed		= (BYTE) (a * rgb11.rgbRed		+ b * rgb21.rgbRed		+ c * rgb12.rgbRed		+ d * rgb22.rgbRed);
		color.rgbGreen		= (BYTE) (a * rgb11.rgbGreen	+ b * rgb21.rgbGreen	+ c * rgb12.rgbGreen	+ d * rgb22.rgbGreen);
		color.rgbBlue		= (BYTE) (a * rgb11.rgbBlue		+ b * rgb21.rgbBlue		+ c * rgb12.rgbBlue		+ d * rgb22.rgbBlue);
		color.rgbReserved	= (BYTE) (a * rgb11.rgbReserved	+ b * rgb21.rgbReserved + c * rgb12.rgbReserved	+ d * rgb22.rgbReserved);
	}
	return color;
}



void CTxDIB::PreMultiplyWithAlpha()
{
	if(!isValid())
	{
		return;
	}

	int cnt = m_width * m_height;
	for(int i=0; i < cnt; i++)
	{
		PreMulRGBA(m_bits[i]);
	}
}

void CTxDIB::_copy( CTxDIB& val )
{
	_copy(val.m_bits, val.m_width, val.m_height, TRUE);
	m_maxAlpha = val.m_maxAlpha;
}

void CTxDIB::crop( int left, int top, int right, int bottom, CTxDIB* dst /*= NULL*/ )
{
	if(!isValid())
	{
		return;
	}

	left	= max(0, min(left,		m_width));
	top		= max(0, min(top,		m_height));
	right	= max(0, min(right,		m_width));
	bottom	= max(0, min(bottom,	m_height));

	int newWidth	= right - left;
	int newHeight	= bottom - top;

	if(newWidth <= 0 || newHeight <= 0)
	{
		if(dst)
		{
			dst->destroy();
		}
		return;
	}

	LPRGBQUAD newBits = (LPRGBQUAD) malloc(newWidth * newHeight * sizeof(RGBQUAD));

	int startSrc = (m_height - bottom) * m_width + left;
	int startDst = 0;
	for(int i = 0; i < newHeight; i++, startSrc += m_width, startDst += newWidth)
	{
		memcpy(newBits + startDst, m_bits + startSrc, newWidth * sizeof(RGBQUAD));
	}

	if(dst)
	{
		dst->_copy(newBits, newWidth, newHeight);
		dst->m_maxAlpha = m_maxAlpha;
	} else
	{
		_copy(newBits, newWidth, newHeight);
	}
}

void CTxDIB::_copy( LPRGBQUAD newBits, int newWidth, int newHeight, BOOL copyBits )
{
	destroy();
	m_width		= newWidth;
	m_height	= newHeight;
	if(!copyBits)
	{
		m_bits		= newBits;
	} else
	{
		if(newBits)
		{
			size_t sz	= m_width * m_height * sizeof(RGBQUAD);
			m_bits		= (LPRGBQUAD) malloc(sz);
			memcpy(m_bits, newBits, sz);
		} else
		{
			newBits = NULL;
		}
	}
}

void CTxDIB::tile( HDC hdc, LPRECT rcDraw, LPRECT rcClip /*= NULL*/ )
{
	if(!m_width || !m_height || !m_bits)
	{
		return;
	}
	BLENDFUNCTION bf;
	bf.BlendOp				= AC_SRC_OVER;
	bf.BlendFlags			= 0;
	bf.AlphaFormat			= AC_SRC_ALPHA;
	bf.SourceConstantAlpha	= 255;

	int x = 0;
	int y = 0;

	HBITMAP bmp		= createBitmap(hdc);
	HDC		memDC	= CreateCompatibleDC(hdc);
	HBITMAP	oldBmp	= (HBITMAP) SelectObject(memDC, bmp);
	RECT	rcDst;
	RECT	rcTemp;

	int drawWidth	= 0;
	int drawHeight	= 0;
	int imgX		= 0;
	int imgY		= 0;

	for(int top = rcDraw->top; top <= rcDraw->bottom; top += m_height)
	{
		for(int left = rcDraw->left; left <= rcDraw->right; left += m_width)
		{
			rcDst.left		= left;
			rcDst.top		= top;
			rcDst.right		= left + m_width;
			rcDst.bottom	= top  + m_height;
			if(!rcClip || rcClip && IntersectRect(&rcTemp, &rcDst, rcClip))
			{
				imgX		= 0;
				imgY		= 0;
				drawWidth	= m_width;
				drawHeight	= m_height;
				if(rcClip)
				{
					imgY		= rcTemp.top  - rcDst.top;
					imgX		= rcTemp.left - rcDst.left;
					drawWidth	= rcTemp.right - rcTemp.left;
					drawHeight	= rcTemp.bottom - rcTemp.top;
					rcDst = rcTemp;
				}
				drawWidth	-= (rcDst.right	- rcDraw->right)  <= 0 ? 0 : (rcDst.right	- rcDraw->right);
				drawHeight	-= (rcDst.bottom	- rcDraw->bottom) <= 0 ? 0 : (rcDst.bottom	- rcDraw->bottom);

				AlphaBlend(hdc, rcDst.left, rcDst.top, drawWidth, drawHeight, memDC, imgX, imgY, drawWidth, drawHeight, bf);
			}
		}
	}
	SelectObject(memDC, oldBmp);
	DeleteObject(bmp);
	DeleteDC(memDC);
}

HBITMAP CTxDIB::createBitmap( HDC hdc )
{
	HBITMAP bmp = NULL;

	BITMAPINFO bmp_info = {0}; 
	bmp_info.bmiHeader.biSize			= sizeof(BITMAPINFOHEADER); 
	bmp_info.bmiHeader.biWidth			= m_width; 
	bmp_info.bmiHeader.biHeight			= m_height; 
	bmp_info.bmiHeader.biPlanes			= 1; 
	bmp_info.bmiHeader.biBitCount		= 32; 

	HDC dc = hdc;
	if(!dc)
	{
		dc = GetDC(NULL);
	}

	void* buf = 0; 
	bmp = ::CreateDIBSection( 
		hdc, 
		&bmp_info, 
		DIB_RGB_COLORS, 
		&buf, 
		0, 
		0 
		); 
	memcpy(buf, m_bits, m_width * m_height * sizeof(RGBQUAD));
	if(!hdc)
	{
		ReleaseDC(NULL, dc);
	}
	return bmp;
}

BOOL CTxDIB::createFromHBITMAP( HBITMAP bmp )
{
	FIBITMAP *dib = NULL;
	if(bmp) 
	{
		BITMAP bm;
		GetObject(bmp, sizeof(BITMAP), (LPSTR) &bm);
		dib = FreeImage_Allocate(bm.bmWidth, bm.bmHeight, bm.bmBitsPixel);
		int nColors = FreeImage_GetColorsUsed(dib);
		HDC dc = GetDC(NULL);
		int res = GetDIBits(dc, bmp, 0, FreeImage_GetHeight(dib), FreeImage_GetBits(dib), FreeImage_GetInfo(dib), DIB_RGB_COLORS);
		ReleaseDC(NULL, dc);
		FreeImage_GetInfoHeader(dib)->biClrUsed = nColors;
		FreeImage_GetInfoHeader(dib)->biClrImportant = nColors;
		attach(dib);
		FreeImage_Unload(dib);
		return TRUE;
	}
	return FALSE;
}

BOOL CTxDIB::attach( LPVOID pdib )
{
	destroy();

	FIBITMAP* dib = (FIBITMAP*) pdib;
	if(dib)
	{
		BOOL delDIB = FALSE;;
		if(FreeImage_GetBPP(dib) != 32)
		{
			FIBITMAP* dib32 = FreeImage_ConvertTo32Bits(dib);
			if(dib32)
			{
				dib = dib32;
				delDIB = TRUE;
			} else
			{
				dib = NULL;
			}
		}
		if(dib)
		{
			BITMAPINFO* hdr = FreeImage_GetInfo(dib);
			m_width		= FreeImage_GetWidth(dib);
			m_height	= FreeImage_GetHeight(dib);
			m_bits		= (LPRGBQUAD) malloc(m_width * m_height * sizeof(RGBQUAD));
			memcpy(m_bits, FreeImage_GetBits(dib), m_width * m_height * sizeof(RGBQUAD));
			if(delDIB)
			{
				FreeImage_Unload(dib);
			}
			return TRUE;
		}
	}
	return FALSE;
}

BOOL CTxDIB::createFromHICON( HICON ico )
{
	destroy();
	BOOL ret = FALSE;
	if (ico) 
	{ 
		ICONINFO iinfo;
		GetIconInfo(ico,	&iinfo);
		if(iinfo.hbmColor)
		{
			if(createFromHBITMAP(iinfo.hbmColor))
			{
				CTxDIB mask;
				if(mask.createFromHBITMAP(iinfo.hbmMask))
				{
					int cnt = m_width * m_height;
					for(int i=0; i < cnt; i++)
					{
						if(mask.m_bits[i].rgbBlue)
						{
							m_bits[i].rgbReserved	= 0;
							m_bits[i].rgbRed		= 0;
							m_bits[i].rgbGreen		= 0;
							m_bits[i].rgbBlue		= 0;
						} else if(!m_bits[i].rgbReserved)
						{
							m_bits[i].rgbReserved = 255;
						}
						PreMulRGBA(m_bits[i]);
					}
				}
				ret = TRUE;
			}
		} else if(iinfo.hbmMask)
		{
			BITMAP bm;
			GetObject(iinfo.hbmMask, sizeof(BITMAP), (LPSTR) &bm);
			if(bm.bmWidth * 2 == bm.bmHeight)
			{
				m_width		= bm.bmWidth;
				m_height	= bm.bmHeight / 2;
				m_bits		= (LPRGBQUAD) malloc(m_width * m_height * sizeof(RGBQUAD));
				ZeroMemory(m_bits, m_width * m_height * sizeof(RGBQUAD));
				CTxDIB dib;
				dib.createFromHBITMAP(iinfo.hbmMask);
				dib.crop(0, m_width, m_width, m_height, NULL);
				int cnt = m_width * m_height;
				for(int i=0; i < cnt; i++)
				{
					if(dib.m_bits[i].rgbBlue)
					{
						m_bits[i].rgbReserved	= 255;
						m_bits[i].rgbRed		= 0;
						m_bits[i].rgbGreen		= 0;
						m_bits[i].rgbBlue		= 0;
					} else
					{
						m_bits[i].rgbReserved	= 0;
						m_bits[i].rgbRed		= 0;
						m_bits[i].rgbGreen		= 0;
						m_bits[i].rgbBlue		= 0;
					}
				}
				ret = TRUE;
			}
		}

		if(iinfo.hbmColor)	DeleteObject(iinfo.hbmColor);
		if(iinfo.hbmMask)	DeleteObject(iinfo.hbmMask);
	}
	return ret;
}

void CTxDIB::colorize( COLORREF clr )
{
	if(m_bits)
	{
		BYTE r = GetRValue(clr);
		BYTE g = GetGValue(clr);
		BYTE b = GetBValue(clr);
		int cnt = m_width * m_height;
		for(int i=0; i < cnt; i++)
		{
			if(m_bits[i].rgbReserved)
			{
				m_bits[i].rgbRed	 = r;
				m_bits[i].rgbGreen	 = g;
				m_bits[i].rgbBlue	 = b;
				PreMulRGBA(m_bits[i]);
			}
		}
	}
}

RGBQUAD CTxDIB::GetAreaColorInterpolated( float const xc, float const yc, float const w, float const h )
{
	RGBQUAD color;      //calculated colour

	//area is wider and/or taller than one pixel:
	CTxDibRect2 area(xc-w/2.0f, yc-h/2.0f, xc+w/2.0f, yc+h/2.0f);   //area
	int xi1=(int)(area.botLeft.x+0.49999999f);                //low x
	int yi1=(int)(area.botLeft.y+0.49999999f);                //low y


	int xi2=(int)(area.topRight.x+0.5f);                      //top x
	int yi2=(int)(area.topRight.y+0.5f);                      //top y (for loops)

	float rr,gg,bb,aa;                                        //red, green, blue and alpha components
	rr=gg=bb=aa=0;
	int x,y;                                                  //loop counters
	float s=0;                                                //surface of all pixels
	float cps;                                                //surface of current crosssection
	if (h>1 && w>1) {
		//width and height of area are greater than one pixel, so we can employ "ordinary" averaging
		CTxDibRect2 intBL, intTR;     //bottom left and top right intersection
		intBL=area.CrossSection(CTxDibRect2(((float)xi1)-0.5f, ((float)yi1)-0.5f, ((float)xi1)+0.5f, ((float)yi1)+0.5f));
		intTR=area.CrossSection(CTxDibRect2(((float)xi2)-0.5f, ((float)yi2)-0.5f, ((float)xi2)+0.5f, ((float)yi2)+0.5f));
		float wBL, wTR, hBL, hTR;
		wBL=intBL.Width();            //width of bottom left pixel-area intersection
		hBL=intBL.Height();           //height of bottom left...
		wTR=intTR.Width();            //width of top right...
		hTR=intTR.Height();           //height of top right...

		AddAveragingCont(GetPixelColorWithOverflow(xi1,yi1), wBL*hBL, rr, gg, bb, aa);    //bottom left pixel
		AddAveragingCont(GetPixelColorWithOverflow(xi2,yi1), wTR*hBL, rr, gg, bb, aa);    //bottom right pixel
		AddAveragingCont(GetPixelColorWithOverflow(xi1,yi2), wBL*hTR, rr, gg, bb, aa);    //top left pixel
		AddAveragingCont(GetPixelColorWithOverflow(xi2,yi2), wTR*hTR, rr, gg, bb, aa);    //top right pixel
		//bottom and top row
		for (x=xi1+1; x<xi2; x++) {
			AddAveragingCont(GetPixelColorWithOverflow(x,yi1), hBL, rr, gg, bb, aa);    //bottom row
			AddAveragingCont(GetPixelColorWithOverflow(x,yi2), hTR, rr, gg, bb, aa);    //top row
		}
		//leftmost and rightmost column
		for (y=yi1+1; y<yi2; y++) {
			AddAveragingCont(GetPixelColorWithOverflow(xi1,y), wBL, rr, gg, bb, aa);    //left column
			AddAveragingCont(GetPixelColorWithOverflow(xi2,y), wTR, rr, gg, bb, aa);    //right column
		}
		for (y=yi1+1; y<yi2; y++) 
		{
			for (x=xi1+1; x<xi2; x++) 
			{ 
				color=GetPixelColorWithOverflow(x,y);
				rr+=color.rgbRed;
				gg+=color.rgbGreen;
				bb+=color.rgbBlue;
				aa+=color.rgbReserved;
			}//for x
		}//for y
	} else 
	{
		//width or height greater than one:
		CTxDibRect2 intersect;                                          //intersection with current pixel
		CTxDibPoint2 center;
		for (y=yi1; y<=yi2; y++) 
		{
			for (x=xi1; x<=xi2; x++) 
			{
				intersect = area.CrossSection(CTxDibRect2(((float)x)-0.5f, ((float)y)-0.5f, ((float)x)+0.5f, ((float)y)+0.5f));
				center = intersect.Center();
				color=GetPixelColorInterpolated(center.x, center.y);
				cps=intersect.Surface();
				rr+=color.rgbRed*cps;
				gg+=color.rgbGreen*cps;
				bb+=color.rgbBlue*cps;
				aa+=color.rgbReserved*cps;
			}//for x
		}//for y      
	}//if

	s=area.Surface();
	rr/=s; gg/=s; bb/=s; aa/=s;
	if (rr>255) rr=255; if (rr<0) rr=0; color.rgbRed=(BYTE) rr;
	if (gg>255) gg=255; if (gg<0) gg=0; color.rgbGreen=(BYTE) gg;
	if (bb>255) bb=255; if (bb<0) bb=0; color.rgbBlue=(BYTE) bb;
	if (aa>255) aa=255; if (aa<0) aa=0; color.rgbReserved=(BYTE) aa;
	return color;
}

typedef long fixed;												// Our new fixed point type
#define itofx(x) ((x) << 8)										// Integer to fixed point
#define ftofx(x) (long)((x) * 256)								// Float to fixed point
#define dtofx(x) (long)((x) * 256)								// Double to fixed point
#define fxtoi(x) ((x) >> 8)										// Fixed point to integer
#define fxtof(x) ((float) (x) / 256)							// Fixed point to float
#define fxtod(x) ((double)(x) / 256)							// Fixed point to double
#define Mulfx(x,y) (((x) * (y)) >> 8)							// Multiply a fixed by a fixed
#define Divfx(x,y) (((x) << 8) / (y))							// Divide a fixed by a fixed
#define _PIXEL	DWORD											// Pixel

void CTxDIB::resample2( int newWidth, int newHeight, CTxDIB* dst /*= NULL */ )
{
	// Check for valid bitmap
	if (isValid())
	{
		// Calculate scaling params
		long _width		= max(1, newWidth);
		long _height	= max(1, newHeight);
		float dx = (float)m_width  / (float)_width;
		float dy = (float)m_height / (float)_height;
		fixed f_dx = ftofx(dx);
		fixed f_dy = ftofx(dy);
		fixed f_1 = itofx(1);

		LPRGBQUAD lpData = (LPRGBQUAD) malloc(newWidth * newHeight * sizeof(RGBQUAD));

		// Scale bitmap
		DWORD dwDstHorizontalOffset;
		DWORD dwDstVerticalOffset = 0;
		DWORD dwDstTotalOffset;
		LPRGBQUAD lpSrcData = m_bits;
		DWORD dwSrcTotalOffset;
		LPRGBQUAD lpDstData = lpData;
		for (long i = 0; i < _height; i++)
		{
			dwDstHorizontalOffset = 0;
			for (long j = 0; j < _width; j++)
			{
				// Update destination total offset
				dwDstTotalOffset = dwDstVerticalOffset + dwDstHorizontalOffset;

				// Update bitmap
				fixed f_i = itofx(i);
				fixed f_j = itofx(j);
				fixed f_a = Mulfx(f_i, f_dy);
				fixed f_b = Mulfx(f_j, f_dx);
				long m = fxtoi(f_a);
				long n = fxtoi(f_b);
				fixed f_f = f_a - itofx(m);
				fixed f_g = f_b - itofx(n);
				dwSrcTotalOffset = m * m_width + n;
				DWORD dwSrcTopLeft = dwSrcTotalOffset;
				DWORD dwSrcTopRight = dwSrcTotalOffset + 1;
				if (n >= m_width-1)
					dwSrcTopRight = dwSrcTotalOffset;
				DWORD dwSrcBottomLeft = dwSrcTotalOffset + m_width;
				if (m >= m_height-1)
					dwSrcBottomLeft = dwSrcTotalOffset;
				DWORD dwSrcBottomRight = dwSrcTotalOffset + m_width + 1;
				if ((n >= m_width-1) || (m >= m_height-1))
					dwSrcBottomRight = dwSrcTotalOffset;
				fixed f_w1 = Mulfx(f_1-f_f, f_1-f_g);
				fixed f_w2 = Mulfx(f_1-f_f, f_g);
				fixed f_w3 = Mulfx(f_f, f_1-f_g);
				fixed f_w4 = Mulfx(f_f, f_g);
				RGBQUAD pixel1 = lpSrcData[dwSrcTopLeft];
				RGBQUAD pixel2 = lpSrcData[dwSrcTopRight];
				RGBQUAD pixel3 = lpSrcData[dwSrcBottomLeft];
				RGBQUAD pixel4 = lpSrcData[dwSrcBottomRight];
				fixed f_r1 = itofx(pixel1.rgbRed);
				fixed f_r2 = itofx(pixel2.rgbRed);
				fixed f_r3 = itofx(pixel3.rgbRed);
				fixed f_r4 = itofx(pixel4.rgbRed);
				fixed f_g1 = itofx(pixel1.rgbGreen);
				fixed f_g2 = itofx(pixel2.rgbGreen);
				fixed f_g3 = itofx(pixel3.rgbGreen);
				fixed f_g4 = itofx(pixel4.rgbGreen);
				fixed f_b1 = itofx(pixel1.rgbBlue);
				fixed f_b2 = itofx(pixel2.rgbBlue);
				fixed f_b3 = itofx(pixel3.rgbBlue);
				fixed f_b4 = itofx(pixel4.rgbBlue);
				fixed f_a1 = itofx(pixel1.rgbReserved);
				fixed f_a2 = itofx(pixel2.rgbReserved);
				fixed f_a3 = itofx(pixel3.rgbReserved);
				fixed f_a4 = itofx(pixel4.rgbReserved);
				lpDstData[dwDstTotalOffset].rgbRed	= (BYTE)fxtoi(Mulfx(f_w1, f_r1) + Mulfx(f_w2, f_r2) + Mulfx(f_w3, f_r3) + Mulfx(f_w4, f_r4));
				lpDstData[dwDstTotalOffset].rgbGreen = (BYTE)fxtoi(Mulfx(f_w1, f_g1) + Mulfx(f_w2, f_g2) + Mulfx(f_w3, f_g3) + Mulfx(f_w4, f_g4));
				lpDstData[dwDstTotalOffset].rgbBlue	= (BYTE)fxtoi(Mulfx(f_w1, f_b1) + Mulfx(f_w2, f_b2) + Mulfx(f_w3, f_b3) + Mulfx(f_w4, f_b4));
				lpDstData[dwDstTotalOffset].rgbReserved = (BYTE)fxtoi(Mulfx(f_w1, f_a1) + Mulfx(f_w2, f_a2) + Mulfx(f_w3, f_a3) + Mulfx(f_w4, f_a4));

				// Update destination horizontal offset
				dwDstHorizontalOffset ++;
			}

			dwDstVerticalOffset += _width;
		}

		if(dst)
		{
			dst->_copy(lpData, newWidth, newHeight);
			dst->m_maxAlpha = m_maxAlpha;
		} else
		{
			_copy(lpData, newWidth, newHeight);
		}
	}
}

bool CTxDIB::QIShrink( int newWidth, int newHeight, CTxDIB* dst /*= NULL */ )
{
	if (!isValid()) return false;

	if (newWidth > m_width || newHeight > m_height) 
	{ 
		return false;
	}

	if (newWidth==m_width && newHeight==m_height) 
	{
		if(dst)
		{
			*dst = *this;
		}
		return true;
	}

	LPRGBQUAD newBits = (LPRGBQUAD) malloc(newWidth * newHeight * sizeof(RGBQUAD));

	const int oldx = m_width;
	const int oldy = m_height;

	int accuCellSize = 5;

	unsigned int *accu = new unsigned int[newWidth*accuCellSize];      //array for summing pixels... one pixel for every destination column
	unsigned int *accuPtr;                              //pointer for walking through accu
	//each cell consists of blue, red, green component and count of pixels summed in this cell
	memset(accu, 0, newWidth * accuCellSize * sizeof(unsigned int));  //clear accu

	//RGB24 version with pointers
	LPRGBQUAD destPtr, srcPtr, destPtrS, srcPtrS;        //destination and source pixel, and beginnings of current row
	srcPtrS = m_bits;
	destPtrS = newBits;
	int ex=0, ey=0;                                               //ex and ey replace division... 
	int dy=0;
	//(we just add pixels, until by adding newWidth or newHeight we get a number greater than old size... then
	// it's time to move to next pixel)

	for(int y=0; y<oldy; y++)
	{                                    //for all source rows
		ey += newHeight;                                                   
		ex = 0;                                                       //restart with ex = 0
		accuPtr=accu;                                                 //restart from beginning of accu
		srcPtr=srcPtrS;                                               //and from new source line

		for(int x = 0; x < oldx; x++)
		{                                    //for all source columns
			ex += newWidth;
			accuPtr[0]  += srcPtr[x].rgbRed;                                  //add current pixel to current accu slot
			accuPtr[1]	+= srcPtr[x].rgbGreen;
			accuPtr[2]	+= srcPtr[x].rgbBlue;
			accuPtr[4]	+= srcPtr[x].rgbReserved;
			accuPtr[3]++;
			if (ex > oldx) 
			{                                                //when we reach oldx, it's time to move to new slot
				accuPtr += accuCellSize;
				ex -= oldx;                                                   //(substract oldx from ex and resume from there on)
			}//if (ex overflow)
		}//for x

		if (ey >= oldy) 
		{                                                 //now when this happens
			ey -= oldy;                                                     //it's time to move to new destination row
			destPtr = destPtrS;                                             //reset pointers to proper initial values
			accuPtr = accu;
			for (int k = 0; k < newWidth; k++) 
			{                                    //copy accu to destination row (divided by number of pixels in each slot)
				destPtr[k].rgbRed		= (BYTE)(accuPtr[0] / accuPtr[3]);
				destPtr[k].rgbGreen		= (BYTE)(accuPtr[1] / accuPtr[3]);
				destPtr[k].rgbBlue		= (BYTE)(accuPtr[2] / accuPtr[3]);
				destPtr[k].rgbReserved	= (BYTE)(accuPtr[4] / accuPtr[3]);
				accuPtr += accuCellSize;
			}//for k
			memset(accu, 0, newWidth * accuCellSize * sizeof(unsigned int));                   //clear accu
			destPtrS += newWidth;
		}//if (ey overflow)

		srcPtrS += m_width;                                     //next round we start from new source row
	}//for y

	delete [] accu;                                                 //delete helper array

	//copy new image to the destination
	if(dst)
	{
		dst->_copy(newBits, newWidth, newHeight);
		dst->m_maxAlpha = m_maxAlpha;
	} else
	{
		_copy(newBits, newWidth, newHeight);
	}
	return true;
}

BOOL CTxDIB::savePNG( LPCWSTR fileName )
{
	FIBITMAP* dib = FreeImage_Allocate(m_width, m_height, 32);
	memcpy(FreeImage_GetBits(dib), m_bits, m_width * m_height * 4);
	BOOL ret = FreeImage_SaveU(FIF_PNG, dib, fileName);
	FreeImage_Unload(dib);
	return ret;
}

BOOL CTxDIB::saveJPG( LPCWSTR fileName, int quality /*= JPEG_QUALITY_GOOD*/ )
{
	FIBITMAP* dib = FreeImage_Allocate(m_width, m_height, 32);
	memcpy(FreeImage_GetBits(dib), m_bits, m_width * m_height * 4);
	FIBITMAP* dib24 = FreeImage_ConvertTo24Bits(dib);
	
	int flags = JPEG_QUALITYGOOD;
	switch(quality)
	{
	case JPEG_QUALITY_SUPER:
		flags = JPEG_QUALITYSUPERB;
		break;
	case JPEG_QUALITY_GOOD:
		flags = JPEG_QUALITYGOOD;
		break;
	case JPEG_QUALITY_NORMAL:
		flags = JPEG_QUALITYNORMAL;
		break;
	case JPEG_QUALITY_AVERAGE:
		flags = JPEG_QUALITYAVERAGE;
		break;
	case JPEG_QUALITY_BAD:
		flags = JPEG_QUALITYBAD;
		break;
	}

	BOOL ret = FreeImage_SaveU(FIF_JPEG, dib24, fileName, flags);
	FreeImage_Unload(dib);
	FreeImage_Unload(dib24);
	return ret;
}

BOOL CTxDIB::saveBMP( LPCWSTR fileName )
{
	FIBITMAP* dib = FreeImage_Allocate(m_width, m_height, 32);
	memcpy(FreeImage_GetBits(dib), m_bits, m_width * m_height * 4);
	BOOL ret = FreeImage_SaveU(FIF_BMP, dib, fileName, BMP_SAVE_RLE);
	FreeImage_Unload(dib);
	return ret;
}

BOOL CTxDIB::calcAlpha( CTxDIB* imgWhite, CTxDIB* imgBlack )
{
	if(!imgBlack || !imgWhite)	
	{
		return FALSE;
	}

	if(	imgWhite->getWidth()  != imgBlack->getWidth() ||
		imgWhite->getHeight() != imgBlack->getHeight())
	{
		return FALSE;
	}

	destroy();

	m_width		= imgBlack->getWidth();
	m_height	= imgBlack->getHeight();
	size_t cnt	= m_width * m_height;
	size_t sz	= cnt * sizeof(RGBQUAD);
	m_bits		= (LPRGBQUAD) malloc(sz);

	int alphaR, alphaG, alphaB, resultR, resultG, resultB;

	for(size_t i=0; i < cnt; i++)
	{
		alphaR = imgBlack->m_bits[i].rgbRed		- imgWhite->m_bits[i].rgbRed	+ 255;
		alphaG = imgBlack->m_bits[i].rgbGreen	- imgWhite->m_bits[i].rgbGreen	+ 255;
		alphaB = imgBlack->m_bits[i].rgbBlue	- imgWhite->m_bits[i].rgbBlue	+ 255;

		if (alphaG != 0)
		{
			resultR = imgBlack->m_bits[i].rgbRed	* 255 / alphaG;
			resultG = imgBlack->m_bits[i].rgbGreen	* 255 / alphaG;
			resultB = imgBlack->m_bits[i].rgbBlue	* 255 / alphaG;
		}
		else
		{
			resultR = 0;
			resultG = 0;
			resultB = 0;
		}
		
		m_bits[i].rgbReserved	= (BYTE)alphaG;
		m_bits[i].rgbRed		= (BYTE)resultR;
		m_bits[i].rgbGreen		= (BYTE)resultG;
		m_bits[i].rgbBlue		= (BYTE)resultB;
		PreMulRGBA(m_bits[i]);
	}

	return TRUE;
}

#define RBLOCK 96

void CTxDIB::rotateLeft( CTxDIB* dst /*= NULL*/ )
{
	if(!isValid()) return;

	int width	= getHeight();
	int height	= getWidth();

	size_t		sz			= width * height * sizeof(RGBQUAD);
	LPRGBQUAD	newBbits	= (LPRGBQUAD) malloc(sz);

	int xs, ys;                                   //x-segment and y-segment
	long x, x2, y;
	LPRGBQUAD srcPtr;
	LPRGBQUAD dstPtr;
	for (xs = 0; xs < width; xs += RBLOCK) 
	{       //for all image blocks of RBLOCK*RBLOCK pixels
		for (ys = 0; ys < height; ys += RBLOCK) 
		{
			//RGB24 optimized pixel access:
			for (x = xs; x < min(width, xs + RBLOCK); x++)
			{    //do rotation
				x2 = width - x - 1;
				dstPtr = newBbits + ys * width + x;
				srcPtr = m_bits + x2 * m_width + ys;
				for (y = ys; y < min(height, ys + RBLOCK); y++)
				{
					*dstPtr = *srcPtr;
					srcPtr++;
					dstPtr+= width;
				}//for y
			}//for x
		}//for ys
	}//for xs

	if(dst)
	{
		dst->_copy(newBbits, width, height, FALSE);
	} else
	{
		_copy(newBbits, width, height, FALSE);
	}
}

void CTxDIB::rotateRight( CTxDIB* dst /*= NULL*/ )
{
	if(!isValid()) return;

	int width	= getHeight();
	int height	= getWidth();

	size_t		sz			= width * height * sizeof(RGBQUAD);
	LPRGBQUAD	newBbits	= (LPRGBQUAD) malloc(sz);

	int xs, ys;                                   //x-segment and y-segment
	long x, y2, y;
	LPRGBQUAD srcPtr;
	LPRGBQUAD dstPtr;

	for (xs = 0; xs < width; xs += RBLOCK) 
	{       //for all image blocks of RBLOCK*RBLOCK pixels
		for (ys = 0; ys < height; ys += RBLOCK) 
		{
			//RGB24 optimized pixel access:
			for (y = ys; y < min(height, ys + RBLOCK); y++)
			{    //do rotation
				y2 = height - y - 1;
				dstPtr = newBbits + y * width + xs;
				srcPtr = m_bits + xs * m_width + y2;
				for (x = xs; x < min(width, xs + RBLOCK); x++)
				{
					*dstPtr = *srcPtr;
					dstPtr++;
					srcPtr += m_width;
				}//for y
			}//for x
		}//for ys
	}//for xs

	if(dst)
	{
		dst->_copy(newBbits, width, height, FALSE);
	} else
	{
		_copy(newBbits, width, height, FALSE);
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

CTxSkinDIB::CTxSkinDIB()
{
	ZeroMemory(&m_margins, sizeof(m_margins));
	m_tileX	= FALSE;
	m_tileY	= FALSE;
}

CTxSkinDIB::~CTxSkinDIB()
{

}

BOOL CTxSkinDIB::load( LPCWSTR fileName, MARGINS* mg, BOOL tileX, BOOL tileY )
{
	CTxDIB dib;
	if(dib.load(fileName))
	{
		return load(&dib, mg, tileX, tileY);
	}
	return FALSE;
}

BOOL CTxSkinDIB::load( CTxDIB* dib, MARGINS* mg, BOOL tileX, BOOL tileY )
{
	if(!dib)	return FALSE;

	m_margins	= *mg;
	m_tileX		= tileX;
	m_tileY		= tileY;

	if(m_margins.cxLeftWidth)
	{
		if(m_margins.cyTopHeight)
		{
			dib->crop(	0, 
						0, 
						m_margins.cxLeftWidth, 
						m_margins.cyTopHeight, 
						&m_dibLeftTop);
		}
		if(m_margins.cyBottomHeight)
		{
			dib->crop(	0, 
						dib->getHeight() - m_margins.cyBottomHeight, 
						m_margins.cxLeftWidth, 
						dib->getHeight(), 
						&m_dibLeftBottom);
		}
		dib->crop(	0, 
					m_margins.cyTopHeight, 
					m_margins.cxLeftWidth, 
					dib->getHeight() - m_margins.cyBottomHeight, 
					&m_dibLeftCenter);
	}

	if(m_margins.cxRightWidth)
	{
		if(m_margins.cyTopHeight)
		{
			dib->crop(	dib->getWidth() - m_margins.cxRightWidth, 
						0, 
						dib->getWidth(), 
						m_margins.cyTopHeight, 
						&m_dibRightTop);
		}
		if(m_margins.cyBottomHeight)
		{
			dib->crop(	dib->getWidth() - m_margins.cxRightWidth, 
						dib->getHeight() - m_margins.cyBottomHeight, 
						dib->getWidth(), 
						dib->getHeight(), 
						&m_dibRightBottom);
		}
		dib->crop(	dib->getWidth() - m_margins.cxRightWidth, 
					m_margins.cyTopHeight, 
					dib->getWidth(), 
					dib->getHeight() - m_margins.cyBottomHeight, 
					&m_dibRightCenter);
	}

	if(m_margins.cyTopHeight)
	{
		dib->crop(	m_margins.cxLeftWidth, 
					0, 
					dib->getWidth() - m_margins.cxRightWidth, 
					m_margins.cyTopHeight, 
					&m_dibTop);
	}

	if(m_margins.cyBottomHeight)
	{
		dib->crop(	m_margins.cxLeftWidth, 
					dib->getHeight() - m_margins.cyBottomHeight, 
					dib->getWidth() - m_margins.cxRightWidth, 
					dib->getHeight(), 
					&m_dibBottom);
	}

	dib->crop(	m_margins.cxLeftWidth,
				m_margins.cyTopHeight,
				dib->getWidth() - m_margins.cxRightWidth,
				dib->getHeight() - m_margins.cyBottomHeight,
				&m_dibCenter);

	return TRUE;
}

void CTxSkinDIB::draw( HDC hdc, LPRECT rcDraw, LPRECT rcClip )
{
	RECT rcPart;
	RECT rcTmp;
	if(m_margins.cxLeftWidth)
	{
		if(m_margins.cyTopHeight)
		{
			rcPart.left		= rcDraw->left;
			rcPart.right	= rcPart.left + m_margins.cxLeftWidth;
			rcPart.top		= rcDraw->top;
			rcPart.bottom	= rcPart.top + m_margins.cyTopHeight;
			if(!rcClip || rcClip && IntersectRect(&rcTmp, rcClip, &rcPart))
			{
				m_dibLeftTop.draw(hdc, rcPart.left, rcPart.top, rcPart.right - rcPart.left, rcPart.bottom - rcPart.top);
			}
		}

		if(m_margins.cyBottomHeight)
		{
			rcPart.left		= rcDraw->left;
			rcPart.right	= rcPart.left + m_margins.cxLeftWidth;
			rcPart.top		= rcDraw->bottom - m_margins.cyBottomHeight;
			rcPart.bottom	= rcPart.top + m_margins.cyBottomHeight;
			if(!rcClip || rcClip && IntersectRect(&rcTmp, rcClip, &rcPart))
			{
				m_dibLeftBottom.draw(hdc, rcPart.left, rcPart.top, rcPart.right - rcPart.left, rcPart.bottom - rcPart.top);
			}
		}

		rcPart.left		= rcDraw->left;
		rcPart.right	= rcPart.left + m_margins.cxLeftWidth;
		rcPart.top		= rcDraw->top + m_margins.cyTopHeight;
		rcPart.bottom	= rcDraw->bottom - m_margins.cyBottomHeight;
		if(!rcClip || rcClip && IntersectRect(&rcTmp, rcClip, &rcPart))
		{
			if(!m_tileY)
			{
				m_dibLeftCenter.draw(hdc, rcPart.left, rcPart.top, rcPart.right - rcPart.left, rcPart.bottom - rcPart.top);
			} else
			{
				m_dibLeftCenter.tile(hdc, &rcPart, rcClip);
			}
		}

	}

	if(m_margins.cxRightWidth)
	{
		if(m_margins.cyTopHeight)
		{
			rcPart.left		= rcDraw->right - m_margins.cxRightWidth;
			rcPart.right	= rcPart.left + m_margins.cxRightWidth;
			rcPart.top		= rcDraw->top;
			rcPart.bottom	= rcPart.top + m_margins.cyTopHeight;
			if(!rcClip || rcClip && IntersectRect(&rcTmp, rcClip, &rcPart))
			{
				m_dibRightTop.draw(hdc, rcPart.left, rcPart.top, rcPart.right - rcPart.left, rcPart.bottom - rcPart.top);
			}
		}

		if(m_margins.cyBottomHeight)
		{
			rcPart.left		= rcDraw->right - m_margins.cxRightWidth;
			rcPart.right	= rcPart.left + m_margins.cxRightWidth;
			rcPart.top		= rcDraw->bottom - m_margins.cyBottomHeight;
			rcPart.bottom	= rcPart.top + m_margins.cyBottomHeight;
			if(!rcClip || rcClip && IntersectRect(&rcTmp, rcClip, &rcPart))
			{
				m_dibRightBottom.draw(hdc, rcPart.left, rcPart.top, rcPart.right - rcPart.left, rcPart.bottom - rcPart.top);
			}
		}

		rcPart.left		= rcDraw->right - m_margins.cxRightWidth;
		rcPart.right	= rcPart.left + m_margins.cxRightWidth;
		rcPart.top		= rcDraw->top + m_margins.cyTopHeight;
		rcPart.bottom	= rcDraw->bottom - m_margins.cyBottomHeight;
		if(!rcClip || rcClip && IntersectRect(&rcTmp, rcClip, &rcPart))
		{
			if(!m_tileY)
			{
				m_dibRightCenter.draw(hdc, rcPart.left, rcPart.top, rcPart.right - rcPart.left, rcPart.bottom - rcPart.top);
			} else
			{
				m_dibRightCenter.tile(hdc, &rcPart, rcClip);
			}
		}

	}

	if(m_margins.cyTopHeight)
	{
		rcPart.left		= rcDraw->left + m_margins.cxLeftWidth;
		rcPart.right	= rcDraw->right - m_margins.cxRightWidth;
		rcPart.top		= rcDraw->top;
		rcPart.bottom	= rcDraw->top + m_margins.cyTopHeight;
		if(!rcClip || rcClip && IntersectRect(&rcTmp, rcClip, &rcPart))
		{
			if(!m_tileX)
			{
				m_dibTop.draw(hdc, rcPart.left, rcPart.top, rcPart.right - rcPart.left, rcPart.bottom - rcPart.top);
			} else
			{
				m_dibTop.tile(hdc, &rcPart, rcClip);
			}
		}
	}

	if(m_margins.cyBottomHeight)
	{
		rcPart.left		= rcDraw->left + m_margins.cxLeftWidth;
		rcPart.right	= rcDraw->right - m_margins.cxRightWidth;
		rcPart.top		= rcDraw->bottom - m_margins.cyBottomHeight;
		rcPart.bottom	= rcPart.top + m_margins.cyBottomHeight;
		if(!rcClip || rcClip && IntersectRect(&rcTmp, rcClip, &rcPart))
		{
			if(!m_tileX)
			{
				m_dibBottom.draw(hdc, rcPart.left, rcPart.top, rcPart.right - rcPart.left, rcPart.bottom - rcPart.top);
			} else
			{
				m_dibBottom.tile(hdc, &rcPart, rcClip);
			}
		}
	}

	rcPart.left		= rcDraw->left + m_margins.cxLeftWidth;
	rcPart.right	= rcDraw->right - m_margins.cxRightWidth;
	rcPart.top		= rcDraw->top + m_margins.cyTopHeight;
	rcPart.bottom	= rcDraw->bottom - m_margins.cyBottomHeight;
	if(!rcClip || rcClip && IntersectRect(&rcTmp, rcClip, &rcPart))
	{
		if(!m_tileY && !m_tileX)
		{
			m_dibCenter.draw(hdc, rcPart.left, rcPart.top, rcPart.right - rcPart.left, rcPart.bottom - rcPart.top);
		} else if(m_tileX && m_tileY)
		{
			m_dibCenter.tile(hdc, &rcPart, rcClip);
		} else if(m_tileX && !m_tileY)
		{
			CTxDIB dib;
			m_dibCenter.resample(m_dibCenter.getWidth(), rcPart.bottom - rcPart.top, &dib);
			dib.tile(hdc, &rcPart, rcClip);
		} else
		{
			CTxDIB dib;
			m_dibCenter.resample(rcPart.right - rcPart.left, m_dibCenter.getHeight(), &dib);
			dib.tile(hdc, &rcPart, rcClip);
		}
	}
}

//////////////////////////////////////////////////////////////////////////

CTxDibSet::CTxDibSet( CTxDIB* img, int rows, int cols )
{
	m_cols = cols;
	m_rows = rows;
	m_width		= img->getWidth() / cols;
	m_height	= img->getHeight() / rows;
	for(int row = 0; row < rows; row++)
	{
		imgCols vCols;
		for(int col = 0; col < cols; col++)
		{
			CTxDIB* frame = new CTxDIB;
			img->crop(col * m_width, row * m_height, (col + 1) * m_width, (row + 1) * m_height, frame);
			vCols.push_back(frame);
		}
		m_items.push_back(vCols);
	}
}

CTxDibSet::~CTxDibSet()
{
	for(size_t row = 0; row < m_items.size(); row++)
	{
		for(size_t col = 0; col < m_items[row].size(); col++)
		{
			delete m_items[row][col];
		}
	}
}
