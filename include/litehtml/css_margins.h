#ifndef LH_CSS_MARGINS_H
#define LH_CSS_MARGINS_H

#include "css_length.h"

namespace litehtml
{
	struct css_margins
	{
		css_length	left;
		css_length	right;
		css_length	top;
		css_length	bottom;

		css_margins() = default;

		css_margins(const css_margins& val)
		{
			left	= val.left;
			right	= val.right;
			top		= val.top;
			bottom	= val.bottom;
		}

		css_margins& operator=(const css_margins& val)
		{
			left	= val.left;
			right	= val.right;
			top		= val.top;
			bottom	= val.bottom;
			return *this;
		}

        tstring to_string()
        {
            return _t("left: ") + left.to_string() +
                   _t(", right: ") + right.to_string() +
                   _t(", top: ") + top.to_string() +
                   _t(", bottom: ") + bottom.to_string();
        }
	};
}

#endif  // LH_CSS_MARGINS_H
