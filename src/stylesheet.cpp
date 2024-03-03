#include "html.h"
#include "stylesheet.h"
#include <algorithm>
#include "document.h"

#ifndef M_PI
#       define M_PI    3.14159265358979323846
#endif

void litehtml::css::parse_stylesheet(const char* str, const char* baseurl, const std::shared_ptr<document>& doc, const media_query_list::ptr& media)
{
	string text = str;

	// remove comments
	string::size_type c_start = text.find("/*");
	while(c_start != string::npos)
	{
		string::size_type c_end = text.find("*/", c_start + 2);
		if(c_end == string::npos)
		{
			text.erase(c_start);
			break;
		}
		text.erase(c_start, c_end - c_start + 2);
		c_start = text.find("/*");
	}

	string::size_type pos = text.find_first_not_of(" \n\r\t");
	while(pos != string::npos)
	{
		while(pos != string::npos && text[pos] == '@')
		{
			string::size_type sPos = pos;
			pos = text.find_first_of("{;", pos);
			if(pos != string::npos && text[pos] == '{')
			{
				pos = find_close_bracket(text, pos, '{', '}');
			}
			if(pos != string::npos)
			{
				parse_atrule(text.substr(sPos, pos - sPos + 1), baseurl, doc, media);
			} else
			{
				parse_atrule(text.substr(sPos), baseurl, doc, media);
			}

			if(pos != string::npos)
			{
				pos = text.find_first_not_of(" \n\r\t", pos + 1);
			}
		}

		if(pos == string::npos)
		{
			break;
		}

		string::size_type style_start	= text.find('{', pos);
		string::size_type style_end	= text.find('}', pos);
		if(style_start != string::npos && style_end != string::npos)
		{
			auto str_style = text.substr(style_start + 1, style_end - style_start - 1);
			style::ptr style = std::make_shared<litehtml::style>();
			style->add(str_style, baseurl ? baseurl : "", doc->container());

			parse_selectors(text.substr(pos, style_start - pos), style, media);

			if(media && doc)
			{
				doc->add_media_list(media);
			}

			pos = style_end + 1;
		} else
		{
			pos = string::npos;
		}

		if(pos != string::npos)
		{
			pos = text.find_first_not_of(" \n\r\t", pos);
		}
	}
}

bool litehtml::css::parse_css_angle(const string& str, double& angle)
{
	const char* start = str.c_str();
	for(;start[0]; start++)
	{
		if(!isspace(start[0])) break;
	}
	if(start[0] == 0) return false;
	char* end = nullptr;
	double a = strtod(start, &end);
	if(end && end[0] != 0) return false;
	if(!strcmp(end, "deg"))
	{
		a = a * M_PI / 180.0;
	} else if(!strcmp(end, "grad"))
	{
		a = a * M_PI / 200.0;
	} else if(!strcmp(end, "turn"))
	{
		a = a * 2.0 * M_PI;
	} else if(!strcmp(end, "rad"))
	{
		return false;
	}
	angle = a;
	return true;
}

void litehtml::css::parse_gradient(const string &token, document_container *container, background_gradient& grad)
{
	size_t pos1 = token.find('(');
	size_t pos2 = token.find_last_of(')');
	std::string grad_str;
	if(pos1 != std::string::npos)
	{
		auto gradient_type_str = token.substr(0, pos1);
		trim(gradient_type_str);
		background_gradient::gradient_type gradient_type = (background_gradient::gradient_type) (value_index(
				gradient_type_str,
				"linear-gradient;repeating-linear-gradient;radial-gradient;repeating-radial-gradient", -2) + 1);

		if(pos2 != std::string::npos)
		{
			grad_str = token.substr(pos1 + 1, pos2 - pos1 - 1);
		} else
		{
			grad_str = token.substr(pos1);
		}
		string_vector items;
		split_string(grad_str, items, ",", "", "()");
		int num_colors = 0;

		auto parse_color_stop_list = [&](string_vector& parts)
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
						grad.m_colors.push_back(gc);
						num_colors++;
					}
					if(parts.size() > 2)
					{
						length.fromString(parts[2]);
						if(!length.is_predefined())
						{
							background_gradient::gradient_color gc;
							gc.color = color;
							gc.length = length;
							grad.m_colors.push_back(gc);
							num_colors++;
						}
					}
				} else
				{
					background_gradient::gradient_color gc;
					gc.color = color;
					grad.m_colors.push_back(gc);
					num_colors++;
				}
			};

		if(gradient_type == background_gradient::linear_gradient || gradient_type == background_gradient::repeating_linear_gradient)
		{
			for (const auto &item: items)
			{
				string_vector parts;
				split_string(item, parts, " \t", "", "()");
				if (!parts.empty())
				{
					if (parts[0] == "to")
					{
						for (size_t part_idx = 1; part_idx < parts.size(); part_idx++)
						{
							int side = value_index(parts[1], "left;right;top;bottom");
							if (side >= 0)
							{
								grad.m_side |= (background_gradient::gradient_side) (1 << side);
							}
						}
					} else if (parts.size() == 1 && css::parse_css_angle(parts[0], grad.angle))
					{
						continue;
					} else if (web_color::is_color(parts[0], container))
					{
						parse_color_stop_list(parts);
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
		} else if(gradient_type == background_gradient::radial_gradient || gradient_type == background_gradient::repeating_linear_gradient)
		{

		}
		if(num_colors >= 2)
		{
			grad.m_type = gradient_type;
		}
	}
}

void litehtml::css::parse_css_url( const string& str, string& url )
{
	url = "";
	size_t pos1 = str.find('(');
	size_t pos2 = str.find(')');
	if(pos1 != string::npos && pos2 != string::npos)
	{
		url = str.substr(pos1 + 1, pos2 - pos1 - 1);
		if(url.length())
		{
			if(url[0] == '\'' || url[0] == '"')
			{
				url.erase(0, 1);
			}
		}
		if(url.length())
		{
			if(url[url.length() - 1] == '\'' || url[url.length() - 1] == '"')
			{
				url.erase(url.length() - 1, 1);
			}
		}
	}
}

bool litehtml::css::parse_selectors( const string& txt, const style::ptr& styles, const media_query_list::ptr& media )
{
	string selector = txt;
	trim(selector);
	string_vector tokens;
	split_string(selector, tokens, ",");

	bool added_something = false;

	for(auto & token : tokens)
	{
		css_selector::ptr new_selector = std::make_shared<css_selector>(media);
        new_selector->m_style = styles;
		trim(token);
		if(new_selector->parse(token))
		{
			new_selector->calc_specificity();
			add_selector(new_selector);
			added_something = true;
		}
	}

	return added_something;
}

void litehtml::css::sort_selectors()
{
	std::sort(m_selectors.begin(), m_selectors.end(),
		 [](const css_selector::ptr& v1, const css_selector::ptr& v2)
		 {
			 return (*v1) < (*v2);
		 }
	);
}

void litehtml::css::parse_atrule(const string& text, const char* baseurl, const std::shared_ptr<document>& doc, const media_query_list::ptr& media)
{
	if(text.substr(0, 7) == "@import")
	{
		int sPos = 7;
		string iStr;
		iStr = text.substr(sPos);
		if(iStr[iStr.length() - 1] == ';')
		{
			iStr.erase(iStr.length() - 1);
		}
		trim(iStr);
		string_vector tokens;
		split_string(iStr, tokens, " ", "", "(\"");
		if(!tokens.empty())
		{
			string url;
			parse_css_url(tokens.front(), url);
			if(url.empty())
			{
				url = tokens.front();
			}
			tokens.erase(tokens.begin());
			if(doc)
			{
				document_container* doc_cont = doc->container();
				if(doc_cont)
				{
					string css_text;
					string css_baseurl;
					if(baseurl)
					{
						css_baseurl = baseurl;
					}
					doc_cont->import_css(css_text, url, css_baseurl);
					if(!css_text.empty())
					{
						media_query_list::ptr new_media = media;
						if(!tokens.empty())
						{
							string media_str;
							for(auto iter = tokens.begin(); iter != tokens.end(); iter++)
							{
								if(iter != tokens.begin())
								{
									media_str += " ";
								}
								media_str += (*iter);
							}
							new_media = media_query_list::create_from_string(media_str, doc);
							if(!new_media)
							{
								new_media = media;
							}
						}
						parse_stylesheet(css_text.c_str(), css_baseurl.c_str(), doc, new_media);
					}
				}
			}
		}
	} else if(text.substr(0, 6) == "@media")
	{
		string::size_type b1 = text.find_first_of('{');
		string::size_type b2 = text.find_last_of('}');
		if(b1 != string::npos)
		{
			string media_type = text.substr(6, b1 - 6);
			trim(media_type);
			media_query_list::ptr new_media = media_query_list::create_from_string(media_type, doc);

			string media_style;
			if(b2 != string::npos)
			{
				media_style = text.substr(b1 + 1, b2 - b1 - 1);
			} else
			{
				media_style = text.substr(b1 + 1);
			}

			parse_stylesheet(media_style.c_str(), baseurl, doc, new_media);
		}
	}
}
