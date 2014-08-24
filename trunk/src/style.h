#pragma once
#include "attributes.h"
#include <string>

namespace litehtml
{
	class property_value
	{
	public:
		tstring	m_value;
		bool			m_important;

		property_value()
		{
			m_important = false;
		}
		property_value(const tchar_t* val, bool imp)
		{
			m_important = imp;
			m_value		= val;
		}
		property_value(const property_value& val)
		{
			m_value		= val.m_value;
			m_important	= val.m_important;
		}

		property_value& operator=(const property_value& val)
		{
			m_value		= val.m_value;
			m_important	= val.m_important;
			return *this;
		}
	};

	typedef std::map<tstring, property_value>	props_map;

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

		void add(const tchar_t* txt, const tchar_t* baseurl)
		{
			parse(txt, baseurl);
		}

		void add_property(const tchar_t* name, const tchar_t* val, const tchar_t* baseurl, bool important);

		const tchar_t* get_property(const tchar_t* name) const
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
		void parse_property(const tstring& txt, const tchar_t* baseurl);
		void parse(const tchar_t* txt, const tchar_t* baseurl);
		void parse_short_border(const tstring& prefix, const tstring& val, bool important);
		void parse_short_background(const tstring& val, const tchar_t* baseurl, bool important);
		void parse_short_font(const tstring& val, bool important);
		void add_parsed_property(const tstring& name, const tstring& val, bool important);
		void remove_property(const tstring& name, bool important);
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