#ifndef LH_STYLE_H
#define LH_STYLE_H

#include "attributes.h"
#include <string>

namespace litehtml
{
	class property_value
	{
	public:
		string	m_value;
		bool			m_important;

		property_value()
		{
			m_important = false;
		}
		property_value(const char* val, bool imp)
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

	typedef std::map<string_id, property_value>	props_map;

	class style
	{
	public:
		typedef std::shared_ptr<style>		ptr;
		typedef std::vector<style::ptr>		vector;
	private:
		props_map							m_properties;
		static std::map<string_id, string>	m_valid_values;
	public:
		style() = default;
		style(const style& val);

		style& operator=(const style& val)
		{
			m_properties = val.m_properties;
			return *this;
		}

		void add(const char* txt, const char* baseurl, const element* el)
		{
			parse(txt, baseurl, el);
		}

		void add_property(string_id name, const char* val, const char* baseurl, bool important, const element* el);

		const char* get_property(string_id name) const
		{
			if(name)
			{
				auto f = m_properties.find(name);
				if(f != m_properties.end())
				{
					return f->second.m_value.c_str();
				}
			}
			return nullptr;
		}

		void combine(const style& src);
		void clear()
		{
			m_properties.clear();
		}

	private:
		void parse_property(const string& txt, const char* baseurl, const element* el);
		void parse(const char* txt, const char* baseurl, const element* el);
		void parse_short_background(const string& val, const char* baseurl, bool important);
		void parse_short_font(const string& val, bool important);
		static void subst_vars(string& str, const element* el);
		void add_parsed_property(string_id name, const string& val, bool important);
		void remove_property(string_id name, bool important);
	};
}

#endif  // LH_STYLE_H
