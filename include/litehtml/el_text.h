#ifndef LH_EL_TEXT_H
#define LH_EL_TEXT_H

#include "html_tag.h"

namespace litehtml
{
	class el_text : public element
	{
	protected:
		tstring			m_text;
		tstring			m_transformed_text;
		size			m_size;
		bool			m_use_transformed;
		bool			m_draw_spaces;
	public:
		el_text(const tchar_t* text, const std::shared_ptr<litehtml::document>& doc);

		void				get_text(tstring& text) override;
		const tchar_t*		get_style_property(const tchar_t* name, bool inherited, const tchar_t* def = nullptr) const override;
		void				parse_styles(bool is_reparse) override;
		int					get_base_line() override;
		void				draw(uint_ptr hdc, int x, int y, const position* clip) override;

	protected:
		void				get_content_size(size& sz, int max_width) override;
	};
}

#endif  // LH_EL_TEXT_H
