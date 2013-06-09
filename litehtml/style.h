#pragma once
#include "attributes.h"
#include <string>

namespace litehtml
{
	class property_value
	{
	public:
		std::wstring	m_value;
		bool			m_important;

		property_value()
		{
			m_important = false;
		}
		property_value(const wchar_t* val, bool imp)
		{
			m_important = imp;
			m_value		= val;
		}
		property_value(const property_value& val)
		{
			m_value		= val.m_value;
			m_important	= val.m_important;
		}

		void operator=(const property_value& val)
		{
			m_value		= val.m_value;
			m_important	= val.m_important;
		}
	};

	typedef std::map<std::wstring, property_value>	props_map;

	class style : public object
	{
	public:
		typedef object_ptr<style>			ptr;
		typedef std::vector<style::ptr>		vector;
	private:
		props_map	m_properties;
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

		void add_property(const wchar_t* name, const wchar_t* val, const wchar_t* baseurl, bool important);

		const wchar_t* get_property(const wchar_t* name) const
		{
			if(name)
			{
				props_map::const_iterator f = m_properties.find(name);
				if(f != m_properties.end())
				{
					return f->second.m_value.c_str();
				}
			}
			return 0;
		}

		void combine(const litehtml::style& src);
		void clear()
		{
			m_properties.clear();
		}

	private:
		void parse_property(const std::wstring& txt, const wchar_t* baseurl);
		void parse(const wchar_t* txt, const wchar_t* baseurl);
		void parse_short_border(const std::wstring& prefix, const std::wstring& val, bool important);
		void parse_short_background(const std::wstring& val, const wchar_t* baseurl, bool important);
		void parse_short_font(const std::wstring& val, bool important);
		void add_parsed_property(const std::wstring& name, const std::wstring& val, bool important);
	};

/*
	class used_styles
	{
	public:
		typedef std::vector<used_styles>	vector;

		style_sheet::ptr	m_style_sheet;
		bool				m_used;

		used_styles()
		{
			m_used = false;
		}

		used_styles(style_sheet::ptr sh, bool used)
		{
			m_used			= used;
			m_style_sheet	= sh;
		}

		used_styles(const used_styles& val)
		{
			m_style_sheet	= val.m_style_sheet;
			m_used			= val.m_used;
		}
	};
*/
}