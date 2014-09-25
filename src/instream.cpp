#include "html.h"
#include "instream.h"


litehtml::str_instream::str_instream( const tchar_t* src ) : m_str(src)
{
}

litehtml::ucode_t litehtml::str_instream::get_char()
{
	if(!(*m_str))
	{
		return 0;
	}
	return *m_str++;
}

void litehtml::str_instream::ucode_to_chars( ucode_t wch, tstring& dst )
{
	dst += (tchar_t) wch;
}

//////////////////////////////////////////////////////////////////////////

litehtml::utf8_instream::utf8_instream( const byte* src ) : m_str(src)
{

}

litehtml::ucode_t litehtml::utf8_instream::get_char()
{
	ucode_t b1 = getb();

	if(!b1)
	{
		return 0;
	}

	// Determine whether we are dealing
	// with a one-, two-, three-, or four-
	// byte sequence.
	if ((b1 & 0x80) == 0)
	{
		// 1-byte sequence: 000000000xxxxxxx = 0xxxxxxx
		return b1;
	} else if ((b1 & 0xe0) == 0xc0)
	{
		// 2-byte sequence: 00000yyyyyxxxxxx = 110yyyyy 10xxxxxx
		ucode_t r = (b1 & 0x1f) << 6;
		r |= get_next_utf8(getb());
		return r;
	} else if ((b1 & 0xf0) == 0xe0)
	{
		// 3-byte sequence: zzzzyyyyyyxxxxxx = 1110zzzz 10yyyyyy 10xxxxxx
		ucode_t r = (b1 & 0x0f) << 12;
		r |= get_next_utf8(getb()) << 6;
		r |= get_next_utf8(getb());
		return r;
	} else if ((b1 & 0xf8) == 0xf0)
	{
		// 4-byte sequence: 11101110wwwwzzzzyy + 110111yyyyxxxxxx
		//     = 11110uuu 10uuzzzz 10yyyyyy 10xxxxxx
		// (uuuuu = wwww + 1)
		int b2 = get_next_utf8(getb());
		int b3 = get_next_utf8(getb());
		int b4 = get_next_utf8(getb());
		return ((b1 & 7) << 18) | ((b2 & 0x3f) << 12) |
			((b3 & 0x3f) << 6) | (b4 & 0x3f);
	}

	//bad start for UTF-8 multi-byte sequence
	return '?';
}

void litehtml::utf8_instream::ucode_to_chars( ucode_t code, tstring& dst )
{
#ifdef LITEHTML_UTF8
	if (code <= 0x7F) 
	{
		dst += (tchar_t) code;
	} else if (code <= 0x7FF) 
	{
		dst += (code >> 6) + 192;
		dst += (code & 63) + 128;
	} else if (0xd800 <= code && code <= 0xdfff) 
	{
		//invalid block of utf8
	} else if (code <= 0xFFFF) 
	{
		dst += (code >> 12) + 224;
		dst += ((code >> 6) & 63) + 128;
		dst += (code & 63) + 128;
	} else if (code <= 0x10FFFF) 
	{
		dst += (code >> 18) + 240;
		dst += ((code >> 12) & 63) + 128;
		dst += ((code >> 6) & 63) + 128;
		dst += (code & 63) + 128;
	}
#else
	dst += (tchar_t) code;
#endif
}

