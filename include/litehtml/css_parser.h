#pragma once
#include "css_tokenizer.h"
#include "stylesheet.h"

namespace litehtml
{
//struct url {};

class css_parser
{
	css_token_vector m_tokens;
	int m_index = 0;

	css_token next_token();
	css_token peek_token();

public:
	// Stage 2
	static raw_rule::vector parse_stylesheet(const string& input,           bool top_level);
	static raw_rule::vector parse_stylesheet(const css_token_vector& input, bool top_level);
	raw_rule::vector		consume_list_of_rules(bool top_level);
	raw_rule::ptr			consume_qualified_rule();
	raw_rule::ptr			consume_at_rule();
	css_token				consume_simple_block(char opening_bracket);
	css_token				consume_component_value();
	css_token				consume_function(const string& name);

	// Stage 3
	raw_declaration			consume_declaration();
public:
	void					consume_style_block_contents(/*out*/ raw_declaration::vector& decls, /*out*/ raw_rule::vector& rules);

	css_parser() {}
	css_parser(const css_token_vector& tokens) : m_tokens(tokens) {}

};

using keep_whitespace_fn = std::function<bool (const css_token& left, const css_token& right)>;
void remove_whitespace(css_token_vector& tokens, keep_whitespace_fn keep_whitespace = 0);

enum {
	f_componentize = 1,
	f_remove_whitespace = 2
};
template<class Input>
css_token_vector normalize(Input input, int options = 0, keep_whitespace_fn keep_whitespace = 0);

vector<css_token_vector> parse_comma_separated_list(const css_token_vector& compvals);
bool is_declaration_value(const css_token_vector& tokens, int index = 0);
bool is_any_value(const css_token_vector& tokens);

} // namespace litehtml