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
			def_value<int> auto_margin_main_start;
			def_value<int> auto_margin_main_end;
			bool auto_margin_cross_start;
			bool auto_margin_cross_end;
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
					auto_margin_main_start(0),
					auto_margin_main_end(0),
					auto_margin_cross_start(false),
					auto_margin_cross_end(false)
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
			int num_auto_margin_main_start;		// number of items with auto margin left/top
			int num_auto_margin_main_end;		// number of items with auto margin right/bottom
			int num_auto_margin_cross_start;	// number of items with auto margin left/top
			int num_auto_margin_cross_end;		// number of items with auto margin right/bottom
			int first_baseline;
			int last_baseline;

			flex_line() :
					cross_size(0),
					top(0),
					total_grow(0),
					base_size(0),
					total_shrink(0),
					main_size(0),
					num_auto_margin_main_start(0),
					num_auto_margin_main_end(0),
					num_auto_margin_cross_start(0),
					num_auto_margin_cross_end(0),
					first_baseline(0),
					last_baseline(0)
			{}

			void distribute_free_space(int container_main_size);
		};

		std::list<flex_line> m_lines;
		def_value<int>	m_first_baseline;
		def_value<int>	m_last_baseline;

		std::list<flex_line> get_lines(const containing_block_context &self_size, formatting_context *fmt_ctx, bool is_row_direction,
									   int container_main_size, bool single_line);
		int _render_content(int x, int y, bool second_pass, const containing_block_context &self_size, formatting_context* fmt_ctx) override;

	public:
		explicit render_item_flex(std::shared_ptr<element>  src_el) : render_item_block(std::move(src_el)),
																	  m_first_baseline(0), m_last_baseline(0)
		{}

		std::shared_ptr<render_item> clone() override
		{
			return std::make_shared<render_item_flex>(src_el());
		}
		std::shared_ptr<render_item> init() override;

		int get_first_baseline() override;
		int get_last_baseline() override;
	};
}

#endif //LITEHTML_RENDER_FLEX_H
