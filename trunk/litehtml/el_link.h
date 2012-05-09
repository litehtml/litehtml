#pragma once
#include "element.h"

namespace litehtml
{
	class el_link : public element
	{
	public:
		el_link(litehtml::document* doc);
		virtual ~el_link();

	protected:
		virtual void	finish();
	};
}
