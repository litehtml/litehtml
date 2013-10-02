#pragma once
#include "style.h"

namespace litehtml
{
	class document_container;

	class css
	{
		css_selector::vector	m_selectors;
	public:
		css()
		{

		}
		
		~css()
		{

		}

		const css_selector::vector& selectors() const
		{
			return m_selectors;
		}

		void	add_selector(css_selector::ptr selector);
		void	parse_stylesheet(const tchar_t* str, const tchar_t* baseurl, document_container* doc);
		void	parse_selectors(const tstring& txt, litehtml::style::ptr styles);
		void	sort_selectors();
		static void	parse_css_url(const tstring& str, tstring& url);

	};

	inline void litehtml::css::add_selector( css_selector::ptr selector )
	{
		selector->m_order = (int) m_selectors.size();
		m_selectors.push_back(selector);
	}

}