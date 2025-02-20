#include "html.h"
#include "css_angle.h"
#include <numbers>

namespace litehtml
{
	css_angle::css_angle(float val, css_angle_units units)
	{
		m_value = val;
		m_units = units;
		m_is_predefined = false;
	}

	bool css_angle::is_predefined() const
	{ 
		return m_is_predefined;
	}

	void css_angle::predef(int val)
	{ 
		m_predef		= val;
		m_is_predefined = true;
	}

	int css_angle::predef() const
	{ 
		if(m_is_predefined)
		{
			return m_predef; 
		}
		return 0;
	}

	void css_angle::set_value(float val, css_angle_units units)
	{ 
		m_value			= val; 
		m_is_predefined = false;	
		m_units			= units;
	}

	float css_angle::val() const
	{
		if(!m_is_predefined)
		{
			return m_value;
		}
		return 0;
	}

	css_angle_units css_angle::units() const
	{
		return m_units;
	}

	bool css_angle::from_token(const css_token& token, const string& predefined_keywords)
	{
		if (!is_one_of(token.type, DIMENSION))
			return false;

		if (token.type == DIMENSION)
		{
			int idx = value_index(lowcase(token.unit), css_angle_units_strings);

			if (idx == -1)
				return false;

			m_value = token.n.number;
			m_units = (css_angle_units)idx;
			m_is_predefined = false;
			return true;
		}
		return false;
	}

	float css_angle::val_from_units(css_angle_units units) const
	{
		if (m_units == units)
		{
			return m_value;
		}
		const auto rad = radians();
		switch (units)
		{
			case litehtml::css_angle_units_deg:
				return rad * 180 / std::numbers::pi_v<float>;
			case litehtml::css_angle_units_grad:
				return rad * 200 / std::numbers::pi_v<float>;
			case litehtml::css_angle_units_rad:
				return rad;
			case litehtml::css_angle_units_turn:
				return rad / (2 * std::numbers::pi_v<float>);
			default:
				return rad;
		}
	}

	float css_angle::radians() const
	{
		switch (m_units)
		{
			case litehtml::css_angle_units_deg:
				return m_value * std::numbers::pi_v<float> / 180;
			case litehtml::css_angle_units_grad:
				return m_value * std::numbers::pi_v<float> / 200;
			case litehtml::css_angle_units_rad:
				return m_value;
			case litehtml::css_angle_units_turn:
				return m_value * 2 * std::numbers::pi_v<float>;
			default:
				return m_value;
		}
	}
}