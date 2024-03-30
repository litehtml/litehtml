#include "html.h"
#include "document_container.h"

void litehtml::document_container::split_text(const char* text, const std::function<void(const char*)>& on_word, const std::function<void(const char*)>& on_space)
{
	std::u32string str;
	std::u32string str_in = (const char32_t*)utf8_to_utf32(text);
	ucode_t c;
	for (size_t i = 0; i < str_in.length(); i++)
	{
		c = (ucode_t)str_in[i];
		if (c <= ' ' && (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f'))
		{
			if (!str.empty())
			{
				on_word(utf32_to_utf8(str));
				str.clear();
			}
			str += (wchar_t)c;
			on_space(utf32_to_utf8(str));
			str.clear();
		}
		// CJK character range
		else if (c >= 0x4E00 && c <= 0x9FCC)
		{
			if (!str.empty())
			{
				on_word(utf32_to_utf8(str));
				str.clear();
			}
			str += (wchar_t)c;
			on_word(utf32_to_utf8(str));
			str.clear();
		}
		else
		{
			str += (wchar_t)c;
		}
	}
	if (!str.empty())
	{
		on_word(utf32_to_utf8(str));
	}
}
