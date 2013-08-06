#pragma once
#include "element.h"

namespace litehtml
{
	class el_base : public element
	{
	public:
		el_base(litehtml::document* doc);
		virtual ~el_base();

		virtual void	finish();
	};
}
