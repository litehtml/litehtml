#include "html.h"
#include "background.h"

litehtml::background_paint::background_paint()
{
	position_x		= 0;
	position_y		= 0;
	attachment		= background_attachment_scroll;
	repeat			= background_repeat_repeat;
	color			= web_color::transparent;
	is_root			= false;
}
