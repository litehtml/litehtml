#ifndef LH_CSS_ANGLE_H
#define LH_CSS_ANGLE_H

#include "types.h"
#include "css_tokenizer.h"

namespace litehtml
{
	// <number>
	// https://developer.mozilla.org/en-US/docs/Web/CSS/angle
	class css_angle
	{
		union
		{
			float	m_value;
			int		m_predef;
		};
		css_angle_units	m_units;
		bool		m_is_predefined;
	public:
		css_angle(float val, css_angle_units units);

		bool		is_predefined() const;
		void		predef(int val);
		int			predef() const;
		void		set_value(float val, css_angle_units units);
		float		val() const;
		float		val_from_units(css_angle_units units) const;
		float		radians() const;
		css_angle_units	units() const;
		bool		from_token(const css_token& token, const string& predefined_keywords = "");
	};
}

#endif  // LH_CSS_ANGLE_H
