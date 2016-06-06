#pragma once

#include <stdlib.h>
#include <map>
#include <set>
#include <vector>
#include "object.h"
#include "os_types.h"

namespace litehtml
{
	class document;
	class element;
	class string_hash;

	typedef std::map<litehtml::tstring, litehtml::tstring>			string_map;
	typedef std::map<litehtml::string_hash, litehtml::tstring>      strings_hash_map;
	typedef std::vector< litehtml::object_ptr<litehtml::element> >	elements_vector;
	typedef std::vector<int>										int_vector;
	typedef std::vector<litehtml::tstring>							string_vector;
	typedef std::vector<litehtml::string_hash>						string_hash_vector;
	typedef std::set<litehtml::string_hash>							string_hash_set;

	const unsigned int font_decoration_none			= 0x00;
	const unsigned int font_decoration_underline	= 0x01;
	const unsigned int font_decoration_linethrough	= 0x02;
	const unsigned int font_decoration_overline		= 0x04;

	typedef unsigned char	byte;
	typedef unsigned int	ucode_t;

	typedef unsigned int	hash_code;

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

	struct point
	{
		int x;
		int y;

		point() :
			x(0),
			y(0)
		{
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

	struct font_metrics
	{
		int		height;
		int		ascent;
		int		descent;
		int		x_height;
		bool	draw_spaces;

		font_metrics()
		{
			height			= 0;
			ascent			= 0;
			descent			= 0;
			x_height		= 0;
			draw_spaces		= true;
		}
		int base_line()	{ return descent; }
	};

	struct font_item
	{
		uint_ptr		font;
		font_metrics	metrics;
	};

	typedef std::map<tstring, font_item>	fonts_map;

	enum draw_flag
	{
		draw_root,
		draw_block,
		draw_floats,
		draw_inlines,
		draw_positioned,
	};

extern const std::vector<string_hash> style_display_strings;

	enum style_display
	{
		display_none,
		display_block,
		display_inline,
		display_inline_block,
		display_inline_table,
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

    extern const std::vector<string_hash> font_size_strings;

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

extern const std::vector<string_hash> font_style_strings;

	enum font_style
	{
		fontStyleNormal,
		fontStyleItalic
	};

extern const std::vector<string_hash> font_variant_strings;

	enum font_variant
	{
		font_variant_normal,
		font_variant_italic
	};

extern const std::vector<string_hash> font_weight_strings;

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

extern const std::vector<string_hash> list_style_type_strings;

	enum list_style_type
	{
		list_style_type_none,
		list_style_type_circle,
		list_style_type_disc,
		list_style_type_square,
		list_style_type_armenian,
		list_style_type_cjk_ideographic,
		list_style_type_decimal,
		list_style_type_decimal_leading_zero,
		list_style_type_georgian,
		list_style_type_hebrew,
		list_style_type_hiragana,
		list_style_type_hiragana_iroha,
		list_style_type_katakana,
		list_style_type_katakana_iroha,
		list_style_type_lower_alpha,
		list_style_type_lower_greek,
		list_style_type_lower_latin,
		list_style_type_lower_roman,
		list_style_type_upper_alpha,
		list_style_type_upper_latin,
		list_style_type_upper_roman,
	};

extern const std::vector<string_hash> list_style_position_strings;

	enum list_style_position
	{
		list_style_position_inside,
		list_style_position_outside
	};

extern const std::vector<string_hash> vertical_align_strings;

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

extern const std::vector<string_hash> border_width_strings;

	enum border_width
	{
		border_width_thin,
		border_width_medium,
		border_width_thick
	};

extern const std::vector<string_hash> border_style_strings;

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

extern const std::vector<string_hash> element_float_strings;

	enum element_float
	{
		float_none,
		float_left,
		float_right
	};

extern const std::vector<string_hash> element_clear_strings;

	enum element_clear
	{
		clear_none,
		clear_left,
		clear_right,
		clear_both
	};

extern const std::vector<string_hash> css_units_strings;

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
		css_units_px,
		css_units_dpi,
		css_units_dpcm,
		css_units_vw,
		css_units_vh,
		css_units_vmin,
		css_units_vmax,
	};

extern const std::vector<string_hash> background_attachment_strings;

	enum background_attachment
	{
		background_attachment_scroll,
		background_attachment_fixed
	};

extern const std::vector<string_hash> background_repeat_strings;

	enum background_repeat
	{
		background_repeat_repeat,
		background_repeat_repeat_x,
		background_repeat_repeat_y,
		background_repeat_no_repeat
	};

extern const std::vector<string_hash> background_box_strings;

	enum background_box
	{
		background_box_border,
		background_box_padding,
		background_box_content
	};

extern const std::vector<string_hash> element_position_strings;

	enum element_position
	{
		element_position_static,
		element_position_relative,
		element_position_absolute,
		element_position_fixed,
	};

extern const std::vector<string_hash> text_align_strings;

	enum text_align
	{
		text_align_left,
		text_align_right,
		text_align_center,
		text_align_justify
	};

extern const std::vector<string_hash> text_transform_strings;

	enum text_transform
	{
		text_transform_none,
		text_transform_capitalize,
		text_transform_uppercase,
		text_transform_lowercase
	};

extern const std::vector<string_hash> white_space_strings;

	enum white_space
	{
		white_space_normal,
		white_space_nowrap,
		white_space_pre,
		white_space_pre_line,
		white_space_pre_wrap
	};

extern const std::vector<string_hash> overflow_strings;

	enum overflow
	{
		overflow_visible,
		overflow_hidden,
		overflow_scroll,
		overflow_auto,
		overflow_no_display,
		overflow_no_content
	};

extern const std::vector<string_hash> background_size_strings;

	enum background_size
	{
		background_size_auto,
		background_size_cover,
		background_size_contain,
	};

extern const std::vector<string_hash> visibility_strings;

	enum visibility
	{
		visibility_visible,
		visibility_hidden,
		visibility_collapse,
	};

extern const std::vector<string_hash> border_collapse_strings;

	enum border_collapse
	{
		border_collapse_collapse,
		border_collapse_separate,
	};


extern const std::vector<string_hash> pseudo_class_strings;

	enum pseudo_class
	{
		pseudo_class_only_child,
		pseudo_class_only_of_type,
		pseudo_class_first_child,
		pseudo_class_first_of_type,
		pseudo_class_last_child,
		pseudo_class_last_of_type,
		pseudo_class_nth_child,
		pseudo_class_nth_of_type,
		pseudo_class_nth_last_child,
		pseudo_class_nth_last_of_type,
		pseudo_class_not,
		pseudo_class_lang,
	};

extern const std::vector<string_hash> content_property_string;

	enum content_property
	{
		content_property_none,
		content_property_normal,
		content_property_open_quote,
		content_property_close_quote,
		content_property_no_open_quote,
		content_property_no_close_quote,
	};


	struct floated_box
	{
		typedef std::vector<floated_box>	vector;

		position		pos;
		element_float	float_side;
		element_clear	clear_floats;
		element*		el;
	};

	struct int_int_cache
	{
		int		hash;
		int		val;
		bool	is_valid;
		bool	is_default;

		int_int_cache()
		{
			hash		= 0;
			val			= 0;
			is_valid	= false;
			is_default	= false;
		}
		void invalidate()
		{
			is_valid	= false;
			is_default	= false;
		}
		void set_value(int vHash, int vVal)
		{
			hash		= vHash;
			val			= vVal;
			is_valid	= true;
		}
	};

	enum select_result
	{
		select_no_match				= 0x00,
		select_match				= 0x01,
		select_match_pseudo_class	= 0x02,
		select_match_with_before	= 0x10,
		select_match_with_after		= 0x20,
	};

	template<class T>
	class def_value
	{
		T		m_val;
		bool	m_is_default;
	public:
		def_value(T def_val)
		{
			m_is_default	= true;
			m_val			= def_val;
		}
		void reset(T def_val)
		{
			m_is_default	= true;
			m_val			= def_val;
		}
		bool is_default()
		{
			return m_is_default;
		}
		T operator=(T new_val)
		{
			m_val			= new_val;
			m_is_default	= false;
			return m_val;
		}
		operator T()
		{
			return m_val;
		}
	};


extern const std::vector<string_hash> media_orientation_strings;

	enum media_orientation
	{
		media_orientation_portrait,
		media_orientation_landscape,
	};

extern const std::vector<string_hash> media_feature_strings;

	enum media_feature
	{
		media_feature_none,

		media_feature_width,
		media_feature_min_width,
		media_feature_max_width,

		media_feature_height,
		media_feature_min_height,
		media_feature_max_height,

		media_feature_device_width,
		media_feature_min_device_width,
		media_feature_max_device_width,

		media_feature_device_height,
		media_feature_min_device_height,
		media_feature_max_device_height,

		media_feature_orientation,

		media_feature_aspect_ratio,
		media_feature_min_aspect_ratio,
		media_feature_max_aspect_ratio,

		media_feature_device_aspect_ratio,
		media_feature_min_device_aspect_ratio,
		media_feature_max_device_aspect_ratio,

		media_feature_color,
		media_feature_min_color,
		media_feature_max_color,

		media_feature_color_index,
		media_feature_min_color_index,
		media_feature_max_color_index,

		media_feature_monochrome,
		media_feature_min_monochrome,
		media_feature_max_monochrome,

		media_feature_resolution,
		media_feature_min_resolution,
		media_feature_max_resolution,
	};

extern const std::vector<string_hash> box_sizing_strings;

	enum box_sizing
	{
		box_sizing_content_box,
		box_sizing_border_box,
	};

extern const std::vector<string_hash> pointer_events_strings;

	enum pointer_events
	{
		pointer_events_auto,
		pointer_events_none,
		pointer_events_visiblePainted,
		pointer_events_visibleFill,
		pointer_events_visibleStroke,
		pointer_events_visible,
		pointer_events_painted,
		pointer_events_fill,
		pointer_events_stroke,
		pointer_events_all
	};

extern const std::vector<string_hash> media_type_strings;

	enum media_type
	{
		media_type_none,
		media_type_all,
		media_type_screen,
		media_type_print,
		media_type_braille,
		media_type_embossed,
		media_type_handheld,
		media_type_projection,
		media_type_speech,
		media_type_tty,
		media_type_tv,
	};

	struct media_features
	{
		media_type	type;
		int			width;			// (pixels) For continuous media, this is the width of the viewport including the size of a rendered scroll bar (if any). For paged media, this is the width of the page box.
		int			height;			// (pixels) The height of the targeted display area of the output device. For continuous media, this is the height of the viewport including the size of a rendered scroll bar (if any). For paged media, this is the height of the page box.
		int			device_width;	// (pixels) The width of the rendering surface of the output device. For continuous media, this is the width of the screen. For paged media, this is the width of the page sheet size.
		int			device_height;	// (pixels) The height of the rendering surface of the output device. For continuous media, this is the height of the screen. For paged media, this is the height of the page sheet size.
		int			color;			// The number of bits per color component of the output device. If the device is not a color device, the value is zero.
		int			color_index;	// The number of entries in the color lookup table of the output device. If the device does not use a color lookup table, the value is zero.
		int			monochrome;		// The number of bits per pixel in a monochrome frame buffer. If the device is not a monochrome device, the output device value will be 0.
		int			resolution;		// The resolution of the output device (in DPI)
	};

	enum render_type
	{
		render_all,
		render_no_fixed,
		render_fixed_only,
	};

	// List of the Void Elements (can't have any contents)
	extern const std::vector<string_hash> void_elements;
}
