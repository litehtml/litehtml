#pragma once
#include "html_tag.h"

namespace litehtml
{
	class el_script : public element
	{
		tstring m_text;
		tstring m_src;
	public:
		el_script(litehtml::document* doc);

		virtual ~el_script();

		const tstring & text() const { return m_text; }

		virtual void			parse_attributes() override;
		virtual bool			appendChild(litehtml::element* el) override;
		virtual bool			addChildAfter(litehtml::element* new_child, litehtml::element * existing_child) override;
		virtual const tchar_t*	get_tagName() const override;
		virtual void			set_attr(const string_hash& name, const tchar_t* val) override;
		virtual const tchar_t*	get_attr(const string_hash& name, const tchar_t* def = 0) const override;
	};
}
