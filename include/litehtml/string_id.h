#ifndef LH_STRING_ID_H
#define LH_STRING_ID_H

namespace litehtml
{

#define STRING_ID(...)\
	enum string_id { __VA_ARGS__ };\
	const auto initial_string_ids = #__VA_ARGS__;

STRING_ID(

	// CSS property names
	_background_,
	_background_color_,
	_background_image_,
	_background_image_baseurl_,
	_background_repeat_,
	_background_origin_,
	_background_clip_,
	_background_attachment_,
	_background_position_,
	_background_size_,

	_border_,
	_border_width_,
	_border_style_,
	_border_color_,

	_border_spacing_,
	__litehtml_border_spacing_x_,
	__litehtml_border_spacing_y_,

	_border_left_,
	_border_right_,
	_border_top_,
	_border_bottom_,

	_border_left_style_,
	_border_right_style_,
	_border_top_style_,
	_border_bottom_style_,

	_border_left_width_,
	_border_right_width_,
	_border_top_width_,
	_border_bottom_width_,

	_border_left_color_,
	_border_right_color_,
	_border_top_color_,
	_border_bottom_color_,

	_border_radius_,
	_border_radius_x_,
	_border_radius_y_,

	_border_bottom_left_radius_,
	_border_bottom_left_radius_x_,
	_border_bottom_left_radius_y_,

	_border_bottom_right_radius_,
	_border_bottom_right_radius_x_,
	_border_bottom_right_radius_y_,

	_border_top_left_radius_,
	_border_top_left_radius_x_,
	_border_top_left_radius_y_,

	_border_top_right_radius_,
	_border_top_right_radius_x_,
	_border_top_right_radius_y_,

	_list_style_,
	_list_style_type_,
	_list_style_position_,
	_list_style_image_,
	_list_style_image_baseurl_,

	_margin_,
	_margin_left_,
	_margin_right_,
	_margin_top_,
	_margin_bottom_,
	_padding_,
	_padding_left_,
	_padding_right_,
	_padding_top_,
	_padding_bottom_,

	_font_,
	_font_family_,
	_font_style_,
	_font_variant_,
	_font_weight_,
	_font_size_,
	_line_height_,
	_text_decoration_,

	_white_space_,
	_text_align_,
	_vertical_align_,
	_color_,
	_width_,
	_height_,
	_min_width_,
	_min_height_,
	_max_width_,
	_max_height_,
	_position_,
	_overflow_,
	_display_,
	_visibility_,
	_box_sizing_,
	_z_index_,
	_float_,
	_clear_,
	_text_indent_,
	_left_,
	_right_,
	_top_,
	_bottom_,
	_cursor_,
	_content_,
	_border_collapse_,
	_text_transform_,

	_flex_,
	_flex_flow_,
	_flex_direction_,
	_flex_wrap_,
	_justify_content_,
	_align_items_,
	_align_content_,
	_align_self_,
	_flex_grow_,
	_flex_shrink_,
	_flex_basis_,
)
#undef STRING_ID

string_id _id(const string& str);
string _s(string_id id);

} // namespace litehtml

#endif  // LH_STRING_ID_H
