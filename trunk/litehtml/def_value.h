#pragma once

namespace litehtml
{
	template<class T>
	class def_value
	{
		T		m_val;
		bool	m_isDefined;
	public:
		def_value()
		{ 
			m_isDefined = 0;
		}
		def_value(T& val)
		{ 
			m_isDefined = true; 
			m_val = val; 
		}

		bool defined() const	
		{ 
			return m_isDefined;	
		}

		void operator=(const def_value& val)
		{
			m_isDefined = val.m_isDefined;
			if(m_isDefined)
			{
				m_val = val.m_val;
			}
		}

		operator const T&() const
		{
			return m_val;
		}

	};

	typedef def_value<int>	def_int;
}