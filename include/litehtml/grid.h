#ifndef LITEHTML_GRID_H
#define LITEHTML_GRID_H

#include <optional>
#include <vector>
#include "string_id.h"

namespace litehtml
{
	// <grid-line> =
	//   auto                                                |
	//   <custom-ident>                                      |
	//   [ [ <integer [-∞,-1]> | <integer [1,∞]> ] && <custom-ident>? ]  |
	//   [ span && [ <integer [1,∞]> || <custom-ident> ] ]
	struct css_grid_line
	{
		typedef std::vector<css_grid_line> vector;

		struct span_def
		{
			string_id custom_ident;
			int value;

			span_def() : custom_ident(empty_id),value(1) {}
			span_def(string_id _custom_ident, int _val) : custom_ident(_custom_ident), value(_val) {}
		};

		struct integer_custom_ident_def
		{
			string_id custom_ident;
			int value;
			integer_custom_ident_def() : custom_ident(empty_id),value(0) {}
			integer_custom_ident_def(string_id _custom_ident, int _val) : custom_ident(_custom_ident), value(_val) {}
		};

		struct custom_ident_def
		{
			string_id custom_ident;

			custom_ident_def() : custom_ident(empty_id) {}
			explicit custom_ident_def(string_id _custom_ident) : custom_ident(_custom_ident) {}
		};

		struct auto_def { };

		std::variant<auto_def, span_def, integer_custom_ident_def, custom_ident_def> line;

		css_grid_line() : line(auto_def()) {}

		bool from_tokens(const css_token_vector& tokens);
		template<class T> bool is() const { return std::holds_alternative<T>(line); }
		template<class T> const T& get() const { return std::get<T>(*this); }
	};

	struct css_grid_template_areas
	{
		std::vector<string_vector> areas;
		css_grid_template_areas() : areas() {}

		bool is_none() const { return areas.empty(); }
		bool from_tokens(const css_token_vector& tokens);
		bool from_token(const css_token& token);
		void clear()
		{
			areas.clear();
		}
	};

	// grid-template-columns =
	//  none                       |
	//  <track-list>               |
	//  <auto-track-list>          |
	//  subgrid <line-name-list>?
	struct css_grid_template
	{
		struct none {};

		struct line_names;
		struct track_size;
		struct track_repeat;
		struct fixed_size;
		struct auto_repeat;
		struct fixed_repeat;
		struct name_repeat;

		// <track-list> =
		//  [ <line-names>? [ <track-size> | <track-repeat> ] ]+ <line-names>?
		struct track_list
		{
			std::vector<std::variant<line_names, track_size, track_repeat>> value;

			bool parse(const css_token_vector& tokens);
		};

		// <auto-track-list> =
		//  [ <line-names>? [ <fixed-size> | <fixed-repeat> ] ]* <line-names>? <auto-repeat> [ <line-names>? [ <fixed-size> | <fixed-repeat> ] ]* <line-names>?
		struct auto_track_list
		{
			std::vector<std::variant<line_names, fixed_size, auto_repeat, fixed_repeat>> value;

			bool parse(const css_token_vector& tokens);
		};

		// <line-names> =
		//  '[' <custom-ident>* ']'
		struct line_names
		{
			std::vector<std::string> names;

			bool parse(const css_token& token);
		};

		// <name-repeat> =
		//  repeat( [ <integer [1,∞]> | auto-fill ] , <line-names>+ )
		struct name_repeat
		{
			struct auto_fill {};

			std::variant<int, auto_fill> arg1;
			std::vector<line_names> arg2;

			bool parse(const css_token& token);
		};

		// subgrid
		// <line-name-list> =
		//  [ <line-names> | <name-repeat> ]+
		struct subgrid
		{
			std::vector<std::variant<line_names, name_repeat>> line_name_list;

			bool parse(const css_token_vector& tokens);
		};

		// <track-breadth> =
		//  <length-percentage [0,∞]>  |
		//  <flex [0,∞]>               |
		//  min-content                |
		//  max-content                |
		//  auto
		struct track_breadth
		{
			struct flex
			{
				int n;
				flex() : n(0) {}
			};

			std::variant<css_length, flex> value;

			bool parse(css_token_vector::const_iterator& iter, const css_token_vector::const_iterator& end);
		};

		// <inflexible-breadth> =
		//  <length-percentage [0,∞]>  |
		//  min-content                |
		//  max-content                |
		//  auto
		struct inflexible_breadth
		{
			css_length value;

			bool parse(const css_token& token)
			{
				return value.from_token(token, f_length_percentage, minmax_predef_str);
			}
		};

		// <length-percentage> =
		//  <length>      |
		//  <percentage>
		//
		// <fixed-breadth> =
		//  <length-percentage [0,∞]>
		struct fixed_breadth
		{
			css_length value;

			bool parse(const css_token& token)
			{
				return value.from_token(token, f_length_percentage);
			}
		};

		struct minmax
		{
			std::variant<fixed_breadth, inflexible_breadth> arg1;
			std::variant<track_breadth, fixed_breadth> arg2;

			bool parse(const css_token& token);
		};

		struct fit_content
		{
			fixed_breadth value;

			bool parse(const css_token& token);
		};

		// <track-size> =
		//  <track-breadth>                                   |
		//  minmax( <inflexible-breadth> , <track-breadth> )  |
		//  fit-content( <length-percentage [0,∞]> )
		struct track_size
		{
			std::variant<track_breadth, minmax, fit_content> value;

			bool parse(css_token_vector::const_iterator& iter, const css_token_vector::const_iterator& end);
		};

		// <track-repeat> =
		//  repeat( [ <integer [1,∞]> ] , [ <line-names>? <track-size> ]+ <line-names>? )
		struct track_repeat
		{
			int arg1;
			std::vector<std::variant<line_names, track_size>> arg2;

			bool parse(const css_token& token);
		};

		// <fixed-size> =
		//  <fixed-breadth>                                   |
		//  minmax( <fixed-breadth> , <track-breadth> )       |
		//  minmax( <inflexible-breadth> , <fixed-breadth> )
		struct fixed_size
		{
			std::variant<fixed_breadth, minmax> value;

			bool parse(const css_token& token);
		};

		// <fixed-repeat> =
		//  repeat( [ <integer [1,∞]> ] , [ <line-names>? <fixed-size> ]+ <line-names>? )
		struct fixed_repeat
		{
			int arg1;
			std::vector<std::variant<line_names, fixed_size>> arg2;

			fixed_repeat() : arg1(0) {}
			bool parse(const css_token& token);
		};

		enum auto_repeat_type
		{
			auto_fill,
			auto_fit,
		};
		// <auto-repeat> =
		//  repeat( [ auto-fill | auto-fit ] , [ <line-names>? <fixed-size> ]+ <line-names>? )
		struct auto_repeat
		{
			auto_repeat_type arg1;
			std::vector<std::variant<line_names, fixed_size>> arg2;

			bool parse(const css_token& token);
		};

		static constexpr const char* minmax_predef_str = "auto;min-content;max-content";
		enum minmax_predef
		{
			minmax_auto,
			minmax_min_content,
			minmax_max_content,
		};

		std::variant<none, track_list, auto_track_list, subgrid> value;

		bool from_tokens(const css_token_vector& tokens);
	};
}

#endif //LITEHTML_GRID_H
