#include "html.h"
#include "types.h"
#include "grid.h"
#include "css_parser.h"
#include "style.h"

bool litehtml::css_grid_line::from_tokens(const css_token_vector& tokens)
{
	if(tokens.size() == 1 && tokens[0].type == IDENT && tokens[0].name == "auto")
	{
		line = auto_def();
		return true;
	}

	bool span = false;
	int integer = 0;
	std::string custom_ident;
	for(const auto& token : tokens)
	{
		if(token.type == IDENT)
		{
			if(token.name == "span")
			{
				span = true;
			}
			if(custom_ident.empty())
			{
				custom_ident = token.name;
			} else
				return false;
		} else if(token.type == NUMBER && token.n.number_type == css_number_integer)
		{
			// Only one number is allowed
			// Number without units required. 0 is invalid
			if(integer != 0 || token.n.number_type != css_number_integer || token.n.number == 0) return false;
			integer = (int) token.n.number;
		} else
			return false;
	}

	if(span)
	{
		// Number >= 1 or custom ident are required
		if(integer < 1 && custom_ident.empty()) return false;
		line = span_def(_id(custom_ident), integer);
		return true;
	}
	if(integer != 0)
	{
		line = integer_custom_ident_def(_id(custom_ident), integer);
		return true;
	}
	if(!custom_ident.empty())
	{
		line = custom_ident_def(_id(custom_ident));
		return true;
	}

	return false;
}

bool litehtml::css_grid_template_areas::from_tokens(const css_token_vector& tokens)
{
	if(tokens.size() == 1 && tokens[0].type == IDENT && tokens[0].name == "none")
	{
		return true;
	}

	for(auto& line_tok : tokens)
	{
		if(!from_token(line_tok))
		{
			return false;
		}
	}
	return true;
}

bool litehtml::css_grid_template_areas::from_token(const litehtml::css_token &token)
{
	if(token.type == STRING)
	{
		trim(token.str, "\"'");
		string_vector cols;
		split_string(token.str, cols, split_delims_spaces, "", "", split_delims_spaces);
		if (cols.empty()) return false;
		areas.emplace_back(cols);
		return true;
	}
	return false;
}

// grid-template-columns =
//  none                       |
//  <track-list>               |
//  <auto-track-list>          |
//  subgrid <line-name-list>?
// https://developer.mozilla.org/en-US/docs/Web/CSS/grid-template-columns#formal_syntax
bool litehtml::css_grid_template::from_tokens(const css_token_vector& tokens)
{
	if(tokens.size() == 1 && tokens[0].type == IDENT && tokens[0].name == "none")
	{
		value = none();
		return true;
	}

	track_list tl;
	if(tl.parse(tokens))
	{
		value = tl;
		return true;
	}

	auto_track_list atl;
	if(atl.parse(tokens))
	{
		value = atl;
		return true;
	}

	subgrid sg;
	if(sg.parse(tokens))
	{
		value = sg;
		return true;
	}
	return false;
}

// <line-names> =
//  '[' <custom-ident>* ']'
// https://developer.mozilla.org/en-US/docs/Web/CSS/grid-template-columns#formal_syntax
bool litehtml::css_grid_template::line_names::parse(const litehtml::css_token &token)
{
	if(token.type == SQUARE_BLOCK)
	{
		for(const auto &item : token.value)
		{
			if (item.type == IDENT)
			{
				names.emplace_back(item.name);
			} else return false;
		}
		return true;
	}
	return false;
}

// <name-repeat> =
//  repeat( [ <integer [1,∞]> | auto-fill ] , <line-names>+ )
//https://developer.mozilla.org/en-US/docs/Web/CSS/grid-template-columns#formal_syntax
bool litehtml::css_grid_template::name_repeat::parse(const litehtml::css_token &token)
{
	if(token.type == CV_FUNCTION && token.name == "repeat")
	{
		auto args = parse_comma_separated_list(token.value);
		if(args.size() != 2) return false;

		if(args[0].size() != 1) return false;
		if(args[0][0].type == NUMBER && args[0][0].n.number_type == css_number_integer && args[0][0].n.number >= 1)
		{
			arg1 = (int) args[0][0].n.number;
		} else if(args[0][0].type == IDENT && args[0][0].name == "auto-fill")
		{
			arg1 = auto_fill();
		} else return false;

		for(const auto &item : args[1])
		{
			line_names val;
			if(val.parse(item))
			{
				arg2.emplace_back(val);
			} else
				return false;
		}
		return true;
	}
	return false;
}

// subgrid
// <line-name-list> =
//  [ <line-names> | <name-repeat> ]+
// https://developer.mozilla.org/en-US/docs/Web/CSS/grid-template-columns#formal_syntax
bool litehtml::css_grid_template::subgrid::parse(const litehtml::css_token_vector &tokens)
{
	if(tokens.size() <= 1) return false;

	if(tokens[0].type == IDENT && tokens[0].name == "subgrid")
	{
		for(const auto &item : tokens)
		{
			line_names ln;
			name_repeat rpt;
			if(ln.parse(item))
			{
				line_name_list.emplace_back(ln);
			} else if(rpt.parse(item))
			{
				line_name_list.emplace_back(rpt);
			} else return false;
		}
		return true;
	}

	return false;
}

// <track-breadth> =
//  <length-percentage [0,∞]>  |
//  <flex [0,∞]>               |
//  min-content                |
//  max-content                |
//  auto
// https://developer.mozilla.org/en-US/docs/Web/CSS/grid-template-columns#formal_syntax
bool litehtml::css_grid_template::track_breadth::parse(css_token_vector::const_iterator& iter, const css_token_vector::const_iterator& end)
{
	auto iter_ = iter;

	if(iter_->type == IDENT && iter_->name == "subgrid")
	{
		iter_ = std::next(iter_);
		if(iter_ == end) return false;
		if(iter_->type == NUMBER && iter_->n.number_type == css_number_integer && iter_->n.number >= 0)
		{
			flex f;
			f.n = (int) iter_->n.number;
			value = f;
			iter = iter_;
			return true;
		}
	}

	css_length len;
	if(len.from_token((*iter_), f_length_percentage, minmax_predef_str))
	{
		value = len;
		return true;
	}

	return false;
}

// <track-list> =
//  [ <line-names>? [ <track-size> | <track-repeat> ] ]+ <line-names>?
// https://developer.mozilla.org/en-US/docs/Web/CSS/grid-template-columns#formal_syntax
bool litehtml::css_grid_template::track_list::parse(const litehtml::css_token_vector &tokens)
{
	for(auto iter = tokens.cbegin(); iter != tokens.cend(); iter++)
	{
		line_names ln;
		track_size ts;
		track_repeat tr;
		if(ln.parse(*iter))
		{
			value.emplace_back(ln);
			continue;
		}
		if(ts.parse(iter, tokens.cend()))
		{
			value.emplace_back(ts);
			continue;
		}
		if(tr.parse(*iter))
		{
			value.emplace_back(ts);
			continue;
		}
		return false;
	}
	return !value.empty();
}

// <track-size> =
//  <track-breadth>                                   |
//  minmax( <inflexible-breadth> , <track-breadth> )  |
//  fit-content( <length-percentage [0,∞]> )
// https://developer.mozilla.org/en-US/docs/Web/CSS/grid-template-columns#formal_syntax
bool litehtml::css_grid_template::track_size::parse(css_token_vector::const_iterator &iter, const css_token_vector::const_iterator& end)
{
	minmax mm;
	if(mm.parse(*iter))
	{
		value = mm;
		return true;
	}
	fit_content fc;
	if(fc.parse(*iter))
	{
		value = fc;
		return true;
	}
	track_breadth tb;
	if(tb.parse(iter, end))
	{
		value = tb;
		return true;
	}

	return false;
}

bool litehtml::css_grid_template::minmax::parse(const css_token& token)
{
	if(token.type == CV_FUNCTION && token.name == "minmax")
	{
		auto args = parse_comma_separated_list(token.value);
		if(args.size() != 2) return false;

		if(args[0].size() == 1)
		{
			fixed_breadth fb;
			inflexible_breadth ifb;
			if(fb.parse(args[0][0]))
			{
				arg1 = fb;
			} else if(ifb.parse(args[0][0]))
			{
				arg1 = ifb;
			} else return false;
		} else return false;

		if(!args[1].empty())
		{
			track_breadth tb;
			fixed_breadth fb;
			auto iter = args[1].cbegin();
			if(tb.parse(iter, args[1].cend()))
			{
				arg2 = tb;
			} else if(fb.parse(args[0][0]))
			{
				arg2 = fb;
			} else return false;
		} else return false;
		return true;
	}
	return false;
}

// <fixed-size> =
//  <fixed-breadth>                                   |
//  minmax( <fixed-breadth> , <track-breadth> )       |
//  minmax( <inflexible-breadth> , <fixed-breadth> )
// https://developer.mozilla.org/en-US/docs/Web/CSS/grid-template-columns#formal_syntax
bool litehtml::css_grid_template::fixed_size::parse(const litehtml::css_token &token)
{
	fixed_breadth fb;
	if(fb.parse(token))
	{
		value = fb;
		return true;
	}

	minmax mm;
	if(mm.parse(token))
	{
		value = mm;
		return true;
	}

	return false;
}

// <fixed-repeat> =
//  repeat( [ <integer [1,∞]> ] , [ <line-names>? <fixed-size> ]+ <line-names>? )
// https://developer.mozilla.org/en-US/docs/Web/CSS/grid-template-columns#formal_syntax
bool litehtml::css_grid_template::fixed_repeat::parse(const litehtml::css_token &token)
{
	if(token.type == CV_FUNCTION && token.name == "repeat")
	{
		auto args = parse_comma_separated_list(token.value);
		if(args.size() != 2) return false;

		if(args[0].size() == 1 && args[0][0].type == NUMBER and args[0][0].n.number_type == css_number_integer && args[0][0].n.number >= 1)
		{
			arg1 = (int) args[0][0].n.number;
		} else return false;

		for(const auto &item : args[1])
		{
			line_names ln;
			fixed_size fs;
			if(ln.parse(item))
			{
				arg2.emplace_back(ln);
			} else if(fs.parse(item))
			{
				arg2.emplace_back(fs);
			} else
				return false;
		}
		return !arg2.empty();
	}

	return false;
}

// <track-repeat> =
//  repeat( [ <integer [1,∞]> ] , [ <line-names>? <track-size> ]+ <line-names>? )
// https://developer.mozilla.org/en-US/docs/Web/CSS/grid-template-columns#formal_syntax
bool litehtml::css_grid_template::track_repeat::parse(const litehtml::css_token &token)
{
	if(token.type == CV_FUNCTION && token.name == "repeat")
	{
		auto args = parse_comma_separated_list(token.value);
		if(args.size() != 2) return false;

		if(args[0].size() == 1 && args[0][0].type == NUMBER and args[0][0].n.number_type == css_number_integer && args[0][0].n.number >= 1)
		{
			arg1 = (int) args[0][0].n.number;
		} else return false;

		for(auto iter = args[1].cbegin(); iter != args[1].cend(); iter++)
		{
			line_names ln;
			track_size fs;
			if(ln.parse(*iter))
			{
				arg2.emplace_back(ln);
			} else if(fs.parse(iter, args[1].cend()))
			{
				arg2.emplace_back(fs);
			} else
				return false;
		}
		return !arg2.empty();
	}

	return false;
}

// <auto-repeat> =
//  repeat( [ auto-fill | auto-fit ] , [ <line-names>? <fixed-size> ]+ <line-names>? )
// https://developer.mozilla.org/en-US/docs/Web/CSS/grid-template-columns#formal_syntax
bool litehtml::css_grid_template::auto_repeat::parse(const litehtml::css_token &token)
{
	if(token.type == CV_FUNCTION && token.name == "repeat")
	{
		auto args = parse_comma_separated_list(token.value);
		if(args.size() != 2) return false;

		if(args[0].size() == 1 && args[0][0].type == IDENT and is_one_of(args[0][0].name, "auto-fill", "auto-fit"))
		{
			if(args[0][0].name == "auto-fill")
			{
				arg1 = auto_fill;
			} else
			{
				arg1 = auto_fit;
			}
		} else return false;

		for(const auto &item : args[1])
		{
			line_names ln;
			fixed_size fs;
			if(ln.parse(item))
			{
				arg2.emplace_back(ln);
			} else if(fs.parse(item))
			{
				arg2.emplace_back(fs);
			} else
				return false;
		}
		return !arg2.empty();
	}

	return false;
}

// <auto-track-list> =
//  [ <line-names>? [ <fixed-size> | <fixed-repeat> ] ]* <line-names>? <auto-repeat> [ <line-names>? [ <fixed-size> | <fixed-repeat> ] ]* <line-names>?
// https://developer.mozilla.org/en-US/docs/Web/CSS/grid-template-columns#formal_syntax
bool litehtml::css_grid_template::auto_track_list::parse(const litehtml::css_token_vector &tokens)
{
	int auto_repeat_found = false;
	for(const auto &token : tokens)
	{
		line_names ln;
		fixed_size fs;
		auto_repeat ar;
		fixed_repeat fr;

		if(ln.parse(token))
		{
			value.emplace_back(ln);
			continue;
		}
		if(fs.parse(token))
		{
			value.emplace_back(fs);
			continue;
		}
		if(ar.parse(token))
		{
			if(auto_repeat_found) return false;
			value.emplace_back(ar);
			auto_repeat_found = true;
			continue;
		}
		if(fr.parse(token))
		{
			value.emplace_back(fr);
			continue;
		}
		return false;
	}
	return auto_repeat_found;
}

bool litehtml::css_grid_template::fit_content::parse(const litehtml::css_token &token)
{
	if(token.type == CV_FUNCTION && token.name == "repeat")
	{
		if(token.value.size() == 1)
		{
			return value.parse(token.value[0]);
		}
	}
	return false;
}
