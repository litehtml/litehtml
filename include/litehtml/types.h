#ifndef LH_TYPES_H
#define LH_TYPES_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <list>
#include <set>
#include <variant>
#include <optional>
#include <algorithm>

namespace litehtml
{
	using uint_ptr = std::uintptr_t;
	using std::string;
	using std::vector;
	using std::shared_ptr;
	using std::make_shared;
	using std::optional;
	using std::min;
	using std::max;
	using std::swap;
	using std::abs;

	class document;
	class element;

	using string_map = std::map<string, string>;
	using elements_list = std::list<std::shared_ptr<element>>;
	using int_vector = std::vector<int>;
	using string_vector = std::vector<string>;

	template <class... Types>
	struct variant : std::variant<Types...>
	{
		using base = variant<Types...>; // for derived class ctors
		using std::variant<Types...>::variant; // inherit ctors
		template<class T> bool is() const { return std::holds_alternative<T>(*this); }
		template<class T> const T& get() const { return std::get<T>(*this); }
		template<class T> T& get() { return std::get<T>(*this); }
	};

	enum document_mode
	{
		no_quirks_mode,
		quirks_mode,
		limited_quirks_mode
	};

	const unsigned int font_decoration_none			= 0x00;
	const unsigned int font_decoration_underline	= 0x01;
	const unsigned int font_decoration_linethrough	= 0x02;
	const unsigned int font_decoration_overline		= 0x04;

	using byte = unsigned char;
	using ucode_t = unsigned int;

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

	struct pointF
	{
		float x;
		float y;

		pointF() : x(0), y(0) {}
		pointF(float _x, float _y) : x(_x), y(_y) {}

		void set(float _x, float _y) { x = _x; y = _y; }
	};

	struct size
	{
		int		width;
		int		height;

		size(int w, int h) : width(w), height(h)
		{
		}

		size() : width(0), height(0)
		{
		}
	};

	struct position
	{
		using vector = std::vector<position>;

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

		void move_to(int _x, int _y)
		{
			x = _x;
			y = _y;
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

		bool is_point_inside(int _x, int _y) const
		{
			if(_x >= left() && _x < right() && _y >= top() && _y < bottom())
			{
				return true;
			}
			return false;
		}
	};

	struct font_metrics
	{
		int 	font_size;
		int		height;
		int		ascent;
		int		descent;
		int		x_height;
		int 	ch_width;
		bool	draw_spaces;

		font_metrics()
		{
			font_size		= 0;
			height			= 0;
			ascent			= 0;
			descent			= 0;
			x_height		= 0;
			ch_width		= 0;
			draw_spaces		= true;
		}
		int base_line() const	{ return descent; }
	};

	struct font_item
	{
		uint_ptr		font;
		font_metrics	metrics;
	};

	using fonts_map = std::map<string, font_item>;

	enum draw_flag
	{
		draw_root,
		draw_block,
		draw_floats,
		draw_inlines,
		draw_positioned,
	};

	struct containing_block_context
	{
		enum cbc_value_type
		{
			cbc_value_type_absolute,	// width/height of containing block is defined as absolute value
			cbc_value_type_percentage,	// width/height of containing block is defined as percentage
			cbc_value_type_auto,		// width/height of containing block is defined as auto
			cbc_value_type_none,		// min/max width/height of containing block is defined as none
		};

		enum cbc_size_mode
		{
			size_mode_normal = 0x00,
			size_mode_exact_width = 0x01,
			size_mode_exact_height = 0x02,
			size_mode_content = 0x04,
		};

		struct typed_int
		{
			int 			value;
			cbc_value_type	type;

			typed_int(const typed_int& v) = default;

			typed_int(int val, cbc_value_type tp)
			{
				value = val;
				type = tp;
			}

			operator int() const
			{
				return value;
			}

			typed_int& operator=(int val)
			{
				value = val;
				return *this;
			}

			typed_int& operator=(const typed_int& v) = default;
		};

		typed_int width;						// width of the containing block
		typed_int render_width;
		typed_int min_width;
		typed_int max_width;

		typed_int height;						// height of the containing block
		typed_int min_height;
		typed_int max_height;

		int context_idx;
		uint32_t size_mode;

		containing_block_context() :
				width(0, cbc_value_type_auto),
				render_width(0, cbc_value_type_auto),
				min_width(0, cbc_value_type_none),
				max_width(0, cbc_value_type_none),
				height(0, cbc_value_type_auto),
				min_height(0, cbc_value_type_none),
				max_height(0, cbc_value_type_none),
				context_idx(0),
				size_mode(size_mode_normal)
		{}

		containing_block_context new_width(int w, uint32_t _size_mode = size_mode_normal) const
		{
			containing_block_context ret = *this;
			ret.render_width = w - (ret.width - ret.render_width);
			ret.width = w;
			ret.size_mode = _size_mode;
			return ret;
		}

		containing_block_context new_width_height(int w, int h, uint32_t _size_mode = size_mode_normal) const
		{
			containing_block_context ret = *this;
			ret.render_width = w - (ret.width - ret.render_width);
			ret.width = w;
			ret.height = h;
			ret.size_mode = _size_mode;
			return ret;
		}
	};

#define  style_display_strings		"none;block;inline;inline-block;inline-table;list-item;table;table-caption;table-cell;table-column;table-column-group;table-footer-group;table-header-group;table-row;table-row-group;inline-text;flex;inline-flex"

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
		display_flex,
		display_inline_flex,
	};

#define  font_size_strings		"xx-small;x-small;small;medium;large;x-large;xx-large;smaller;larger"

	enum font_size
	{
		font_size_xx_small,
		font_size_x_small,
		font_size_small,
		font_size_medium,
		font_size_large,
		font_size_x_large,
		font_size_xx_large,
		font_size_smaller,
		font_size_larger,
	};

#define line_height_strings "normal"

	enum line_height
	{
		line_height_normal
	};

#define  font_style_strings		"normal;italic"

	enum font_style
	{
		font_style_normal,
		font_style_italic
	};

#define  font_system_family_name_strings		"caption;icon;menu;message-box;small-caption;status-bar"

#define  font_variant_strings		"normal;small-caps"

	enum font_variant
	{
		font_variant_normal,
		font_variant_small_caps
	};

#define  font_weight_strings	"normal;bold;bolder;lighter"

	enum font_weight
	{
		font_weight_normal,
		font_weight_bold,
		font_weight_bolder,
		font_weight_lighter,
	};

#define  list_style_type_strings	"none;circle;disc;square;armenian;cjk-ideographic;decimal;decimal-leading-zero;georgian;hebrew;hiragana;hiragana-iroha;katakana;katakana-iroha;lower-alpha;lower-greek;lower-latin;lower-roman;upper-alpha;upper-latin;upper-roman"

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

#define  list_style_position_strings	"inside;outside"

	enum list_style_position
	{
		list_style_position_inside,
		list_style_position_outside
	};

#define  vertical_align_strings	"baseline;sub;super;top;text-top;middle;bottom;text-bottom"

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

#define  border_width_strings	"thin;medium;thick"

	enum border_width
	{
		border_width_thin,
		border_width_medium,
		border_width_thick
	};

	const float border_width_thin_value = 1;
	const float border_width_medium_value = 3;
	const float border_width_thick_value = 5;
	const float border_width_values[] = { border_width_thin_value, border_width_medium_value, border_width_thick_value };

#define  border_style_strings	"none;hidden;dotted;dashed;solid;double;groove;ridge;inset;outset"

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

#define  element_float_strings	"none;left;right"

	enum element_float
	{
		float_none,
		float_left,
		float_right
	};

#define  element_clear_strings	"none;left;right;both"

	enum element_clear
	{
		clear_none,
		clear_left,
		clear_right,
		clear_both
	};

#define  css_units_strings	"none;%;in;cm;mm;em;ex;pt;pc;px;vw;vh;vmin;vmax;rem;ch"

	enum css_units : byte // see css_length
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
		css_units_vw,
		css_units_vh,
		css_units_vmin,
		css_units_vmax,
		css_units_rem,
		css_units_ch,
	};

#define  background_attachment_strings	"scroll;fixed"

	enum background_attachment
	{
		background_attachment_scroll,
		background_attachment_fixed
	};

#define  background_repeat_strings	"repeat;repeat-x;repeat-y;no-repeat"

	enum background_repeat
	{
		background_repeat_repeat,
		background_repeat_repeat_x,
		background_repeat_repeat_y,
		background_repeat_no_repeat
	};

// https://drafts.csswg.org/css-box-4/#typedef-visual-box
#define  background_box_strings	"border-box;padding-box;content-box"

	enum background_box
	{
		background_box_border,
		background_box_padding,
		background_box_content
	};

#define  background_position_strings				"left;right;top;bottom;center"
	const float background_position_percentages[] =	 {0,   100,  0,  100,    50};

	enum background_position
	{
		background_position_left,
		background_position_right,
		background_position_top,
		background_position_bottom,
		background_position_center,
	};

#define element_position_strings	"static;relative;absolute;fixed"

	enum element_position
	{
		element_position_static,
		element_position_relative,
		element_position_absolute,
		element_position_fixed,
	};

#define text_align_strings		"left;right;center;justify"

	enum text_align
	{
		text_align_left,
		text_align_right,
		text_align_center,
		text_align_justify
	};

#define text_transform_strings		"none;capitalize;uppercase;lowercase"

	enum text_transform
	{
		text_transform_none,
		text_transform_capitalize,
		text_transform_uppercase,
		text_transform_lowercase
	};

#define white_space_strings		"normal;nowrap;pre;pre-line;pre-wrap"

	enum white_space
	{
		white_space_normal,
		white_space_nowrap,
		white_space_pre,
		white_space_pre_line,
		white_space_pre_wrap
	};

#define overflow_strings		"visible;hidden;scroll;auto;no-display;no-content"

	enum overflow
	{
		overflow_visible,
		overflow_hidden,
		overflow_scroll,
		overflow_auto,
		overflow_no_display,
		overflow_no_content
	};

#define background_size_strings		"auto;cover;contain"

	enum background_size
	{
		background_size_auto,	// must be first, see parse_bg_size
		background_size_cover,
		background_size_contain,
	};

#define visibility_strings			"visible;hidden;collapse"

	enum visibility
	{
		visibility_visible,
		visibility_hidden,
		visibility_collapse,
	};

#define border_collapse_strings		"collapse;separate"

	enum border_collapse
	{
		border_collapse_collapse,
		border_collapse_separate,
	};

#define content_property_string		"none;normal;open-quote;close-quote;no-open-quote;no-close-quote"

	enum content_property
	{
		content_property_none,
		content_property_normal,
		content_property_open_quote,
		content_property_close_quote,
		content_property_no_open_quote,
		content_property_no_close_quote,
	};

	class render_item;

	struct floated_box
	{
		position		                pos;
		element_float	                float_side;
		element_clear	                clear_floats;
		std::shared_ptr<render_item>	el;
		int								context;
		int 							min_width;

		floated_box() = default;
		floated_box(const floated_box& val)
		{
			pos = val.pos;
			float_side = val.float_side;
			clear_floats = val.clear_floats;
			el = val.el;
			context = val.context;
			min_width = val.min_width;
		}

		floated_box& operator=(const floated_box& val) = default;

		floated_box(floated_box&& val)
		{
			pos = val.pos;
			float_side = val.float_side;
			clear_floats = val.clear_floats;
			el = std::move(val.el);
			context = val.context;
			min_width = val.min_width;
		}
		void operator=(floated_box&& val)
		{
			pos = val.pos;
			float_side = val.float_side;
			clear_floats = val.clear_floats;
			el = std::move(val.el);
			context = val.context;
			min_width = val.min_width;
		}
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
		def_value(const def_value<T>& val)
		{
			m_is_default	= val.m_is_default;
			m_val			= val.m_val;
		}
		void reset(T def_val)
		{
			m_is_default	= true;
			m_val			= def_val;
		}
		bool is_default() const
		{
			return m_is_default;
		}
		T operator=(T new_val)
		{
			m_val			= new_val;
			m_is_default	= false;
			return m_val;
		}
		def_value<T>& operator=(const def_value<T>& val)
		{
			m_is_default	= val.m_is_default;
			m_val			= val.m_val;
			return *this;
		}
		operator T() const
		{
			return m_val;
		}
	};

	class baseline
	{
	public:
		enum _baseline_type
		{
			baseline_type_none,
			baseline_type_top,
			baseline_type_bottom,
		};

	public:
		baseline() : m_value(0), m_type(baseline_type_none) {}
		baseline(int _value, _baseline_type _type) : m_value(_value), m_type(_type) {}

		int value() const				{ return m_value; 	}
		void value(int _value) 			{ m_value = _value; }
		_baseline_type type() const		{ return m_type; 	}
		void type(_baseline_type _type)	{ m_type = _type; 	}

		operator int() const	{ return m_value; 	}
		baseline& operator=(int _value) { m_value = _value; return *this; }

		void set(int _value, _baseline_type _type)	{ m_value = _value; m_type =_type; }
		/**
		 * Get baseline offset from top of element with specified height
		 * @param height - element height
		 * @return baseline offset
		 */
		int get_offset_from_top(int height) const
		{
			if(m_type == baseline_type_top) return m_value;
			return height - m_value;
		}
		/**
		 * Get baseline offset from bottom of element with specified height
		 * @param height - element height
		 * @return baseline offset
		 */
		int get_offset_from_bottom(int height) const
		{
			if(m_type == baseline_type_bottom) return m_value;
			return height - m_value;
		}
		/**
		 * Calculate baseline by top and bottom positions of element aligned by baseline == 0
		 * @param top - top of the aligned element
		 * @param bottom - bottom of the aligned element
		 */
		void calc(int top, int bottom)
		{
			if(m_type == baseline_type_top)
				m_value = -top;
			else if(m_type == baseline_type_bottom)
				m_value = bottom;
		}
	private:
		int m_value;
		_baseline_type m_type;
	};

#define appearance_strings      "none;auto;menulist-button;textfield;button;checkbox;listbox;menulist;meter;progress-bar;push-button;radio;searchfield;slider-horizontal;square-button;textarea"

	enum appearance
	{
		appearance_none,
		appearance_auto,
		appearance_menulist_button,
		appearance_textfield,
		appearance_button,
		appearance_checkbox,
		appearance_listbox,
		appearance_menulist,
		appearance_meter,
		appearance_progress_bar,
		appearance_push_button,
		appearance_radio,
		appearance_searchfield,
		appearance_slider_horizontal,
		appearance_square_button,
		appearance_textarea,
	};

#define box_sizing_strings		"content-box;border-box"

	enum box_sizing
	{
		box_sizing_content_box,
		box_sizing_border_box,
	};

// https://drafts.csswg.org/mediaqueries/#media-types
// User agents must recognize the following media types as valid, but must make them match nothing.
#define deprecated_media_type_strings	"tty;tv;projection;handheld;braille;embossed;aural;speech"
#define media_type_strings				"all;print;screen;" deprecated_media_type_strings

	enum media_type
	{
		media_type_unknown,
		media_type_all,
		media_type_print,
		media_type_screen,
		media_type_first_deprecated
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

		media_features()
		{
			type = media_type_unknown;
			width = 0;
			height = 0;
			device_width = 0;
			device_height = 0;
			color = 0;
			color_index = 0;
			monochrome = 0;
			resolution = 0;
		}
	};

	enum render_type
	{
		render_all,
		render_no_fixed,
		render_fixed_only,
	};

	const char* const split_delims_spaces = " \t\r\n\f\v";

	// List of the Void Elements (can't have any contents)
	const char* const void_elements = "area;base;br;col;command;embed;hr;img;input;keygen;link;meta;param;source;track;wbr";

#define flex_direction_strings		"row;row-reverse;column;column-reverse"

	enum flex_direction
	{
		flex_direction_row,
		flex_direction_row_reverse,
		flex_direction_column,
		flex_direction_column_reverse
	};

#define flex_wrap_strings		"nowrap;wrap;wrap-reverse"

	enum flex_wrap
	{
		flex_wrap_nowrap,
		flex_wrap_wrap,
		flex_wrap_wrap_reverse
	};

#define flex_justify_content_strings		"normal;flex-start;flex-end;center;space-between;space-around;start;end;left;right;space-evenly;stretch"

	enum flex_justify_content
	{
		flex_justify_content_normal,
		flex_justify_content_flex_start,
		flex_justify_content_flex_end,
		flex_justify_content_center,
		flex_justify_content_space_between,
		flex_justify_content_space_around,
		flex_justify_content_start,
		flex_justify_content_end,
		flex_justify_content_left,
		flex_justify_content_right,
		flex_justify_content_space_evenly,
		flex_justify_content_stretch,
	};

#define self_position_strings		"center;start;end;self-start;self-end;flex-start;flex-end"
#define flex_align_items_strings	"auto;normal;stretch;baseline;" self_position_strings

	enum flex_align_items
	{
		flex_align_items_auto, // used for align-self property only
		flex_align_items_normal,
		flex_align_items_stretch,
		flex_align_items_baseline,

		flex_align_items_center,
		flex_align_items_start,
		flex_align_items_end,
		flex_align_items_self_start,
		flex_align_items_self_end,
		flex_align_items_flex_start,
		flex_align_items_flex_end,

		flex_align_items_first = 0x100,
		flex_align_items_last = 0x200,
		flex_align_items_unsafe = 0x400,
		flex_align_items_safe  = 0x800,
	};

#define flex_align_content_strings		"flex-start;start;flex-end;end;center;space-between;space-around;stretch"

	enum flex_align_content
	{
		flex_align_content_flex_start,
		flex_align_content_start,
		flex_align_content_flex_end,
		flex_align_content_end,
		flex_align_content_center,
		flex_align_content_space_between,
		flex_align_content_space_around,
		flex_align_content_stretch
	};

#define flex_basis_strings		"auto;content;fit-content;min-content;max-content"

	enum flex_basis
	{
		flex_basis_auto,
		flex_basis_content,
		flex_basis_fit_content,
		flex_basis_min_content,
		flex_basis_max_content,
	};

#define caption_side_strings		"top;bottom"

	enum caption_side
	{
		caption_side_top,
		caption_side_bottom
	};
}

#endif  // LH_TYPES_H
