#include "html.h"
#include "types.h"
#include "render_grid.h"

int litehtml::render_item_grid::_render_content(int x, int y, bool second_pass,
												const litehtml::containing_block_context &self_size,
												litehtml::formatting_context *fmt_ctx)
{
	return render_item_block::_render_content(x, y, second_pass, self_size, fmt_ctx);
}

std::shared_ptr<litehtml::render_item> litehtml::render_item_grid::init()
{
	children_to_blocks();
	return shared_from_this();
}

int litehtml::render_item_grid::get_first_baseline()
{
	return render_item::get_first_baseline();
}

int litehtml::render_item_grid::get_last_baseline()
{
	return render_item::get_last_baseline();
}
