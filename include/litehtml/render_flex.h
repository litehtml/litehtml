#ifndef LITEHTML_RENDER_FLEX_H
#define LITEHTML_RENDER_FLEX_H

#include "render_block.h"

namespace litehtml
{
	class render_item_flex : public render_item_block
	{
		int _render_content(int x, int y, bool second_pass, const containing_block_context &self_size, formatting_context* fmt_ctx) override;

	public:
		explicit render_item_flex(std::shared_ptr<element>  src_el) : render_item_block(std::move(src_el))
		{}

		std::shared_ptr<render_item> clone() override
		{
			return std::make_shared<render_item_flex>(src_el());
		}
		std::shared_ptr<render_item> init() override;
	};
}

#endif //LITEHTML_RENDER_FLEX_H
