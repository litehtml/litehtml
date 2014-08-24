#pragma once
#include "html_tag.h"

namespace litehtml
{
	class el_script : public element
	{
		tstring m_text;
	public:
		el_script(litehtml::document* doc);
		virtual ~el_script();

		virtual void			parse_attributes();
		virtual bool			appendChild(litehtml::element* el);
		virtual const tchar_t*	get_tagName() const;
	};
}
