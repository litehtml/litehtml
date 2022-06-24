#ifndef LH_EL_IMAGE_H
#define LH_EL_IMAGE_H

#include "html_tag.h"

namespace litehtml
{

	class el_image : public html_tag
	{
		tstring	m_src;
	public:
		el_image(const std::shared_ptr<litehtml::document>& doc);

		bool	is_replaced() const override;
		void	parse_attributes() override;
		void	parse_styles(bool is_reparse = false) override;
		void    draw(uint_ptr hdc, int x, int y, const position *clip, const std::shared_ptr<render_item> &ri) override;
		void	get_content_size(size& sz, int max_width) override;
        tstring dump_get_name() override;

        std::shared_ptr<render_item> create_render_item(const std::shared_ptr<render_item>& parent_ri) override;

	private:
//		int calc_max_height(int image_height);
	};
}

#endif  // LH_EL_IMAGE_H
