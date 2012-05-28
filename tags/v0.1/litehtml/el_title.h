#pragma once
#include "element.h"

namespace litehtml
{
	class el_title : public element
	{
	public:
		el_title(litehtml::document* doc);
		virtual ~el_title();

	protected:
		virtual void	finish();
	};
}
