#ifndef LH_EL_TEXT_H
#define LH_EL_TEXT_H

#include "html_tag.h"

namespace litehtml
{
	class el_text : public element
	{
	protected:
		string			m_text;
		string			m_transformed_text;
		size			m_size;
		bool			m_use_transformed;
		bool			m_draw_spaces;
	public:
		el_text(const char* text, const std::shared_ptr<litehtml::document>& doc);

		void				get_text(string& text) override;
		const char*			get_style_property(const char* name, bool inherited, const char* def = nullptr) const override;
		void				parse_styles(bool is_reparse) override;
        bool				is_text() const override { return true; }

        void draw(uint_ptr hdc, int x, int y, const position *clip, const std::shared_ptr<render_item> &ri) override;
        string             dump_get_name() override;
        std::vector<std::tuple<string, string>> dump_get_attrs() override;
	protected:
		void				get_content_size(size& sz, int max_width) override;
	};
}

#endif  // LH_EL_TEXT_H
