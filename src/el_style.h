#pragma once
#include "html_tag.h"

namespace litehtml
{
	class el_style : public element
	{
		elements_vector		m_children;
	public:
		el_style(std::shared_ptr<litehtml::document>& doc);
		virtual ~el_style();

		virtual void			parse_attributes() override;
		virtual bool			appendChild(litehtml::element::ptr& el) override;
		virtual const tchar_t*	get_tagName() const override;
	};
}
