#pragma once

#include "object.h"

namespace litehtml
{
#if defined(_WIN64)
	typedef unsigned __int64 uint_ptr;
#else
	typedef unsigned int uint_ptr;
#endif

	class document;
	class element;

	typedef std::map<std::wstring, std::wstring>			string_map;
	typedef std::map<std::wstring, uint_ptr>				fonts_map;
	typedef std::vector<litehtml::object_ptr<litehtml::element>>	elements_vector;
	typedef std::vector<int>								int_vector;
	typedef std::vector<std::wstring>						string_vector;

	const unsigned int font_decoration_none			= 0x00;
	const unsigned int font_decoration_underline	= 0x01;
	const unsigned int font_decoration_linethrough	= 0x02;
	const unsigned int font_decoration_overline		= 0x04;

	typedef unsigned char	byte;

	struct margins
	{
		int	left;
		int	right;
		int top;
		int bottom;

		margins()
		{
			left = right = top = bottom = 0;
		}

		int width()		const	{ return left + right; } 
		int height()	const	{ return top + bottom; } 
	};

	struct size
	{
		int		width;
		int		height;

		size()
		{
			width	= 0;
			height	= 0;
		}
	};

	struct position
	{
		typedef std::vector<position>	vector;

		int	x;
		int	y;
		int	width;
		int	height;

		position()
		{
			x = y = width = height = 0;
		}

		position(int x, int y, int width, int height)
		{
			this->x			= x;
			this->y			= y;
			this->width		= width;
			this->height	= height;
		}

		int right()		const		{ return x + width;		}
		int bottom()	const		{ return y + height;	}
		int left()		const		{ return x;				}
		int top()		const		{ return y;				}

		void operator+=(const margins& mg)
		{
			x		-= mg.left;
			y		-= mg.top;
			width	+= mg.left + mg.right;
			height	+= mg.top + mg.bottom;
		}
		void operator-=(const margins& mg)
		{
			x		+= mg.left;
			y		+= mg.top;
			width	-= mg.left + mg.right;
			height	-= mg.top + mg.bottom;
		}

		void clear()
		{
			x = y = width = height = 0;
		}

		void operator=(const size& sz)
		{
			width	= sz.width;
			height	= sz.height;
		}

		void move_to(int x, int y)
		{
			this->x = x;
			this->y = y;
		}

		bool does_intersect(const position* val) const
		{
			if(!val) return true;

			return (
				left()			<= val->right()		&& 
				right()			>= val->left()		&& 
				bottom()		>= val->top()		&& 
				top()			<= val->bottom()	)
				|| (
				val->left()		<= right()			&& 
				val->right()	>= left()			&& 
				val->bottom()	>= top()			&& 
				val->top()		<= bottom()			);
		}

		bool empty() const
		{
			if(!width && !height)
			{
				return true;
			}
			return false;
		}

		bool is_point_inside(int x, int y) const
		{
			if(x >= left() && x <= right() && y >= top() && y <= bottom())
			{
				return true;
			}
			return false;
		}
	};

#define  style_display_strings		L"none;block;inline;inline-block;list-item;table;table-caption;table-cell;table-column;table-column-group;table-footer-group;table-header-group;table-row;table-row-group"

	enum style_display
	{
		display_none,
		display_block,
		display_inline,
		display_inline_block,
		display_list_item,
		display_table,
		display_table_caption,
		display_table_cell,
		display_table_column,
		display_table_column_group,
		display_table_footer_group,
		display_table_header_group,
		display_table_row,
		display_table_row_group,
		display_inline_text,
	};

	enum style_border
	{
		borderNope,
		borderNone,
		borderHidden,
		borderDotted,
		borderDashed,
		borderSolid,
		borderDouble
	};

#define  font_size_strings		L"xx-small;x-small;small;medium;x-large;xx-large;smaller;larger"

	enum font_size
	{
		fontSize_xx_small,
		fontSize_x_small,
		fontSize_small,
		fontSize_medium,
		fontSize_large,
		fontSize_x_large,
		fontSize_xx_large,
		fontSize_smaller,
		fontSize_larger,
	};

#define  font_style_strings		L"normal;italic"

	enum font_style
	{
		fontStyleNormal,
		fontStyleItalic
	};

#define  font_variant_strings		L"normal;small-caps"

	enum font_variant
	{
		font_variant_normal,
		font_variant_italic
	};

#define  font_weight_strings	L"normal;bold;bolder;lighter100;200;300;400;500;600;700"

	enum font_weight
	{
		fontWeightNormal,
		fontWeightBold,
		fontWeightBolder,
		fontWeightLighter,
		fontWeight100,
		fontWeight200,
		fontWeight300,
		fontWeight400,
		fontWeight500,
		fontWeight600,
		fontWeight700
	};

#define  list_style_type_strings	L"none;circle;disc;square"

	enum list_style_type
	{
		list_style_type_none,
		list_style_type_circle,
		list_style_type_disc,
		list_style_type_square
	};

#define  list_style_position_strings	L"inside;outside"

	enum list_style_position
	{
		list_style_position_inside,
		list_style_position_outside
	};

#define  vertical_align_strings	L"baseline;sub;super;top;text-top;middle;bottom;text-bottom"

	enum vertical_align
	{
		va_baseline,
		va_sub,
		va_super,
		va_top,
		va_text_top,
		va_middle,
		va_bottom,
		va_text_bottom
	};

#define  border_width_strings	L"thin;medium;thick"

	enum border_width
	{
		border_width_thin,
		border_width_medium,
		border_width_thick
	};

#define  border_style_strings	L"none;hidden;dotted;dashed;solid;double;groove;ridge;inset;outset"

	enum border_style
	{
		border_style_none,
		border_style_hidden,
		border_style_dotted,
		border_style_dashed,
		border_style_solid,
		border_style_double,
		border_style_groove,
		border_style_ridge,
		border_style_inset,
		border_style_outset
	};

#define  element_float_strings	L"none;left;right"

	enum element_float
	{
		float_none,
		float_left,
		float_right
	};

#define  element_clear_strings	L"none;left;right;both"

	enum element_clear
	{
		clear_none,
		clear_left,
		clear_right,
		clear_both
	};

#define  css_units_strings	L"none;%;in;cm;mm;em;ex;pt;pc;px"

	enum css_units
	{
		css_units_none,
		css_units_percentage,
		css_units_in,
		css_units_cm,
		css_units_mm,
		css_units_em,
		css_units_ex,
		css_units_pt,
		css_units_pc,
		css_units_px
	};

#define  background_attachment_strings	L"scroll;fixed"

	enum background_attachment
	{
		background_attachment_scroll,
		background_attachment_fixed
	};

#define  background_repeat_strings	L"repeat;repeat-x;repeat-y;no-repeat"

	enum background_repeat
	{
		background_repeat_repeat,
		background_repeat_repeat_x,
		background_repeat_repeat_y,
		background_repeat_no_repeat
	};

#define  background_box_strings	L"border-box;padding-box;content-box"

	enum background_box
	{
		background_box_border,
		background_box_padding,
		background_box_content
	};

#define element_position_strings	L"static;absolute;fixed;relative"

	enum element_position
	{
		element_position_static,
		element_position_absolute,
		element_position_fixed,
		element_position_relative
	};

#define text_align_strings		L"left;right;center;justify"

	enum text_align
	{
		text_align_left,
		text_align_right,
		text_align_center,
		text_align_justify
	};

#define text_transform_strings		L"none;capitalize;uppercase;lowercase"

	enum text_transform
	{
		text_transform_none,
		text_transform_capitalize,
		text_transform_uppercase,
		text_transform_lowercase
	};

#define white_space_strings		L"normal;nowrap;pre;pre-line;pre-wrap"

	enum white_space
	{
		white_space_normal,
		white_space_nowrap,
		white_space_pre,
		white_space_pre_line,
		white_space_pre_wrap
	};

#define overflow_strings		L"visible;hidden;scroll;auto;no-display;no-content"

	enum overflow
	{
		overflow_visible,
		overflow_hidden,
		overflow_scroll,
		overflow_auto,
		overflow_no_display,
		overflow_no_content
	};

}
