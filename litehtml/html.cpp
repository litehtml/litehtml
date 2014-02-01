#include "html.h"
#include "types.h"
#include "tokenizer.h"
#include "html_tag.h"

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

int litehtml::value_index( const tstring& val, const tstring& strings, int defValue, tchar_t delim )
{
	if(val.empty() || strings.empty() || !delim)
	{
		return defValue;
	}

	int idx = 0;
	tstring::size_type delim_start	= 0;
	tstring::size_type delim_end	= strings.find(delim, delim_start);
	tstring::size_type item_len = 0;
	while(true)
	{
		if(delim_end == tstring::npos)
		{
			item_len = strings.length() - delim_start;
		} else
		{
			item_len = delim_end - delim_start;
		}
		if(item_len == val.length())
		{
			if(val == strings.substr(delim_start, item_len))
			{
				return idx;
			}
		}
		idx++;
		delim_start = delim_end;
		if(delim_start == tstring::npos) break;
		delim_start++;
		if(delim_start == strings.length()) break;
		delim_end = strings.find(delim, delim_start);
	}
	return defValue;

/*
	string_vector tokens;
	tokenize(strings, tokens, delim);
	for(size_t i = 0; i < tokens.size(); i++)
	{
		if(!t_strcmp(tokens[i].c_str(), val))
		{
			return (int) i;
		}
	}
	return defValue;
*/
}

int litehtml::value_in_list( const tstring& val, const tstring& strings, tchar_t delim )
{
	int idx = value_index(val, strings, -1, delim);
	if(idx >= 0)
	{
		return true;
	}
	return false;
}

