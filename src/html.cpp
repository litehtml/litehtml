#include "html.h"
#include "types.h"
#include "utf8_strings.h"

void litehtml::trim(string &s) 
{
	string::size_type pos = s.find_first_not_of(" \n\r\t");
	if(pos != string::npos)
	{
		s.erase(s.begin(), s.begin() + pos);
	}
	pos = s.find_last_not_of(" \n\r\t");
	if(pos != string::npos)
	{
		s.erase(s.begin() + pos + 1, s.end());
	}
}

void litehtml::lcase(string &s) 
{
	for(char & i : s)
	{
		i = t_tolower(i);
	}
}

litehtml::string::size_type litehtml::find_close_bracket(const string &s, string::size_type off, char open_b, char close_b)
{
	int cnt = 0;
	for(string::size_type i = off; i < s.length(); i++)
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
	return string::npos;
}

litehtml::string litehtml::index_value(int index, const string& strings, char delim)
{
	std::vector<string> vals;
	string delims;
	delims.push_back(delim);
	split_string(strings, vals, delims);
	if(index >= 0 && index < vals.size())
	{
		return vals[index];
	}
	return std::to_string(index);
}

int litehtml::value_index( const string& val, const string& strings, int defValue, char delim )
{
	if(val.empty() || strings.empty() || !delim)
	{
		return defValue;
	}

	int idx = 0;
	string::size_type delim_start	= 0;
	string::size_type delim_end	= strings.find(delim, delim_start);
	string::size_type item_len;
	while(true)
	{
		if(delim_end == string::npos)
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
		if(delim_start == string::npos) break;
		delim_start++;
		if(delim_start == strings.length()) break;
		delim_end = strings.find(delim, delim_start);
	}
	return defValue;
}

bool litehtml::value_in_list( const string& val, const string& strings, char delim )
{
	int idx = value_index(val, strings, -1, delim);
	if(idx >= 0)
	{
		return true;
	}
	return false;
}

void litehtml::split_string(const string& str, string_vector& tokens, const string& delims, const string& delims_preserve, const string& quote)
{
	if(str.empty() || (delims.empty() && delims_preserve.empty()))
	{
		return;
	}

	string all_delims = delims + delims_preserve + quote;

	string::size_type token_start	= 0;
	string::size_type token_end	= str.find_first_of(all_delims, token_start);
	string::size_type token_len;
	string token;
	while(true)
	{
		while( token_end != string::npos && quote.find_first_of(str[token_end]) != string::npos )
		{
			if(str[token_end] == '(')
			{
				token_end = find_close_bracket(str, token_end, '(', ')');
			} else if(str[token_end] == '[')
			{
				token_end = find_close_bracket(str, token_end, '[', ']');
			} else if(str[token_end] == '{')
			{
				token_end = find_close_bracket(str, token_end, '{', '}');
			} else
			{
				token_end = str.find_first_of(str[token_end], token_end + 1);
			}
			if(token_end != string::npos)
			{
				token_end = str.find_first_of(all_delims, token_end + 1);
			}
		}

		if(token_end == string::npos)
		{
			token_len = string::npos;
		} else
		{
			token_len = token_end - token_start;
		}

		token = str.substr(token_start, token_len);
		if(!token.empty())
		{
			tokens.push_back( token );
		}
		if(token_end != string::npos && !delims_preserve.empty() && delims_preserve.find_first_of(str[token_end]) != string::npos)
		{
			tokens.push_back( str.substr(token_end, 1) );
		}

		token_start = token_end;
		if(token_start == string::npos) break;
		token_start++;
		if(token_start == str.length()) break;
		token_end = str.find_first_of(all_delims, token_start);
	}
}

void litehtml::join_string(string& str, const string_vector& tokens, const string& delims)
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

int litehtml::t_strcasecmp(const char *s1, const char *s2)
{
	int i, d, c;

	for (i = 0;; i++)
	{
		c = tolower((unsigned char)s1[i]);
		d = c - tolower((unsigned char)s2[i]);
		if (d < 0)
			return -1;
		else if (d > 0)
			return 1;
		else if (c == 0)
			return 0;
	}
}

int litehtml::t_strncasecmp(const char *s1, const char *s2, size_t n)
{
	int i, d, c;

	for (i = 0; i < n; i++)
	{
		c = t_tolower((unsigned char)s1[i]);
		d = c - t_tolower((unsigned char)s2[i]);
		if (d < 0)
			return -1;
		else if (d > 0)
			return 1;
	}

	return 0;
}


litehtml::string litehtml::get_escaped_string(const string& in_str)
{
    std::stringstream tss;
	for ( auto ch : in_str )
	{
		switch (ch)
		{
			case '\'':
				tss << "\\'";
				break;

			case '\"':
				tss << "\\\"";
				break;

			case '\?':
				tss << "\\?";
				break;

			case '\\':
				tss << "\\\\";
				break;

			case '\a':
				tss << "\\a";
				break;

			case '\b':
				tss << "\\b";
				break;

			case '\f':
				tss << "\\f";
				break;

			case '\n':
				tss << "\\n";
				break;

			case '\r':
				tss << "\\r";
				break;

			case '\t':
				tss << "\\t";
				break;

			case '\v':
				tss << "\\v";
				break;

			default:
				tss << ch;
		}
	}

	return tss.str();
}

void litehtml::document_container::split_text(const char* text, const std::function<void(const char*)>& on_word, const std::function<void(const char*)>& on_space)
{
	std::wstring str;
	std::wstring str_in = (const wchar_t*)(utf8_to_wchar(text));
	ucode_t c;
	for (size_t i = 0; i < str_in.length(); i++)
	{
		c = (ucode_t)str_in[i];
		if (c <= ' ' && (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f'))
		{
			if (!str.empty())
			{
				on_word(litehtml_from_wchar(str.c_str()));
				str.clear();
			}
			str += c;
			on_space(litehtml_from_wchar(str.c_str()));
			str.clear();
		}
		// CJK character range
		else if (c >= 0x4E00 && c <= 0x9FCC)
		{
			if (!str.empty())
			{
				on_word(litehtml_from_wchar(str.c_str()));
				str.clear();
			}
			str += c;
			on_word(litehtml_from_wchar(str.c_str()));
			str.clear();
		}
		else
		{
			str += c;
		}
	}
	if (!str.empty())
	{
		on_word(litehtml_from_wchar(str.c_str()));
	}
}
