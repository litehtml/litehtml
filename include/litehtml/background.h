#ifndef LH_BACKGROUND_H
#define LH_BACKGROUND_H

#include "types.h"
#include "css_length.h"
#include "css_position.h"
#include "web_color.h"
#include "borders.h"

namespace litehtml
{
	class background_gradient
	{
	public:
		enum gradient_type
		{
			no_gradient,
			linear_gradient,
			repeating_linear_gradient,
			radial_gradient,
			repeating_radial_gradient,
		};
		enum gradient_side
		{
			gradient_side_none = 0,
			gradient_side_left = 0x01,
			gradient_side_right = 0x02,
			gradient_side_top = 0x04,
			gradient_side_bottom = 0x08,
		};
		enum radial_shape
		{
			radial_shape_none,
			radial_shape_circle,
			radial_shape_ellipse,
		};
		enum radial_extent
		{
			radial_extent_none,
			radial_extent_closest_corner,
			radial_extent_closest_side,
			radial_extent_farthest_corner,
			radial_extent_farthest_side,
		};
		class gradient_color
		{
		public:
			bool is_color_hint;
			web_color	color;
			css_length	length;

			gradient_color() :
					is_color_hint(false)
			{}
		};
		gradient_type m_type;
		uint32_t m_side;
		double angle;
		std::vector<gradient_color> m_colors;

		explicit background_gradient(gradient_type type = no_gradient)
		{
			m_type = type;
			m_side = gradient_side_none;
			angle = 0;
		}

		bool is_empty() const
		{
			return m_type == no_gradient || m_colors.empty();
		}

		static background_gradient transparent;
	};

	class background_image
	{
	public:
		enum bg_image_type_t
		{
			bg_image_type_none,
			bg_image_type_url,
			bg_image_type_gradient,
		};
		bg_image_type_t type;
		std::string url;
		background_gradient gradient;

		background_image() : type(bg_image_type_none) {}
		bool is_empty() const
		{
			switch (type)
			{
				case bg_image_type_none:
					return false;
				case bg_image_type_url:
					return url.empty();
				case bg_image_type_gradient:
					return gradient.is_empty();
			}
			return true;
		}
	};

	class background
	{
	public:
		std::vector<background_image> m_image;
		string					m_baseurl;
		web_color				m_color;
		int_vector				m_attachment;
		length_vector			m_position_x;
		length_vector			m_position_y;
		size_vector				m_size;
		int_vector				m_repeat;
		int_vector				m_clip;
		int_vector				m_origin;

		bool is_empty() const
		{
			if(m_color.alpha != 0)
				return false;
			if(m_image.empty())
				return true;
			for(const auto& img : m_image)
			{
				if(!img.is_empty()) return false;
			}
			return true;
		}
	};

	class background_paint
	{
	public:
		string					image;
		string					baseurl;
		background_attachment	attachment;
		background_repeat		repeat;
		web_color				color;
		background_gradient		gradient;
		position				clip_box;
		position				origin_box;
		position				border_box;
		border_radiuses			border_radius;
		size					image_size;
		int						position_x;
		int						position_y;
		bool					is_root;

	public:
		background_paint()
		{
			attachment		= background_attachment_scroll;
			repeat			= background_repeat_repeat;
			color			= web_color::transparent;
			position_x		= 0;
			position_y		= 0;
			is_root			= false;
		}
	};

}

#endif  // LH_BACKGROUND_H
