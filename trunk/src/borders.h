#pragma once
#include "css_length.h"
#include "types.h"

namespace litehtml
{
	struct css_border
	{
		css_length		width;
		border_style	style;
		web_color		color;

		css_border()
		{
			style = border_style_none;
		}

		css_border(const css_border& val)
		{
			width	= val.width;
			style	= val.style;
			color	= val.color;
		}

		css_border& operator=(const css_border& val)
		{
			width	= val.width;
			style	= val.style;
			color	= val.color;
			return *this;
		}
	};

	struct css_border_radius
	{
		css_length	top_left_x;
		css_length	top_left_y;

		css_length	top_right_x;
		css_length	top_right_y;

		css_length	bottom_right_x;
		css_length	bottom_right_y;

		css_length	bottom_left_x;
		css_length	bottom_left_y;

		css_border_radius()
		{

		}

		css_border_radius(const css_border_radius& val)
		{
			top_left_x		= val.top_left_x;
			top_left_y		= val.top_left_y;
			top_right_x		= val.top_right_x;
			top_right_y		= val.top_right_y;
			bottom_left_x	= val.bottom_left_x;
			bottom_left_y	= val.bottom_left_y;
			bottom_right_x	= val.bottom_right_x;
			bottom_right_y	= val.bottom_right_y;
		}

		css_border_radius& operator=(const css_border_radius& val)
		{
			top_left_x		= val.top_left_x;
			top_left_y		= val.top_left_y;
			top_right_x		= val.top_right_x;
			top_right_y		= val.top_right_y;
			bottom_left_x	= val.bottom_left_x;
			bottom_left_y	= val.bottom_left_y;
			bottom_right_x	= val.bottom_right_x;
			bottom_right_y	= val.bottom_right_y;
			return *this;
		}
	};

	struct css_borders
	{
		css_border			left;
		css_border			top;
		css_border			right;
		css_border			bottom;
		css_border_radius	radius;

		css_borders()
		{

		}

		css_borders(const css_borders& val)
		{
			left	= val.left;
			right	= val.right;
			top		= val.top;
			bottom	= val.bottom;
		}
	};
}
