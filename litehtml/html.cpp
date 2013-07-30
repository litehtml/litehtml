#include "html.h"
#include "types.h"
#include "tokenizer.h"
#include "element.h"

void litehtml::trim(tstring &s) 
{
	tstring::size_type pos = s.find_first_not_of(_t(" \n\r\t"));
	if(pos != tstring::npos)
	{
		s.erase(s.begin(), s.begin() + pos);
	}
	pos = s.find_last_not_of(_t(" \n\r\t"));
	if(pos != tstring::npos)
	{
		s.erase(s.begin() + pos + 1, s.end());
	}
}

void litehtml::lcase(tstring &s) 
{
	for(tstring::iterator i = s.begin(); i != s.end(); i++)
	{
		(*i) = t_tolower(*i);
	}
}

int litehtml::value_index( const tchar_t* val, const tchar_t* strings, int defValue, const tchar_t* delim )
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

int litehtml::value_in_list( const tchar_t* val, const tchar_t* strings, const tchar_t* delim )
{
	int idx = value_index(val, strings, -1, delim);
	if(idx >= 0)
	{
		return true;
	}
	return false;
}

