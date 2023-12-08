#ifndef LITEHTML_RENDER_FLEX_H
#define LITEHTML_RENDER_FLEX_H

#include "render_block.h"

namespace litehtml
{
	class render_item_flex : public render_item_block
	{
		struct flex_item
		{
			std::shared_ptr<render_item> el;
			int base_size;
			int main_size;
			int min_width;
			int max_width;
			int line;

			explicit flex_item(std::shared_ptr<render_item>  _el) :
					el(std::move(_el)),
					min_width(0),
					max_width(0),
					line(0),
					base_size(0),
					main_size(0)
			{}
		};
	protected:
		std::list<std::unique_ptr<flex_item>>   m_flex_items;

		int _render_content(int x, int y, bool second_pass, const containing_block_context &self_size, formatting_context* fmt_ctx) override;

	public:
		explicit render_item_flex(std::shared_ptr<element>  src_el) : render_item_block(std::move(src_el))
		{}

		std::shared_ptr<render_item> clone() override
		{
			return std::make_shared<render_item_flex>(src_el());
		}
		void draw_children(uint_ptr hdc, int x, int y, const position* clip, draw_flag flag, int zindex) override;
		std::shared_ptr<render_item> init() override;
	};
}

#endif //LITEHTML_RENDER_FLEX_H
