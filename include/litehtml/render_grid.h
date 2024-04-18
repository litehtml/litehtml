#ifndef LITEHTML_RENDER_GRID_H
#define LITEHTML_RENDER_GRID_H

#include "render_block.h"

namespace litehtml
{
	class render_item_grid : public render_item_block
	{
		int _render_content(int x, int y, bool second_pass, const containing_block_context &self_size, formatting_context* fmt_ctx) override;

	public:
		explicit render_item_grid(std::shared_ptr<element> src_el) : render_item_block(std::move(src_el))
		{}

		std::shared_ptr<render_item> clone() override
		{
			return std::make_shared<render_item_grid>(src_el());
		}
		std::shared_ptr<render_item> init() override;

		int get_first_baseline() override;
		int get_last_baseline() override;
	};
}

#endif //LITEHTML_RENDER_GRID_H
