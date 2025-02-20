#ifndef LITEHTML_CSS_PROPERTIES_H
#define LITEHTML_CSS_PROPERTIES_H

#include "os_types.h"
#include "types.h"
#include "css_margins.h"
#include "borders.h"
#include "css_offsets.h"
#include "background.h"
#include "css_transition.h"
#include "css_transform.h"

namespace litehtml
{
	class html_tag;
	class document;

	class css_properties
	{
	private:
		element_position		m_el_position;
		text_align				m_text_align;
		overflow				m_overflow;
		white_space				m_white_space;
		style_display			m_display;
		visibility				m_visibility;
		appearance				m_appearance;
		box_sizing				m_box_sizing;
		css_length				m_z_index;
		vertical_align			m_vertical_align;
		element_float			m_float;
		element_clear			m_clear;
		css_margins				m_css_margins;
		css_margins				m_css_padding;
		css_borders				m_css_borders;
		css_length				m_css_width;
		css_length				m_css_height;
		css_length				m_css_min_width;
		css_length				m_css_min_height;
		css_length				m_css_max_width;
		css_length				m_css_max_height;
		css_offsets				m_css_offsets;
		css_length				m_css_text_indent;
		css_length				m_css_line_height;
		int						m_line_height;
		list_style_type			m_list_style_type;
		list_style_position		m_list_style_position;
		string					m_list_style_image;
		string					m_list_style_image_baseurl;
		background				m_bg;
		uint_ptr				m_font;
		css_length				m_font_size;
		string					m_font_family;
		css_length				m_font_weight;
		font_style				m_font_style;
		string					m_text_decoration;
		font_metrics			m_font_metrics;
		text_transform			m_text_transform;
		web_color				m_color;
		float					m_opacity;
		string					m_cursor;
		string					m_content;
		border_collapse			m_border_collapse;
		css_length				m_css_border_spacing_x;
		css_length				m_css_border_spacing_y;

		float					m_flex_grow;
		float					m_flex_shrink;
		css_length				m_flex_basis;
		flex_direction			m_flex_direction;
		flex_wrap				m_flex_wrap;
		flex_justify_content	m_flex_justify_content;
		flex_align_items		m_flex_align_items;
		flex_align_items		m_flex_align_self;
		flex_align_content		m_flex_align_content;

		caption_side			m_caption_side;

		string m_animation_name{};
		time m_animation_duration{};
		time m_animation_delay{};
		animation_direction m_animation_direction{animation_direction_normal};
		animation_fill_mode m_animation_fill_mode{animation_fill_mode_none};
		animation_play_state m_animation_play_state{animation_play_state_running};
		timing_function_type m_animation_timing_function{timing_function_ease};
		float m_animation_iteration_count{1.0f};

		string_vector m_transition_properties;
		std::vector<time> m_transition_duration{};
		std::vector<time> m_transition_delay{};
		std::vector<timing_function_type> m_transition_timing_function{};
		std::vector<transition_behavior> m_transition_behavior{};

		css_transform m_transform;
		length_vector m_transform_origin;
		transform_style m_transform_style;
		css_length m_perspective;
		length_vector m_perspective_origin;
		backface_visibility m_backface_visibility;

		int 					m_order;

	private:
		void compute_font(const html_tag* el, const std::shared_ptr<document>& doc);
		void compute_background(const html_tag* el, const std::shared_ptr<document>& doc);
		void compute_flex(const html_tag* el, const std::shared_ptr<document>& doc);
		void compute_animation(const html_tag* el, const std::shared_ptr<document>& doc);
		void compute_transition(const html_tag *el, const std::shared_ptr<document> &doc);
		web_color get_color_property(const html_tag* el, string_id name, bool inherited, web_color default_value, uint_ptr member_offset) const;

	public:
		css_properties() :
				m_el_position(element_position_static),
				m_text_align(text_align_left),
				m_overflow(overflow_visible),
				m_white_space(white_space_normal),
				m_display(display_inline),
				m_visibility(visibility_visible),
				m_appearance(appearance_none),
				m_box_sizing(box_sizing_content_box),
				m_z_index(0),
				m_vertical_align(va_baseline),
				m_float(float_none),
				m_clear(clear_none),
				m_css_margins(),
				m_css_padding(),
				m_css_borders(),
				m_css_width(),
				m_css_height(),
				m_css_min_width(),
				m_css_min_height(),
				m_css_max_width(),
				m_css_max_height(),
				m_css_offsets(),
				m_css_text_indent(),
				m_css_line_height(0),
				m_line_height(0),
				m_list_style_type(list_style_type_none),
				m_list_style_position(list_style_position_outside),
				m_bg(),
				m_font(0),
				m_font_size(0),
				m_font_metrics(),
				m_text_transform(text_transform_none),
				m_color(),
				m_opacity(1.0f),
				m_border_collapse(border_collapse_separate),
				m_css_border_spacing_x(),
				m_css_border_spacing_y(),
				m_flex_grow(0),
				m_flex_shrink(1),
				m_flex_direction(flex_direction_row),
				m_flex_wrap(flex_wrap_nowrap),
				m_flex_justify_content(flex_justify_content_flex_start),
				m_flex_align_items(flex_align_items_stretch),
				m_flex_align_self(flex_align_items_auto),
				m_flex_align_content(flex_align_content_stretch),
				m_order(0)
		{}

		void compute(const html_tag* el, const std::shared_ptr<document>& doc);
		std::vector<std::tuple<string, string>> dump_get_attrs();

		element_position get_position() const;
		void set_position(element_position mElPosition);

		text_align get_text_align() const;
		void set_text_align(text_align mTextAlign);

		overflow get_overflow() const;
		void set_overflow(overflow mOverflow);

		white_space get_white_space() const;
		void set_white_space(white_space mWhiteSpace);

		style_display get_display() const;
		void set_display(style_display mDisplay);

		visibility get_visibility() const;
		void set_visibility(visibility mVisibility);

		appearance get_appearance() const;
		void set_appearance(appearance mAppearance);

		box_sizing get_box_sizing() const;
		void set_box_sizing(box_sizing mBoxSizing);

		int get_z_index() const;
		void set_z_index(int mZIndex);

		vertical_align get_vertical_align() const;
		void set_vertical_align(vertical_align mVerticalAlign);

		element_float get_float() const;
		void set_float(element_float mFloat);

		element_clear get_clear() const;
		void set_clear(element_clear mClear);

		const css_margins &get_margins() const;
		void set_margins(const css_margins &mCssMargins);

		const css_margins &get_padding() const;
		void set_padding(const css_margins &mCssPadding);

		const css_borders &get_borders() const;
		void set_borders(const css_borders &mCssBorders);

		const css_length &get_width() const;
		void set_width(const css_length &mCssWidth);

		const css_length &get_height() const;
		void set_height(const css_length &mCssHeight);

		const css_length &get_min_width() const;
		void set_min_width(const css_length &mCssMinWidth);

		const css_length &get_min_height() const;
		void set_min_height(const css_length &mCssMinHeight);

		const css_length &get_max_width() const;
		void set_max_width(const css_length &mCssMaxWidth);

		const css_length &get_max_height() const;
		void set_max_height(const css_length &mCssMaxHeight);

		const css_offsets &get_offsets() const;
		void set_offsets(const css_offsets &mCssOffsets);

		const css_length &get_text_indent() const;
		void set_text_indent(const css_length &mCssTextIndent);

		int get_line_height() const;
		void set_line_height(int mLineHeight);

		list_style_type get_list_style_type() const;
		void set_list_style_type(list_style_type mListStyleType);

		list_style_position get_list_style_position() const;
		void set_list_style_position(list_style_position mListStylePosition);

		string get_list_style_image() const;
		void set_list_style_image(const string& url);

		string get_list_style_image_baseurl() const;
		void set_list_style_image_baseurl(const string& url);

		const background &get_bg() const;
		void set_bg(const background &mBg);

		int get_font_size() const;
		void set_font_size(int mFontSize);

		uint_ptr get_font() const;
		void set_font(uint_ptr mFont);

		const font_metrics& get_font_metrics() const;
		void set_font_metrics(const font_metrics& mFontMetrics);

		text_transform get_text_transform() const;
		void set_text_transform(text_transform mTextTransform);

		web_color get_color() const;
		void set_color(web_color color);

		float get_opacity() const;
		void set_opacity(float opacity);

		string get_cursor() const;
		void set_cursor(const string& cursor);

		string get_content() const;
		void set_content(const string& content);

		border_collapse get_border_collapse() const;
		void set_border_collapse(border_collapse mBorderCollapse);

		const css_length& get_border_spacing_x() const ;
		void set_border_spacing_x(const css_length& mBorderSpacingX);

		const css_length& get_border_spacing_y() const;
		void set_border_spacing_y(const css_length& mBorderSpacingY);

		caption_side get_caption_side() const;
		void set_caption_side(caption_side side);

		float get_flex_grow() const;
		float get_flex_shrink() const;
		const css_length& get_flex_basis() const;
		flex_direction get_flex_direction() const;
		flex_wrap get_flex_wrap() const;
		flex_justify_content get_flex_justify_content() const;
		flex_align_items get_flex_align_items() const;
		flex_align_items get_flex_align_self() const;
		flex_align_content get_flex_align_content() const;

		const string& get_animation_name() const;
		time get_animation_duration() const;
		time get_animation_delay() const;
		animation_direction get_animation_direction() const;
		animation_fill_mode get_animation_fill_mode() const;
		animation_play_state get_animation_play_state() const;
		timing_function_type get_animation_timing_function() const;
		float get_animation_iteration_count() const;
		void set_animation_play_state(animation_play_state state);

		const string_vector& get_transition_properties() const;
		const std::vector<time>& get_transition_duration() const;
		const std::vector<time>& get_transition_delay() const;
		const std::vector<timing_function_type>& get_transition_timing_function() const;
		const std::vector<transition_behavior>& get_transition_behavior() const;

		const css_transform& get_transform() const;
		void set_transform(const css_transform& transform);

		const length_vector& get_transform_origin() const;

		transform_style get_transform_style() const;

		const css_length& get_perspective() const;
		const length_vector& get_perspective_origin() const;

		backface_visibility get_backface_visibility() const;

		int get_order() const;
		void set_order(int order);
	};

	inline element_position css_properties::get_position() const
	{
		return m_el_position;
	}

	inline void css_properties::set_position(element_position mElPosition)
	{
		m_el_position = mElPosition;
	}

	inline text_align css_properties::get_text_align() const
	{
		return m_text_align;
	}

	inline void css_properties::set_text_align(text_align mTextAlign)
	{
		m_text_align = mTextAlign;
	}

	inline overflow css_properties::get_overflow() const
	{
		return m_overflow;
	}

	inline void css_properties::set_overflow(overflow mOverflow)
	{
		m_overflow = mOverflow;
	}

	inline white_space css_properties::get_white_space() const
	{
		return m_white_space;
	}

	inline void css_properties::set_white_space(white_space mWhiteSpace)
	{
		m_white_space = mWhiteSpace;
	}

	inline style_display css_properties::get_display() const
	{
		return m_display;
	}

	inline void css_properties::set_display(style_display mDisplay)
	{
		m_display = mDisplay;
	}

	inline visibility css_properties::get_visibility() const
	{
		return m_visibility;
	}

	inline void css_properties::set_visibility(visibility mVisibility)
	{
		m_visibility = mVisibility;
	}

	inline appearance css_properties::get_appearance() const
	{
		return m_appearance;
	}

	inline void css_properties::set_appearance(appearance mAppearance)
	{
		m_appearance = mAppearance;
	}

	inline box_sizing css_properties::get_box_sizing() const
	{
		return m_box_sizing;
	}

	inline void css_properties::set_box_sizing(box_sizing mBoxSizing)
	{
		m_box_sizing = mBoxSizing;
	}

	inline int css_properties::get_z_index() const
	{
		return (int)m_z_index.val();
	}

	inline void css_properties::set_z_index(int mZIndex)
	{
		m_z_index.set_value((float)mZIndex, css_units_none);
	}

	inline vertical_align css_properties::get_vertical_align() const
	{
		return m_vertical_align;
	}

	inline void css_properties::set_vertical_align(vertical_align mVerticalAlign)
	{
		m_vertical_align = mVerticalAlign;
	}

	inline element_float css_properties::get_float() const
	{
		return m_float;
	}

	inline void css_properties::set_float(element_float mFloat)
	{
		m_float = mFloat;
	}

	inline element_clear css_properties::get_clear() const
	{
		return m_clear;
	}

	inline void css_properties::set_clear(element_clear mClear)
	{
		m_clear = mClear;
	}

	inline const css_margins &css_properties::get_margins() const
	{
		return m_css_margins;
	}

	inline void css_properties::set_margins(const css_margins &mCssMargins)
	{
		m_css_margins = mCssMargins;
	}

	inline const css_margins &css_properties::get_padding() const
	{
		return m_css_padding;
	}

	inline void css_properties::set_padding(const css_margins &mCssPadding)
	{
		m_css_padding = mCssPadding;
	}

	inline const css_borders &css_properties::get_borders() const
	{
		return m_css_borders;
	}

	inline void css_properties::set_borders(const css_borders &mCssBorders)
	{
		m_css_borders = mCssBorders;
	}

	inline const css_length &css_properties::get_width() const
	{
		return m_css_width;
	}

	inline void css_properties::set_width(const css_length &mCssWidth)
	{
		m_css_width = mCssWidth;
	}

	inline const css_length &css_properties::get_height() const
	{
		return m_css_height;
	}

	inline void css_properties::set_height(const css_length &mCssHeight)
	{
		m_css_height = mCssHeight;
	}

	inline const css_length &css_properties::get_min_width() const
	{
		return m_css_min_width;
	}

	inline void css_properties::set_min_width(const css_length &mCssMinWidth)
	{
		m_css_min_width = mCssMinWidth;
	}

	inline const css_length &css_properties::get_min_height() const
	{
		return m_css_min_height;
	}

	inline void css_properties::set_min_height(const css_length &mCssMinHeight)
	{
		m_css_min_height = mCssMinHeight;
	}

	inline const css_length &css_properties::get_max_width() const
	{
		return m_css_max_width;
	}

	inline void css_properties::set_max_width(const css_length &mCssMaxWidth)
	{
		m_css_max_width = mCssMaxWidth;
	}

	inline const css_length &css_properties::get_max_height() const
	{
		return m_css_max_height;
	}

	inline void css_properties::set_max_height(const css_length &mCssMaxHeight)
	{
		m_css_max_height = mCssMaxHeight;
	}

	inline const css_offsets &css_properties::get_offsets() const
	{
		return m_css_offsets;
	}

	inline void css_properties::set_offsets(const css_offsets &mCssOffsets)
	{
		m_css_offsets = mCssOffsets;
	}

	inline const css_length &css_properties::get_text_indent() const
	{
		return m_css_text_indent;
	}

	inline void css_properties::set_text_indent(const css_length &mCssTextIndent)
	{
		m_css_text_indent = mCssTextIndent;
	}

	inline int css_properties::get_line_height() const
	{
		return m_line_height;
	}

	inline void css_properties::set_line_height(int mLineHeight)
	{
		m_line_height = mLineHeight;
	}

	inline list_style_type css_properties::get_list_style_type() const
	{
		return m_list_style_type;
	}

	inline void css_properties::set_list_style_type(list_style_type mListStyleType)
	{
		m_list_style_type = mListStyleType;
	}

	inline list_style_position css_properties::get_list_style_position() const
	{
		return m_list_style_position;
	}

	inline void css_properties::set_list_style_position(list_style_position mListStylePosition)
	{
		m_list_style_position = mListStylePosition;
	}

	inline string css_properties::get_list_style_image() const { return m_list_style_image; }
	inline void css_properties::set_list_style_image(const string& url) { m_list_style_image = url; }

	inline string css_properties::get_list_style_image_baseurl() const { return m_list_style_image_baseurl; }
	inline void css_properties::set_list_style_image_baseurl(const string& url) { m_list_style_image_baseurl = url; }

	inline const background &css_properties::get_bg() const
	{
		return m_bg;
	}

	inline void css_properties::set_bg(const background &mBg)
	{
		m_bg = mBg;
	}

	inline int css_properties::get_font_size() const
	{
		return (int)m_font_size.val();
	}

	inline void css_properties::set_font_size(int mFontSize)
	{
		m_font_size = (float)mFontSize;
	}

	inline uint_ptr css_properties::get_font() const
	{
		return m_font;
	}

	inline void css_properties::set_font(uint_ptr mFont)
	{
		m_font = mFont;
	}

	inline const font_metrics& css_properties::get_font_metrics() const
	{
		return m_font_metrics;
	}

	inline void css_properties::set_font_metrics(const font_metrics& mFontMetrics)
	{
		m_font_metrics = mFontMetrics;
	}

	inline text_transform css_properties::get_text_transform() const
	{
		return m_text_transform;
	}

	inline void css_properties::set_text_transform(text_transform mTextTransform)
	{
		m_text_transform = mTextTransform;
	}

	inline web_color css_properties::get_color() const { return m_color; }
	inline void css_properties::set_color(web_color color) { m_color = color; }

	inline float css_properties::get_opacity() const { return m_opacity; }
	inline void css_properties::set_opacity(float opacity) { m_opacity = opacity; }

	inline string css_properties::get_cursor() const { return m_cursor; }
	inline void css_properties::set_cursor(const string& cursor) { m_cursor = cursor; }

	inline string css_properties::get_content() const { return m_content; }
	inline void css_properties::set_content(const string& content) { m_content = content; }

	inline border_collapse css_properties::get_border_collapse() const
	{
		return m_border_collapse;
	}

	inline void css_properties::set_border_collapse(border_collapse mBorderCollapse)
	{
		m_border_collapse = mBorderCollapse;
	}

	inline const css_length& css_properties::get_border_spacing_x() const
	{
		return m_css_border_spacing_x;
	}

	inline void css_properties::set_border_spacing_x(const css_length& mBorderSpacingX)
	{
		m_css_border_spacing_x = mBorderSpacingX;
	}

	inline const css_length& css_properties::get_border_spacing_y() const
	{
		return m_css_border_spacing_y;
	}

	inline void css_properties::set_border_spacing_y(const css_length& mBorderSpacingY)
	{
		m_css_border_spacing_y = mBorderSpacingY;
	}

	inline float css_properties::get_flex_grow() const
	{
		return m_flex_grow;
	}

	inline float css_properties::get_flex_shrink() const
	{
		return m_flex_shrink;
	}

	inline const css_length& css_properties::get_flex_basis() const
	{
		return m_flex_basis;
	}

	inline flex_direction css_properties::get_flex_direction() const
	{
		return m_flex_direction;
	}

	inline flex_wrap css_properties::get_flex_wrap() const
	{
		return m_flex_wrap;
	}

	inline flex_justify_content css_properties::get_flex_justify_content() const
	{
		return m_flex_justify_content;
	}

	inline flex_align_items css_properties::get_flex_align_items() const
	{
		return m_flex_align_items;
	}

	inline flex_align_items css_properties::get_flex_align_self() const
	{
		return m_flex_align_self;
	}

	inline flex_align_content css_properties::get_flex_align_content() const
	{
		return m_flex_align_content;
	}

	inline caption_side css_properties::get_caption_side() const
	{
		return m_caption_side;
	}

	inline void css_properties::set_caption_side(caption_side side)
	{
		m_caption_side = side;
	}

	inline const std::string& css_properties::get_animation_name() const
	{
		return m_animation_name;
	}

	inline time css_properties::get_animation_duration() const
	{
		return m_animation_duration;
	}

	inline time css_properties::get_animation_delay() const
	{
		return m_animation_delay;
	}

	inline animation_direction css_properties::get_animation_direction() const
	{
		return m_animation_direction;
	}

	inline animation_fill_mode css_properties::get_animation_fill_mode() const
	{
		return m_animation_fill_mode;
	}

	inline animation_play_state css_properties::get_animation_play_state() const
	{
		return m_animation_play_state;
	}

	inline timing_function_type css_properties::get_animation_timing_function() const
	{
	    return m_animation_timing_function;
	}

	inline float css_properties::get_animation_iteration_count() const
	{
		return m_animation_iteration_count;
	}

	inline void css_properties::set_animation_play_state(const animation_play_state state)
	{
		m_animation_play_state = state;
	}

	inline const string_vector& css_properties::get_transition_properties() const
	{
		return m_transition_properties;
	}

	inline const std::vector<time>& css_properties::get_transition_duration() const
	{
		return m_transition_duration;
	}

	inline const std::vector<time>& css_properties::get_transition_delay() const
	{
		return m_transition_delay;
	}

	inline const std::vector<timing_function_type>& css_properties::get_transition_timing_function() const
	{
		return m_transition_timing_function;
	}

	inline const std::vector<transition_behavior>& css_properties::get_transition_behavior() const
	{
		return m_transition_behavior;
	}

	inline const css_transform& css_properties::get_transform() const
	{
		return m_transform;
	}

	inline void css_properties::set_transform(const css_transform& transform)
	{
		m_transform = transform;
	}

	inline const length_vector& css_properties::get_transform_origin() const
	{
		return m_transform_origin;
	}

	inline transform_style css_properties::get_transform_style() const
	{
		return m_transform_style;
	}

	inline const css_length & css_properties::get_perspective() const
	{
		return m_perspective;
	}

	inline const length_vector & css_properties::get_perspective_origin() const
	{
		return m_perspective_origin;
	}

	inline backface_visibility css_properties::get_backface_visibility() const
	{
		return m_backface_visibility;
	}

	inline int css_properties::get_order() const
	{
		return m_order;
	}

	inline void css_properties::set_order(int order)
	{
		m_order = order;
	}
}

#endif //LITEHTML_CSS_PROPERTIES_H
