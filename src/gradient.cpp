#include "html.h"
#include "gradient.h"

#ifndef M_PI
#       define M_PI    3.14159265358979323846
#endif

namespace litehtml
{
	/**
	 * Parse CSS angle
	 * @param str css text of angle
	 * @param with_percent true to pare percent as angle (for conic gradient)
	 * @param angle output angle value in degrees
	 * @return
	 */
	static bool parse_css_angle(const string& str, bool with_percent, float& angle)
	{
		const char* start = str.c_str();
		for(;start[0]; start++)
		{
			if(!isspace(start[0])) break;
		}
		if(start[0] == 0) return false;
		char* end = nullptr;
		auto a = (float) t_strtod(start, &end);
		if(end && end[0] == 0) return false;
		if(!strcmp(end, "rad"))
		{
			a = (float) (a * 180.0 / M_PI);
		} else if(!strcmp(end, "grad"))
		{
			a = a * 180.0f / 200.0f;
		} else if(!strcmp(end, "turn"))
		{
			a = a * 360.0f;
		} else if(strcmp(end, "deg"))
		{
			if(with_percent && strcmp(end, "%"))
			{
				a = a * 360.0f / 100.0f;
			} else
			{
				return false;
			}
		}
		angle = a;
		return true;
	}

	/**
	 * Parse colors stop list for radial and linear gradients.
	 * Formal syntax:
	 * \code
	 * <linear-color-stop> =
	 *   <color> <length-percentage>?
	 *
	 * <linear-color-hint> =
	 *   <length-percentage>
	 *
	 * <length-percentage> =
	 *   <length>      |
	 *   <percentage>
	 * \endcode
	 * @param parts
	 * @param container
	 * @param grad
	 */
	static void parse_color_stop_list(const string_vector& parts, document_container *container, std::vector<background_gradient::gradient_color>& colors)
	{
		auto color = web_color::from_string(parts[0], container);
		css_length length;
		if(parts.size() > 1)
		{
			length.fromString(parts[1]);
			if(!length.is_predefined())
			{
				background_gradient::gradient_color gc;
				gc.color = color;
				gc.length = length;
				colors.push_back(gc);
			}
			if(parts.size() > 2)
			{
				length.fromString(parts[2]);
				if(!length.is_predefined())
				{
					background_gradient::gradient_color gc;
					gc.color = color;
					gc.length = length;
					colors.push_back(gc);
				}
			}
		} else
		{
			background_gradient::gradient_color gc;
			gc.color = color;
			colors.push_back(gc);
		}
	}

	static void parse_color_stop_angle_list(const string_vector& parts, document_container *container, background_gradient& grad)
	{
		auto color = web_color::from_string(parts[0], container);
		if(parts.size() > 1)
		{
			float angle = 0;
			if(parse_css_angle(parts[1], true, angle))
			{
				background_gradient::gradient_color gc;
				gc.angle = angle;
				gc.color = color;
				grad.m_colors.push_back(gc);
			}
			if(parts.size() > 2)
			{
				if(parse_css_angle(parts[1], true, angle))
				{
					background_gradient::gradient_color gc;
					gc.color = color;
					gc.angle = angle;
					grad.m_colors.push_back(gc);
				}
			}
		} else
		{
			background_gradient::gradient_color gc;
			gc.color = color;
			grad.m_colors.push_back(gc);
		}
	}

	/**
	 * Parse linear gradient definition.
	 * Formal syntax:
	 * \code{plain}
	 * <linear-gradient()> =
	 *   linear-gradient( [ <linear-gradient-syntax> ] )
	 *
	 * <linear-gradient-syntax> =
	 *   [ <angle> | to <side-or-corner> ]? , <color-stop-list>
	 *
	 * <side-or-corner> =
	 *   [ left | right ]  ||
	 *   [ top | bottom ]
	 *
	 * <color-stop-list> =
	 *   <linear-color-stop> , [ <linear-color-hint>? , <linear-color-stop> ]#
	 *
	 * <linear-color-stop> =
	 *   <color> <length-percentage>?
	 *
	 * <linear-color-hint> =
	 *   <length-percentage>
	 *
	 * <length-percentage> =
	 *   <length>      |
	 *   <percentage>
	 * \endcode
	 * @param gradient_str
	 * @param container
	 * @param grad
	 */
	void parse_linear_gradient(const std::string& gradient_str, document_container *container, background_gradient& grad)
	{
		string_vector items;
		split_string(gradient_str, items, ",", "", "()");

		for (auto &item: items)
		{
			trim(item);
			string_vector parts;
			split_string(item, parts, split_delims_spaces, "", "()");
			if (parts.empty()) continue;

			if (parts[0] == "to")
			{
				uint32_t grad_side = 0;
				for (size_t part_idx = 1; part_idx < parts.size(); part_idx++)
				{
					int side = value_index(parts[part_idx], "left;right;top;bottom");
					if (side >= 0)
					{
						grad_side |= 1 << side;
					}
				}
				switch(grad_side)
				{
					case background_gradient::gradient_side_top:
						grad.angle = 0;
						break;
					case background_gradient::gradient_side_bottom:
						grad.angle = 180;
						break;
					case background_gradient::gradient_side_left:
						grad.angle = 270;
						break;
					case background_gradient::gradient_side_right:
						grad.angle = 90;
						break;
					case background_gradient::gradient_side_top | background_gradient::gradient_side_left:
					case background_gradient::gradient_side_top | background_gradient::gradient_side_right:
					case background_gradient::gradient_side_bottom | background_gradient::gradient_side_left:
					case background_gradient::gradient_side_bottom | background_gradient::gradient_side_right:
						grad.m_side = grad_side;
						break;
					default:
						break;
				}
			} else if (parts.size() == 1 && parse_css_angle(parts[0], false, grad.angle))
			{
				continue;
			} else if (web_color::is_color(parts[0], container))
			{
				parse_color_stop_list(parts, container, grad.m_colors);
			} else
			{
				css_length length;
				length.fromString(parts[0]);
				if (!length.is_predefined())
				{
					background_gradient::gradient_color gc;
					gc.length = length;
					gc.is_color_hint = true;
					grad.m_colors.push_back(gc);
				}
			}
		}
	}

	/**
	 * Parse position part for radial gradient.
	 * Formal syntax:
	 * \code
	 * <position> =
	 *   [ left | center | right | top | bottom | <length-percentage> ]  |
	 *   [ left | center | right ] && [ top | center | bottom ]  |
	 *   [ left | center | right | <length-percentage> ] [ top | center | bottom | <length-percentage> ]  |
	 *   [ [ left | right ] <length-percentage> ] && [ [ top | bottom ] <length-percentage> ]
	 * \endcode
	 * @param grad
	 * @param parts
	 * @param i
	 */
	static inline void parse_radial_position(litehtml::background_gradient &grad, const litehtml::string_vector &parts, size_t i)
	{
		grad.m_side = 0;
		while (i < parts.size())
		{
			int side = litehtml::value_index(parts[i], "left;right;top;bottom;center");
			if (side >= 0)
			{
				if(side == 4)
				{
					if(grad.m_side & litehtml::background_gradient::gradient_side_x_center)
					{
						grad.m_side |= litehtml::background_gradient::gradient_side_y_center;
					} else
					{
						grad.m_side |= litehtml::background_gradient::gradient_side_x_center;
					}
				} else
				{
					grad.m_side |= 1 << side;
				}
			} else
			{
				litehtml::css_length length;
				length.fromString(parts[i]);
				if (!length.is_predefined())
				{
					if(grad.m_side & litehtml::background_gradient::gradient_side_x_length)
					{
						grad.m_side |= litehtml::background_gradient::gradient_side_y_length;
						grad.radial_position_y = length;
					} else
					{
						grad.m_side |= litehtml::background_gradient::gradient_side_x_length;
						grad.radial_position_x = length;
					}
				}
			}
			i++;
		}
	}

	/**
	 * Parse radial gradient definition
	 * Formal syntax:
	 * \code
	 * <radial-gradient()> =
	 *   radial-gradient( [ <radial-gradient-syntax> ] )
	 *
	 * <radial-gradient-syntax> =
	 *   [ <radial-shape> || <radial-size> ]? [ at <position> ]? , <color-stop-list>
	 *
	 * <radial-shape> =
	 *   circle   |
	 *   ellipse
	 *
	 * <radial-size> =
	 *   <radial-extent>               |
	 *   <length [0,∞]>                |
	 *   <length-percentage [0,∞]>{2}
	 *
	 * <position> =
	 *   [ left | center | right | top | bottom | <length-percentage> ]  |
	 *   [ left | center | right ] && [ top | center | bottom ]  |
	 *   [ left | center | right | <length-percentage> ] [ top | center | bottom | <length-percentage> ]  |
	 *   [ [ left | right ] <length-percentage> ] && [ [ top | bottom ] <length-percentage> ]
	 *
	 * <color-stop-list> =
	 *   <linear-color-stop> , [ <linear-color-hint>? , <linear-color-stop> ]#
	 *
	 * <radial-extent> =
	 *   closest-corner   |
	 *   closest-side     |
	 *   farthest-corner  |
	 *   farthest-side
	 *
	 * <length-percentage> =
	 *   <length>      |
	 *   <percentage>

	 * <linear-color-stop> =
	 *   <color> <length-percentage>?
	 *
	 * <linear-color-hint> =
	 *   <length-percentage>
	 * \endcode
	 * @param gradient_str
	 * @param container
	 * @param grad
	 */
	void parse_radial_gradient(const std::string& gradient_str, document_container *container, background_gradient& grad)
	{
		string_vector items;
		split_string(gradient_str, items, ",", "", "()");

		for (auto &item: items)
		{
			trim(item);
			string_vector parts;
			split_string(item, parts, split_delims_spaces, "", "()");
			if (parts.empty()) continue;

			if (web_color::is_color(parts[0], container))
			{
				parse_color_stop_list(parts, container, grad.m_colors);
			} else
			{
				size_t i = 0;
				while(i < parts.size())
				{
					if(parts[i] == "at")
					{
						parse_radial_position(grad, parts, i + 1);
						break;
					} else // parts[i] == "at"
					{
						int val = value_index(parts[i], "closest-corner;closest-side;farthest-corner;farthest-side");
						if(val >= 0)
						{
							grad.radial_extent = (background_gradient::radial_extent_t) (val + 1);
						} else
						{
							val = value_index(parts[i], "circle;ellipse");
							if(val >= 0)
							{
								grad.radial_shape = (background_gradient::radial_shape_t)  (val + 1);
							} else
							{
								css_length length;
								length.fromString(parts[i]);
								if (!length.is_predefined())
								{
									if(!grad.radial_length_x.is_predefined())
									{
										grad.radial_length_y = length;
									} else
									{
										grad.radial_length_x = length;
									}
								}
							}
						}
					} // else parts[i] == "at"
					i++;
				} // while(i < parts.size())
			} // else web_color::is_color(parts[0], container)
		} // for
		if(grad.radial_extent == background_gradient::radial_extent_none)
		{
			if(grad.radial_length_x.is_predefined())
			{
				grad.radial_extent = background_gradient::radial_extent_farthest_corner;
			} else if(grad.radial_length_y.is_predefined())
			{
				grad.radial_length_y = grad.radial_length_x.val();
				grad.radial_shape = background_gradient::radial_shape_circle;
			}
		}
		if(grad.radial_shape == background_gradient::radial_shape_none)
		{
			grad.radial_shape = background_gradient::radial_shape_ellipse;
		}
	}

	/**
	 * Parse conic gradient definition.
	 * Formal syntax:
	 * \code
	 * conic-gradient-syntax =
	 *   [ [ [ from <angle> ]? [ at position ]? ] || <color-interpolation-method> ]? , <angular-color-stop-list>
	 *
	 * <position> =
	 *   [ left | center | right | top | bottom | <length-percentage> ]  |
	 *   [ left | center | right ] && [ top | center | bottom ]  |
	 *   [ left | center | right | <length-percentage> ] [ top | center | bottom | <length-percentage> ]  |
	 *   [ [ left | right ] <length-percentage> ] && [ [ top | bottom ] <length-percentage> ]
	 *
	 * <color-interpolation-method> =
	 *   in [ <rectangular-color-space> | <polar-color-space> <hue-interpolation-method>? ]
	 *
	 * <angular-color-stop-list> =
	 *   <angular-color-stop> , [ <angular-color-hint>? , <angular-color-stop> ]#
	 *
	 * <length-percentage> =
  	 *   <length>      |
	 *   <percentage>
	 *
	 * <rectangular-color-space> =
	 *   srgb          |
	 *   srgb-linear   |
	 *   display-p3    |
	 *   a98-rgb       |
	 *   prophoto-rgb  |
	 *   rec2020       |
	 *   lab           |
	 *   oklab         |
	 *   xyz           |
	 *   xyz-d50       |
	 *   xyz-d65
	 *
	 * <polar-color-space> =
	 *   hsl    |
	 *   hwb    |
	 *   lch    |
	 *   oklch
	 *
	 * <hue-interpolation-method> =
	 *   [ shorter | longer | increasing | decreasing ] hue
	 *
	 * <angular-color-stop> =
	 *   <color> <color-stop-angle>?
	 *
	 * <angular-color-hint> =
	 *   <angle-percentage>
	 *
	 * <color-stop-angle> =
	 *   <angle-percentage>{1,2}
	 *
	 * <angle-percentage> =
	 *   <angle>       |
	 *   <percentage>
	 * \endcode
	 * @param gradient_str
	 * @param container
	 * @param grad
	 */
	void parse_conic_gradient(const std::string& gradient_str, document_container *container, background_gradient& grad)
	{
		string_vector items;
		split_string(gradient_str, items, ",", "", "()");

		for (auto &item: items)
		{
			trim(item);
			string_vector parts;
			split_string(item, parts, split_delims_spaces, "", "()");
			if (parts.empty()) continue;

			// Parse colors stop list
			if (web_color::is_color(parts[0], container))
			{
				parse_color_stop_angle_list(parts, container, grad);
				continue;
			}

			size_t i = 0;
			while(i < parts.size())
			{
				// parse position
				if(parts[i] == "at")
				{
					parse_radial_position(grad, parts, i + 1);
					break;
				}
				// parse "from angle"
				if(parts[i] == "from")
				{
					i++;
					if(i >= parts.size()) continue;
					parse_css_angle(parts[i], false, grad.conic_from_angle);
					continue;
				}
				if(parts[i] == "in")
				{
					i++;
					if(i >= parts.size()) continue;
					int val = value_index(parts[i], "srgb;"
													"srgb-linear;"
													"display-p3;"
													"a98-rgb;"
													"prophoto-rgb;"
													"rec2020;"
													"lab;"
													"oklab;"
													"xyz;"
													"xyz-d50;"
													"xyz-d65");
					if(val >= 0)
					{
						grad.conic_color_space = (decltype(grad.conic_color_space)) (val + 1);
						continue;
					}
					val = value_index(parts[i], "hsl;"
												"hwb;"
												"lch;"
												"oklch");
					if(val < 0) continue;

					grad.conic_color_space = (decltype(grad.conic_color_space)) (background_gradient::conic_color_space_polar_start + 1 + val);
					int interpol = value_index(parts[i], "hue;shorter;longer;increasing;decreasing");
					if(interpol == 0)
					{
						grad.conic_interpolation = background_gradient::interpolation_method_hue;
						continue;
					}
					if(interpol > 0)
					{
						i++;
						if(i >= parts.size()) continue;
						if(parts[i] != "hue") continue;
						grad.conic_interpolation = (decltype(grad.conic_interpolation)) (val + 1);
						continue;
					}
				}
				i++;
			}
		}
	}
}