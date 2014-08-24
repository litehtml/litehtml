#pragma once
#include "types.h"

namespace litehtml
{
	class css_length
	{
		union
		{
			float	m_value;
			int		m_predef;
		};
		css_units	m_units;
		bool		m_is_predefined;
	public:
		css_length()
		{
			m_value			= 0;
			m_predef		= 0;
			m_units			= css_units_none;
			m_is_predefined	= false;
		}

		css_length(const css_length& val)
		{
			if(val.is_predefined())
			{
				m_predef	= val.m_predef;
			} else
			{
				m_value		= val.m_value;
			}
			m_units			= val.m_units;
			m_is_predefined	= val.m_is_predefined;
		}

		css_length&	operator=(const css_length& val)
		{
			if(val.is_predefined())
			{
				m_predef	= val.m_predef;
			} else
			{
				m_value		= val.m_value;
			}
			m_units			= val.m_units;
			m_is_predefined	= val.m_is_predefined;
			return *this;
		}

		bool	is_predefined()	const	
		{ 
			return m_is_predefined;					
		}
		
		void	predef(int val)		
		{ 
			m_predef		= val; 
			m_is_predefined = true;	
		}

		int	predef() const
		{ 
			if(m_is_predefined)
			{
				return m_predef; 
			}
			return 0;
		}

		void	set_value(float val, css_units units)		
		{ 
			m_value			= val; 
			m_is_predefined = false;	
			m_units			= units;
		}

		float	val() const
		{
			if(!m_is_predefined)
			{
				return m_value;
			}
			return 0;
		}

		css_units units() const
		{
			return m_units;
		}

		int calc_percent(int width) const
		{
			if(!is_predefined())
			{
				if(units() == css_units_percentage)
				{
					double res = ((double) width * (double) m_value / 100.0);

					int int_val = (int) res;
					if(res - int_val >= 0.5)
					{
						int_val++;
					}
					return int_val;
				} else
				{
					return (int) val();
				}
			}
			return 0;
		}

		void	fromString(const tstring& str, const tstring& predefs = _t(""));
	};
}