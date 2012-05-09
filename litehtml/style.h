#pragma once
#include "attributes.h"

namespace litehtml
{
	class style
	{
	public:
		typedef std::map<std::wstring, litehtml::style>		map;
		typedef std::vector<litehtml::style>				vector;

	private:
		string_map		m_properties;
	public:
		style();
		style(const style& val);
		virtual ~style();

		void operator=(const style& val)
		{
			m_properties = val.m_properties;
		}

		void add(const wchar_t* txt, const wchar_t* baseurl)
		{
			parse(txt, baseurl);
		}

		void add_property(const wchar_t* name, const wchar_t* val, const wchar_t* baseurl);

		const wchar_t* get_property(const wchar_t* name) const
		{
			if(name)
			{
				string_map::const_iterator f = m_properties.find(name);
				if(f != m_properties.end())
				{
					return f->second.c_str();
				}
			}
			return 0;
		}

		void combine(const litehtml::style& src);

	private:
		void parse_property(const std::wstring& txt, const wchar_t* baseurl);
		void parse(const wchar_t* txt, const wchar_t* baseurl);
		void parse_short_border(const std::wstring& prefix, const std::wstring& val);
		void parse_short_background(const std::wstring& val, const wchar_t* baseurl);
	};

	class style_sheet
	{
	public:
		typedef std::vector<litehtml::style_sheet>	vector;

		css_selector::vector	m_selectors;
		style					m_style;

		style_sheet()	{}
		style_sheet(const style_sheet& val)
		{
			m_selectors	= val.m_selectors;
			m_style		= val.m_style;
		}
		void operator=(const style_sheet& val)
		{
			m_selectors	= val.m_selectors;
			m_style		= val.m_style;
		}
		void add_selector(const std::wstring& txt);
	};
}