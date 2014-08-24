#pragma once

namespace litehtml
{
#if defined( WIN32 ) || defined( WINCE )

	typedef std::wstring	tstring;
	typedef wchar_t			tchar_t;

	#define _t(quote)			L##quote

	#define t_strlen			wcslen
	#define t_strcmp			wcscmp
	#define t_strncmp			wcsncmp
	#define t_strcasecmp		_wcsicmp
	#define t_strncasecmp		_wcsnicmp
	#define t_strtol			wcstol
	#define t_atoi				_wtoi
	#define t_strtod			wcstod

	#ifdef WINCE
		#define t_snprintf		_snwprintf
	#else
		#define t_snprintf		swprintf
	#endif

	#define t_strstr			wcsstr
	#define t_tolower			towlower
	#define t_isdigit			iswdigit

	#ifdef _WIN64
		typedef unsigned __int64 uint_ptr;
	#else
		typedef unsigned int	uint_ptr;
	#endif

#else

	typedef std::string			tstring;
	typedef char				tchar_t;
	typedef void*				uint_ptr;

	#define _t(quote)			quote

	#define t_strlen			strlen
	#define t_strcmp			strcmp
	#define t_strncmp			strncmp

	#define t_strcasecmp		strcasecmp
	#define t_strncasecmp		strncasecmp
	#define t_snprintf			snprintf

	#define t_strtol			strtol
	#define t_atoi				atoi
	#define t_strtod			strtod
	#define t_strstr			strstr
	#define t_tolower			tolower
	#define t_isdigit			isdigit

#endif
}