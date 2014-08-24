#pragma once

#include <Uxtheme.h>
#include <math.h>
#include <vector>
#include <memory>

#define JPEG_QUALITY_SUPER		0
#define JPEG_QUALITY_GOOD		1
#define JPEG_QUALITY_NORMAL		2
#define JPEG_QUALITY_AVERAGE	3
#define JPEG_QUALITY_BAD		4

class CTxDIB
{
	LPRGBQUAD	m_bits;
	int			m_width;
	int			m_height;
	BYTE		m_maxAlpha;
public:
	CTxDIB(void);
	CTxDIB(LPCWSTR fileName);
	CTxDIB(CTxDIB& val);
	virtual ~CTxDIB(void);

	void operator=(CTxDIB& val);

	BOOL	load(LPCWSTR fileName);
	BOOL	load(HRSRC hRes, HMODULE hModule = NULL);
	BOOL	load(LPBYTE data, DWORD size);
	BOOL	savePNG(LPCWSTR fileName);
	BOOL	saveJPG(LPCWSTR fileName, int quality = JPEG_QUALITY_GOOD);
	BOOL	saveBMP(LPCWSTR fileName);

	BOOL	destroy();
	BOOL	draw(HDC hdc, int x, int y, int cx = -1, long cy = -1);
	BOOL	draw(HDC hdc, LPCRECT rcDraw);
	BOOL	createFromHBITMAP(HBITMAP bmp);
	BOOL	createFromHICON(HICON ico);

	HBITMAP createBitmap( HDC hdc = NULL );
	void	setTransColor(COLORREF clr);
	void	resample( int newWidth, int newHeight, CTxDIB* dst = NULL );
	void	tile(HDC hdc, LPRECT rcDraw, LPRECT rcClip = NULL);
	int		getWidth();
	int		getHeight();
	void	crop(int left, int top, int right, int bottom, CTxDIB* dst = NULL);
	void	crop(LPCRECT rcCrop, CTxDIB* dst = NULL);
	BOOL	isValid();
	void	setMaxAlpha(BYTE alpha);
	BYTE	getMaxAlpha();
	void	colorize(COLORREF clr);
	BOOL	calcAlpha(CTxDIB* imgWhite, CTxDIB* imgBlack);
	LPRGBQUAD	getBits() { return m_bits; }
	void	PreMulRGBA(RGBQUAD& color);
	void	rotateLeft(CTxDIB* dst = NULL);
	void	rotateRight(CTxDIB* dst = NULL);
	void	_copy( LPRGBQUAD newBits, int newWidth, int newHeight, BOOL copyBits = FALSE );
	BOOL	attach( LPVOID dib );
	void	PreMultiplyWithAlpha();
private:
	void	OverflowCoordinates(float &x, float &y);
	RGBQUAD GetPixelColorWithOverflow(long x, long y);
	RGBQUAD GetAreaColorInterpolated(float const xc, float const yc, float const w, float const h);
	void	AddAveragingCont(RGBQUAD const &color, float const surf, float &rr, float &gg, float &bb, float &aa);
	RGBQUAD GetPixelColorInterpolated(float x,float y);
	void	resample2( int newWidth, int newHeight, CTxDIB* dst = NULL );
	bool	QIShrink( int newWidth, int newHeight, CTxDIB* dst = NULL );

	void	_copy( CTxDIB& val );
};

class CTxDibSet
{
	typedef std::vector<CTxDIB*>	imgCols;
	
	std::vector<imgCols>	m_items;
	int						m_width;
	int						m_height;
	int						m_cols;
	int						m_rows;
public:
	CTxDibSet(CTxDIB* img, int rows, int cols);
	~CTxDibSet();
	CTxDIB* get(int col, int row) { return m_items[row][col]; }


	int width()		{ return m_width;	}
	int height()	{ return m_height;	}
	int rows()		{ return m_rows;	}
	int cols()		{ return m_cols;	}
};

class CTxSkinDIB
{
	CTxDIB	m_dibLeftTop;
	CTxDIB	m_dibTop;
	CTxDIB	m_dibRightTop;

	CTxDIB	m_dibLeftCenter;
	CTxDIB	m_dibCenter;
	CTxDIB	m_dibRightCenter;

	CTxDIB	m_dibLeftBottom;
	CTxDIB	m_dibBottom;
	CTxDIB	m_dibRightBottom;

	MARGINS	m_margins;
	BOOL	m_tileX;
	BOOL	m_tileY;
public:
	CTxSkinDIB();
	virtual ~CTxSkinDIB();

	BOOL load(LPCWSTR fileName, MARGINS* mg, BOOL tileX, BOOL tileY);
	BOOL load(CTxDIB* dib, MARGINS* mg, BOOL tileX, BOOL tileY);

	void draw(HDC hdc, LPRECT rcDraw, LPRECT rcClip);
};

//***bd*** simple floating point point
class CTxDibPoint2
{
public:
  CTxDibPoint2();
  CTxDibPoint2(float const x_, float const y_);
  CTxDibPoint2(CTxDibPoint2 const &p);

  float Distance(CTxDibPoint2 const p2);
  float Distance(float const x_, float const y_);

  float x,y;
};

//and simple rectangle
class CTxDibRect2
{
public:
  CTxDibRect2();
  CTxDibRect2(float const x1_, float const y1_, float const x2_, float const y2_);
  CTxDibRect2(CTxDibPoint2 const &bl, CTxDibPoint2 const &tr);
  CTxDibRect2(CTxDibRect2 const &p);

  float Surface() const;
  CTxDibRect2 CrossSection(CTxDibRect2 const &r2);
  CTxDibPoint2 Center();
  float Width();
  float Height();

  CTxDibPoint2 botLeft;
  CTxDibPoint2 topRight;
};


inline RGBQUAD CTxDIB::GetPixelColorWithOverflow(long x, long y)
{
	if (!(0 <= y && y < m_height && 0 <= x &&  x < m_width))
	{
		x = max(x, 0); 
		x = min(x, m_width - 1);
		y = max(y, 0); 
		y = min(y, m_height - 1);
	}
	return m_bits[y * m_width + x];
}

inline void CTxDIB::OverflowCoordinates( float &x, float &y )
{
	if (x >= 0 && x < m_width && y >= 0 && y < m_height) 
	{
		return;
	}
	x = max(x, 0); 
	x = min(x, m_width - 1);
	y = max(y, 0); 
	y = min(y, m_height - 1);
}

inline void	CTxDIB::AddAveragingCont(RGBQUAD const &color, float const surf, float &rr, float &gg, float &bb, float &aa)
{
	rr += color.rgbRed		* surf;
	gg += color.rgbGreen	* surf;
	bb += color.rgbBlue		* surf;
	aa += color.rgbReserved	* surf;
}


inline void CTxDIB::PreMulRGBA(RGBQUAD& color)
{
	color.rgbRed		= (color.rgbRed		* color.rgbReserved) / 255;
	color.rgbGreen		= (color.rgbGreen	* color.rgbReserved) / 255;
	color.rgbBlue		= (color.rgbBlue	* color.rgbReserved) / 255;
}

inline int CTxDIB::getWidth()	
{ 
	return m_width;  
}

inline int CTxDIB::getHeight()	
{ 
	return m_height; 
}


inline void CTxDIB::crop(LPCRECT rcCrop, CTxDIB* dst /*= NULL*/) 
{ 
	crop(rcCrop->left, rcCrop->top, rcCrop->right, rcCrop->bottom, dst); 
}

inline BOOL CTxDIB::isValid()	
{ 
	return m_bits ? TRUE : FALSE; 
}

inline void CTxDIB::setMaxAlpha(BYTE alpha)
{ 
	m_maxAlpha = alpha; 
}

inline BYTE CTxDIB::getMaxAlpha()
{ 
	return m_maxAlpha;
}

inline BOOL CTxDIB::draw(HDC hdc, LPCRECT rcDraw)
{ 
	return draw(hdc, rcDraw->left, rcDraw->top, rcDraw->right - rcDraw->left, rcDraw->bottom - rcDraw->top); 
}

inline void CTxDIB::operator=(CTxDIB& val) 
{ 
	_copy(val); 
}


inline CTxDibPoint2::CTxDibPoint2()
{
  x=y=0.0f;
}

inline CTxDibPoint2::CTxDibPoint2(float const x_, float const y_)
{
  x=x_;
  y=y_;
}

inline CTxDibPoint2::CTxDibPoint2(CTxDibPoint2 const &p)
{
  x=p.x;
  y=p.y;
}

inline float CTxDibPoint2::Distance(CTxDibPoint2 const p2)
{
  return (float)sqrt((x-p2.x)*(x-p2.x)+(y-p2.y)*(y-p2.y));
}

inline float CTxDibPoint2::Distance(float const x_, float const y_)
{
  return (float)sqrt((x-x_)*(x-x_)+(y-y_)*(y-y_));
}

inline CTxDibRect2::CTxDibRect2()
{
}

inline CTxDibRect2::CTxDibRect2(float const x1_, float const y1_, float const x2_, float const y2_)
{
  botLeft.x=x1_;
  botLeft.y=y1_;
  topRight.x=x2_;
  topRight.y=y2_;
}

inline CTxDibRect2::CTxDibRect2(CTxDibRect2 const &p)
{
  botLeft=p.botLeft;
  topRight=p.topRight;
}

inline float CTxDibRect2::Surface() const
/*
 * Returns the surface of rectangle.
 */
{
  return (topRight.x-botLeft.x)*(topRight.y-botLeft.y);
}

inline CTxDibRect2 CTxDibRect2::CrossSection(CTxDibRect2 const &r2)
{
  CTxDibRect2 cs;
  cs.botLeft.x=max(botLeft.x, r2.botLeft.x);
  cs.botLeft.y=max(botLeft.y, r2.botLeft.y);
  cs.topRight.x=min(topRight.x, r2.topRight.x);
  cs.topRight.y=min(topRight.y, r2.topRight.y);
  if (cs.botLeft.x<=cs.topRight.x && cs.botLeft.y<=cs.topRight.y) {
    return cs;
  } else {
    return CTxDibRect2(0,0,0,0);
  }//if
}

inline CTxDibPoint2 CTxDibRect2::Center()
{
  return CTxDibPoint2((topRight.x+botLeft.x)/2.0f, (topRight.y+botLeft.y)/2.0f);
}

inline float CTxDibRect2::Width()
{
  return topRight.x-botLeft.x;
}

inline float CTxDibRect2::Height()
{
  return topRight.y-botLeft.y;
}

