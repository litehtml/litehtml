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
			int min_size;
			def_value<int> max_size;
			int main_size;
			int grow;
			int shrink;
			int scaled_flex_shrink_factor;
			bool frozen;
			int order;
			int src_order;
			def_value<int> auto_margin_start;
			def_value<int> auto_margin_end;
			flex_align_items align;

			explicit flex_item(std::shared_ptr<render_item> &_el) :
					el(_el),
					align(flex_align_items_auto),
					grow(0),
					base_size(0),
					shrink(0),
					min_size(0),
					frozen(false),
					main_size(0),
					max_size(0),
					order(0),
					src_order(0),
					scaled_flex_shrink_factor(0),
					auto_margin_start(0),
					auto_margin_end(0)
			{}

			bool operator<(const flex_item& b) const
			{
				if(order < b.order) return true;
				if(order == b.order) return src_order < b.src_order;
				return false;
			}
		};

		struct flex_line
		{
			std::list<flex_item> items;
			int top;
			int main_size;	// sum of all items main size
			int cross_size;	// sum of all items cross size
			int base_size;
			int total_grow;
			int total_shrink;
			int num_auto_margin_start;	// number of items with auto margin left/top
			int num_auto_margin_end;	// number of items with auto margin right/bottom

			flex_line() :
					cross_size(0),
					top(0),
					total_grow(0),
					base_size(0),
					total_shrink(0),
					main_size(0),
					num_auto_margin_start(0),
					num_auto_margin_end(0)
			{}

			void clear()
			{
				items.clear();
				num_auto_margin_start = num_auto_margin_end = top = cross_size = main_size = base_size = total_shrink = total_grow = 0;
			}

			void distribute_free_space(int container_main_size);
		};

		std::list<flex_line> get_lines(const containing_block_context &self_size, formatting_context *fmt_ctx, bool is_row_direction,
									   int container_main_size, bool single_line);
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
