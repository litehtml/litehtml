#ifndef LH_UTF8_STRINGS_H
#define LH_UTF8_STRINGS_H

#include "os_types.h"
#include "types.h"

namespace litehtml
{
	// converts UTF-32 ch to UTF-8 and appends it to str
	void append_char(string& str, char32_t ch);

	class utf8_to_utf32
	{
		const byte* m_utf8;
		std::u32string m_str;
	public:
		utf8_to_utf32(const char* val);
		operator const char32_t*() const
		{
			return m_str.c_str();
		}
	private:
		char32_t getb()
		{
			if (!(*m_utf8)) return 0;
			return *m_utf8++;
		}
		char32_t get_next_utf8(char32_t val)
		{
			return (val & 0x3f);
		}
		char32_t get_char();
	};

	class utf32_to_utf8
	{
		std::string m_str;
	public:
		utf32_to_utf8(const std::u32string& val);
		operator const char*() const
		{
			return m_str.c_str();
		}

		const char* c_str() const
		{
			return m_str.c_str();
		}
	};

#define litehtml_from_utf32(str)	litehtml::utf32_to_utf8(str)
#define litehtml_to_utf32(str)		litehtml::utf8_to_utf32(str)
}

#endif  // LH_UTF8_STRINGS_H
