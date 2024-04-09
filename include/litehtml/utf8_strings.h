#ifndef LH_UTF8_STRINGS_H
#define LH_UTF8_STRINGS_H

#include "os_types.h"
#include "types.h"

namespace litehtml
{
	ucode_t read_utf8_char(const string& str, int& index);
	void prev_utf8_char(const string& str, int& index);
	void append_char(string& str, int ch);

	class utf8_to_wchar
	{
		std::wstring m_str;
	public:
		utf8_to_wchar(const std::string& val);
		operator const wchar_t*() const
		{
			return m_str.c_str();
		}
	};

	class wchar_to_utf8
	{
		std::string m_str;
	public:
		wchar_to_utf8(const std::wstring& val);
		operator const char*() const
		{
			return m_str.c_str();
		}
		const char* c_str() const
		{
			return m_str.c_str();
		}
	};

#define litehtml_from_wchar(str)	litehtml::wchar_to_utf8(str)
#define litehtml_to_wchar(str)		litehtml::utf8_to_wchar(str)
}

#endif  // LH_UTF8_STRINGS_H
