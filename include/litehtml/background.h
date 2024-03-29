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
			conic_gradient,
			repeating_conic_gradient,
		};
		enum gradient_side
		{
			gradient_side_none = 0,
			gradient_side_left = 0x01,
			gradient_side_right = 0x02,
			gradient_side_top = 0x04,
			gradient_side_bottom = 0x08,
			gradient_side_x_center = 0x10,
			gradient_side_y_center = 0x20,
			gradient_side_x_length = 0x40,
			gradient_side_y_length = 0x80,
		};
		enum radial_shape_t
		{
			radial_shape_none,
			radial_shape_circle,
			radial_shape_ellipse,
		};
		enum radial_extent_t
		{
			radial_extent_none,
			radial_extent_closest_corner,
			radial_extent_closest_side,
			radial_extent_farthest_corner,
			radial_extent_farthest_side,
		};
		enum conic_color_space_t
		{
			conic_color_space_none,
			// rectangular-color-space
			conic_color_space_srgb,
			conic_color_space_srgb_linear,
			conic_color_space_display_p3,
			conic_color_space_a98_rgb,
			conic_color_space_prophoto_rgb,
			conic_color_space_rec2020,
			conic_color_space_lab,
			conic_color_space_oklab,
			conic_color_space_xyz,
			conic_color_space_xyz_d50,
			conic_color_space_xyz_d65,

			// polar-color-space
			conic_color_space_polar_start,
			conic_color_space_hsl,
			conic_color_space_hwb,
			conic_color_space_lch,
			conic_color_space_oklch,
		};
		enum interpolation_method_t
		{
			interpolation_method_none,
			interpolation_method_hue,
			interpolation_method_shorter_hue,
			interpolation_method_longer_hue,
			interpolation_method_increasing_hue,
			interpolation_method_decreasing_hue
		};

		class gradient_color
		{
		public:
			bool is_color_hint;
			web_color	color;
			css_length	length;
			def_value<float> angle;

			gradient_color() :
					is_color_hint(false), angle(0)
			{}
		};
		gradient_type m_type;
		uint32_t m_side;
		float angle;
		std::vector<gradient_color> m_colors;
		css_length radial_position_x;
		css_length radial_position_y;
		radial_extent_t radial_extent;
		radial_shape_t radial_shape;
		css_length radial_length_x;
		css_length radial_length_y;
		float conic_from_angle;
		conic_color_space_t conic_color_space;
		interpolation_method_t conic_interpolation;


		explicit background_gradient(gradient_type type = no_gradient)
		{
			m_type = type;
			m_side = gradient_side_none;
			angle = 180;
			radial_extent = radial_extent_none;
			radial_shape = radial_shape_ellipse;
			radial_length_x.predef(0);
			radial_length_y.predef(0);
			radial_position_x.predef(0);
			radial_position_y.predef(0);
			conic_from_angle = 0;
			conic_color_space = conic_color_space_none;
			conic_interpolation = interpolation_method_none;
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

	class background_layer
	{
	public:
		// border_box defines draw boundary. Everything must be drawn inside this rectangle only.
		position border_box;
		// border_radius defines radius of the border_box.
		border_radiuses border_radius;
		// clip_box defines clipping rectangle. Works like border_box. Container must set additional clipping.
		position clip_box;
		// origin_box defines origin rectangle.
		position origin_box;
		background_attachment attachment;
		background_repeat repeat;
		// is_root is true for root element. Container can use this flag to apply background to the top window.
		bool is_root;

		background_layer() :
				attachment(background_attachment_scroll),
				repeat(background_repeat_repeat),
				is_root(false)
				{}

		class image
		{
		public:
			std::string url;
			std::string base_url;
		};

		struct color_point
		{
			float offset;
			web_color color;
			color_point() { offset = 0.0; }
			color_point(float _offset, web_color _color) : offset(_offset), color(_color) {}
		};

		class color
		{
		public:
			web_color color;
		};

		class gradient_base
		{
		public:
			std::vector<color_point> color_points;

			void color_points_transparent_fix();
			bool prepare_color_points(float len, background_gradient::gradient_type g_type, const std::vector<background_gradient::gradient_color>& colors);
			bool prepare_angle_color_points(background_gradient::gradient_type g_type, const std::vector<background_gradient::gradient_color>& colors);
		};

		class linear_gradient : public gradient_base
		{
		public:
			pointF start;
			pointF end;
		};

		class radial_gradient : public gradient_base
		{
		public:
			pointF position;
			pointF radius;
		};

		class conic_gradient : public gradient_base
		{
		public:
			pointF position;
			float angle;
			background_gradient::conic_color_space_t color_space;
			background_gradient::interpolation_method_t interpolation;
			conic_gradient() : angle(0),
							   color_space(background_gradient::conic_color_space_none),
							   interpolation(background_gradient::interpolation_method_none)
			{}
		};
	};

	class background
	{
	public:
		enum layer_type
		{
			type_none,
			type_color,
			type_image,
			type_linear_gradient,
			type_radial_gradient,
			type_conic_gradient,
		};

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
		int get_layers_number() const
		{
			if(m_color != web_color::transparent)
			{
				return (int) m_image.size() + 1;
			}
			return (int) m_image.size();
		}
		bool get_layer(int idx, position pos, const element* el, const std::shared_ptr<render_item>& ri, background_layer& layer) const;
		layer_type get_layer_type(int idx) const;
		std::unique_ptr<background_layer::image> get_image_layer(int idx) const;
		std::unique_ptr<background_layer::color> get_color_layer(int idx) const;
		std::unique_ptr<background_layer::linear_gradient> get_linear_gradient_layer(int idx, const background_layer& layer) const;
		std::unique_ptr<background_layer::radial_gradient> get_radial_gradient_layer(int idx, const background_layer& layer) const;
		std::unique_ptr<background_layer::conic_gradient> get_conic_gradient_layer(int idx, const background_layer& layer) const;
		void draw_layer(uint_ptr hdc, int idx, const background_layer& layer, document_container* container) const;
	};
}

#endif  // LH_BACKGROUND_H
