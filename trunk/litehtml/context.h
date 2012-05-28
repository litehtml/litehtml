#pragma once
#include "style.h"

namespace litehtml
{
	class context
	{
		style_sheet::vector	m_master_css;
	public:
		void					load_master_stylesheet(const wchar_t* str);
		style_sheet::vector&	master_css()
		{
			return m_master_css;
		}
	};
}