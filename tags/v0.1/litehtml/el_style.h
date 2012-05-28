#pragma once
#include "element.h"

namespace litehtml
{
	class el_style : public element
	{
	public:
		el_style(litehtml::document* doc);
		virtual ~el_style();

		virtual void	finish();
	};
}
