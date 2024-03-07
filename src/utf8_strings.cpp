#include "html.h"
#include "utf8_strings.h"

namespace litehtml
{

utf8_to_wchar::utf8_to_wchar(const char* val)
{
	m_utf8 = (const byte*) val;
	if (!m_utf8) return;

	while (true)
	{
		ucode_t wch = get_char();
		if (!wch) break;
		m_str += (wchar_t)wch;
	}
}

ucode_t utf8_to_wchar::get_char()
{
	ucode_t b1 = getb();

	if (!b1)
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
	}
	else if ((b1 & 0xe0) == 0xc0)
	{
		// 2-byte sequence: 00000yyyyyxxxxxx = 110yyyyy 10xxxxxx
		ucode_t r = (b1 & 0x1f) << 6;
		r |= get_next_utf8(getb());
		return r;
	}
	else if ((b1 & 0xf0) == 0xe0)
	{
		// 3-byte sequence: zzzzyyyyyyxxxxxx = 1110zzzz 10yyyyyy 10xxxxxx
		ucode_t r = (b1 & 0x0f) << 12;
		r |= get_next_utf8(getb()) << 6;
		r |= get_next_utf8(getb());
		return r;
	}
	else if ((b1 & 0xf8) == 0xf0)
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

void append_char(string& str, int code)
{
	if (code <= 0x7F)
	{
		str += (char)code;
	}
	else if (code <= 0x7FF)
	{
		str += char((code >> 6) + 192);
		str += (code & 63) + 128;
	}
	else if (0xd800 <= code && code <= 0xdfff)
	{
		// error: surrogate
	}
	else if (code <= 0xFFFF)
	{
		str += char((code >> 12) + 224);
		str += ((code >> 6) & 63) + 128;
		str += (code & 63) + 128;
	}
	else if (code <= 0x10FFFF)
	{
		str += char((code >> 18) + 240);
		str += ((code >> 12) & 63) + 128;
		str += ((code >> 6) & 63) + 128;
		str += (code & 63) + 128;
	}
}

wchar_to_utf8::wchar_to_utf8(const std::wstring& wstr)
{
	for (auto ch: wstr)
		append_char(m_str, ch);
}

} // namespace litehtml