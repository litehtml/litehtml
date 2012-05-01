#pragma once
#include "element.h"

namespace litehtml
{
	class el_para : public element
	{
	public:
		el_para(litehtml::document* doc);
		virtual ~el_para();
	};
}