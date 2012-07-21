#include "html.h"
#include "types.h"
#include "tokenizer.h"
#include "element.h"

void litehtml::trim(std::wstring &s) 
{
	std::wstring::size_type pos = s.find_first_not_of(L" \n\r\t");
	if(pos != std::wstring::npos)
	{
		s.erase(s.begin(), s.begin() + pos);
	}
	pos = s.find_last_not_of(L" \n\r\t");
	if(pos != std::wstring::npos)
	{
		s.erase(s.begin() + pos + 1, s.end());
	}
}

int litehtml::value_index( const wchar_t* val, const wchar_t* strings, int defValue, const wchar_t* delim )
{
	if(!val || !strings || !delim)
	{
		return defValue;
	}
	string_vector tokens;
	tokenize(strings, tokens, delim);
	for(size_t i = 0; i < tokens.size(); i++)
	{
		if(tokens[i] == val)
		{
			return (int) i;
		}
	}
	return defValue;
}

int litehtml::value_in_list( const wchar_t* val, const wchar_t* strings, const wchar_t* delim )
{
	int idx = value_index(val, strings, -1, delim);
	if(idx >= 0)
	{
		return true;
	}
	return false;
}

