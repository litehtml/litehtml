#include "html.h"
#include "types.h"
#include "html_tag.h"

void litehtml::trim(std::string &s)
{
	auto pos = s.find_first_not_of(_t(" \n\r\t"));
	if(pos != std::string::npos)
	{
		s.erase(s.begin(), s.begin() + pos);
	}
	pos = s.find_last_not_of(_t(" \n\r\t"));
	if(pos != std::string::npos)
	{
		s.erase(s.begin() + pos + 1, s.end());
	}
}

void litehtml::trim(std::wstring &s)
{
	auto pos = s.find_first_not_of(L" \n\r\t");
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

void litehtml::lcase(tstring &s)
{
	for(tstring::iterator i = s.begin(); i != s.end(); i++)
	{
		(*i) = t_tolower(*i);
	}
}

litehtml::tstring::size_type litehtml::find_close_bracket(const tstring &s, tstring::size_type off, tchar_t open_b, tchar_t close_b)
{
	int cnt = 0;
	for(tstring::size_type i = off; i < s.length(); i++)
	{
		if(s[i] == open_b)
		{
			cnt++;
		} else if(s[i] == close_b)
		{
			cnt--;
			if(!cnt)
			{
				return i;
			}
		}
	}
	return tstring::npos;
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
	tstring::size_type item_len		= 0;
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
			if( t_strncmp( val.c_str(), strings.c_str() + delim_start, item_len ) == 0 )
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
}

bool litehtml::value_in_list( const tstring& val, const tstring& strings, tchar_t delim )
{
	int idx = value_index(val, strings, -1, delim);
	if(idx >= 0)
	{
		return true;
	}
	return false;
}

void litehtml::join_string(tstring& str, const string_vector& tokens, const tstring& delims)
{
	std::stringstream ss;
	for(size_t i=0; i<tokens.size(); ++i)
	{
		if(i != 0)
		{
			ss << delims;
		}
		ss << tokens[i];
	}

	str = ss.str();
}
