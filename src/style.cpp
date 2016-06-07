#include "html.h"
#include "style.h"
#include "string_hash.h"
#include <functional>
#include <algorithm>
#ifndef WINCE
#include <locale>
#endif

litehtml::style::style()
{
}

litehtml::style::style( const style& val )
{
	m_properties = val.m_properties;
}

litehtml::style::~style()
{

}

void litehtml::style::parse( const tchar_t* txt, const tchar_t* baseurl )
{
	std::vector<tstring> properties;
	split_string(txt, properties, _t(";"));

	for(std::vector<tstring>::const_iterator i = properties.begin(); i != properties.end(); i++)
	{
		parse_property(*i, baseurl);
	}
}

void litehtml::style::parse_property( const tstring& txt, const tchar_t* baseurl )
{
	tstring::size_type pos = txt.find_first_of(_t(":"));
	if(pos != tstring::npos)
	{
		tstring name	= txt.substr(0, pos);
		tstring val	= txt.substr(pos + 1);

		trim(name);
		trim(val);

		if(!name.empty() && !val.empty())
		{
			string_vector vals;
			split_string(val, vals, _t("!"));
			if(vals.size() == 1)
			{
				add_property( string_hash::normalizeFromString( name.c_str() ), val.c_str(), baseurl, false);
			} else if(vals.size() > 1)
			{
				trim(vals[0]);
				add_property( string_hash::normalizeFromString( name.c_str() ), vals[0].c_str(), baseurl, t_strcasecmp( vals[1].c_str(), _t("important") ) == 0 );

			}
		}
	}
}

void litehtml::style::combine( const litehtml::style& src )
{
	for(props_map::const_iterator i = src.m_properties.begin(); i != src.m_properties.end(); i++)
	{
		add_parsed_property(i->first, i->second.m_value, i->second.m_important);
	}
}

void litehtml::style::add_property( const string_hash & name, const tchar_t* val, const tchar_t* baseurl, bool important )
{
	if( name.empty() || !val )
	{
		return;
	}

	// Add baseurl for background image
    switch ( name.GetHashCode() )
    {
        case "background-image"_hash:
        {
            add_parsed_property( name, val, important );
            if ( baseurl )
            {
                add_parsed_property( _t( "background-image-baseurl" ), baseurl, important );
            }
        }
        break;

        case "border-spacing"_hash:
        {
            string_vector tokens;
            split_string( val, tokens, _t( " " ) );
            if ( tokens.size() == 1 )
            {
                add_property( _t( "-litehtml-border-spacing-x" ), tokens[ 0 ].c_str(), baseurl, important );
                add_property( _t( "-litehtml-border-spacing-y" ), tokens[ 0 ].c_str(), baseurl, important );
            }
            else if ( tokens.size() == 2 )
            {
                add_property( _t( "-litehtml-border-spacing-x" ), tokens[ 0 ].c_str(), baseurl, important );
                add_property( _t( "-litehtml-border-spacing-y" ), tokens[ 1 ].c_str(), baseurl, important );
            }
        }
        break;

        // Parse borders shorthand properties

        case "border"_hash:
        {
            string_vector tokens;
            split_string( val, tokens, _t( " " ), _t( "" ), _t( "(" ) );
            int idx;
            tstring str;
            for ( string_vector::const_iterator tok = tokens.cbegin(); tok != tokens.cend(); tok++ )
            {
                idx = value_index( tok->c_str(), border_style_strings, -1 );
                if ( idx >= 0 )
                {
                    add_property( _t( "border-left-style" ), tok->c_str(), baseurl, important );
                    add_property( _t( "border-right-style" ), tok->c_str(), baseurl, important );
                    add_property( _t( "border-top-style" ), tok->c_str(), baseurl, important );
                    add_property( _t( "border-bottom-style" ), tok->c_str(), baseurl, important );
                }
                else
                {
                    if ( web_color::is_color( tok->c_str() ) )
                    {
                        add_property( _t( "border-left-color" ), tok->c_str(), baseurl, important );
                        add_property( _t( "border-right-color" ), tok->c_str(), baseurl, important );
                        add_property( _t( "border-top-color" ), tok->c_str(), baseurl, important );
                        add_property( _t( "border-bottom-color" ), tok->c_str(), baseurl, important );
                    }
                    else
                    {
                        add_property( _t( "border-left-width" ), tok->c_str(), baseurl, important );
                        add_property( _t( "border-right-width" ), tok->c_str(), baseurl, important );
                        add_property( _t( "border-top-width" ), tok->c_str(), baseurl, important );
                        add_property( _t( "border-bottom-width" ), tok->c_str(), baseurl, important );
                    }
                }
            }
        }
        break;

        case "border-left"_hash:
        case "border-right"_hash:
        case "border-top"_hash:
        case "border-bottom"_hash:
        {
            string_vector tokens;
            split_string( val, tokens, _t( " " ), _t( "" ), _t( "(" ) );

            static const string_hash border_table[ 12 ] =
            {
                _t( "border-left-style" ),
                _t( "border-left-color" ),
                _t( "border-left-width" ),
                _t( "border-right-style" ),
                _t( "border-right-color" ),
                _t( "border-right-width" ),
                _t( "border-top-style" ),
                _t( "border-top-color" ),
                _t( "border-top-width" ),
                _t( "border-bottom-style" ),
                _t( "border-bottom-color" ),
                _t( "border-bottom-width" )
            };

            const string_hash * name_table;

            switch ( name.GetHashCode() )
            {
                case "border-left"_hash:
                {
                    name_table = &border_table[ 0 ];
                }
                break;

                case "border-right"_hash:
                {
                    name_table = &border_table[ 3 ];
                }
                break;

                case "border-top"_hash:
                {
                    name_table = &border_table[ 6 ];
                }
                break;

                case "border-bottom"_hash:
                {
                    name_table = &border_table[ 9 ];
                }
                break;
            }

            int idx;

            for ( string_vector::const_iterator tok = tokens.cbegin(); tok != tokens.cend(); tok++ )
            {
                const tchar_t * c_string = tok->c_str();

                idx = value_index( c_string, border_style_strings, -1 );
                if ( idx >= 0 )
                {
                    add_property( name_table[ 0 ], c_string, baseurl, important );
                }
                else
                {
                    if ( web_color::is_color( c_string ) )
                    {
                        add_property( name_table[ 1 ], c_string, baseurl, important );
                    }
                    else
                    {
                        add_property( name_table[ 2 ], c_string, baseurl, important );
                    }
                }
            }
        }
        break;

        // Parse border radius shorthand properties
        case "border-bottom-left-radius"_hash:
        {
            string_vector tokens;
            split_string( val, tokens, _t( " " ) );
            if ( tokens.size() >= 2 )
            {
                add_property( _t( "border-bottom-left-radius-x" ), tokens[ 0 ].c_str(), baseurl, important );
                add_property( _t( "border-bottom-left-radius-y" ), tokens[ 1 ].c_str(), baseurl, important );
            }
            else if ( tokens.size() == 1 )
            {
                add_property( _t( "border-bottom-left-radius-x" ), tokens[ 0 ].c_str(), baseurl, important );
                add_property( _t( "border-bottom-left-radius-y" ), tokens[ 0 ].c_str(), baseurl, important );
            }

        }
        break;

        case "border-bottom-right-radius"_hash:
        {
            string_vector tokens;
            split_string( val, tokens, _t( " " ) );
            if ( tokens.size() >= 2 )
            {
                add_property( _t( "border-bottom-right-radius-x" ), tokens[ 0 ].c_str(), baseurl, important );
                add_property( _t( "border-bottom-right-radius-y" ), tokens[ 1 ].c_str(), baseurl, important );
            }
            else if ( tokens.size() == 1 )
            {
                add_property( _t( "border-bottom-right-radius-x" ), tokens[ 0 ].c_str(), baseurl, important );
                add_property( _t( "border-bottom-right-radius-y" ), tokens[ 0 ].c_str(), baseurl, important );
            }

        }
        break;

        case "border-top-right-radius"_hash:
        {
            string_vector tokens;
            split_string( val, tokens, _t( " " ) );
            if ( tokens.size() >= 2 )
            {
                add_property( _t( "border-top-right-radius-x" ), tokens[ 0 ].c_str(), baseurl, important );
                add_property( _t( "border-top-right-radius-y" ), tokens[ 1 ].c_str(), baseurl, important );
            }
            else if ( tokens.size() == 1 )
            {
                add_property( _t( "border-top-right-radius-x" ), tokens[ 0 ].c_str(), baseurl, important );
                add_property( _t( "border-top-right-radius-y" ), tokens[ 0 ].c_str(), baseurl, important );
            }

        }
        break;

        case "border-top-left-radius"_hash:
        {
            string_vector tokens;
            split_string( val, tokens, _t( " " ) );
            if ( tokens.size() >= 2 )
            {
                add_property( _t( "border-top-left-radius-x" ), tokens[ 0 ].c_str(), baseurl, important );
                add_property( _t( "border-top-left-radius-y" ), tokens[ 1 ].c_str(), baseurl, important );
            }
            else if ( tokens.size() == 1 )
            {
                add_property( _t( "border-top-left-radius-x" ), tokens[ 0 ].c_str(), baseurl, important );
                add_property( _t( "border-top-left-radius-y" ), tokens[ 0 ].c_str(), baseurl, important );
            }

        }
        break;

        // Parse border-radius shorthand properties
        case "border-radius"_hash:
        {
            string_vector tokens;
            split_string( val, tokens, _t( "/" ) );
            if ( tokens.size() == 1 )
            {
                add_property( _t( "border-radius-x" ), tokens[ 0 ].c_str(), baseurl, important );
                add_property( _t( "border-radius-y" ), tokens[ 0 ].c_str(), baseurl, important );
            }
            else if ( tokens.size() >= 2 )
            {
                add_property( _t( "border-radius-x" ), tokens[ 0 ].c_str(), baseurl, important );
                add_property( _t( "border-radius-y" ), tokens[ 1 ].c_str(), baseurl, important );
            }
        }
        break;

        case "border-radius-x"_hash:
        {
            string_vector tokens;
            split_string( val, tokens, _t( " " ) );

            static const string_hash border_radius_x_table[ 4 ] =
            {
                _t( "border-top-left-radius-x" ),
                _t( "border-top-right-radius-x" ),
                _t( "border-bottom-right-radius-x" ),
                _t( "border-bottom-left-radius-x" )
            };

            expand_shorthand_properties( tokens, border_radius_x_table, important );
        }
        break;

        case "border-radius-y"_hash:
        {
            string_vector tokens;
            split_string( val, tokens, _t( " " ) );

            static const string_hash border_radius_y_table[ 4 ] =
            {
                _t( "border-top-left-radius-y" ),
                _t( "border-top-right-radius-y" ),
                _t( "border-bottom-right-radius-y" ),
                _t( "border-bottom-left-radius-y" )
            };

            expand_shorthand_properties( tokens, border_radius_y_table, important );
        }
        break;
    }

    switch( name.GetHashCode() )
    {
        // Parse list-style shorthand properties
        case "list-style"_hash:
        {
            add_parsed_property( _t( "list-style-type" ), _t( "disc" ), important );
            add_parsed_property( _t( "list-style-position" ), _t( "outside" ), important );
            add_parsed_property( _t( "list-style-image" ), _t( "" ), important );
            add_parsed_property( _t( "list-style-image-baseurl" ), _t( "" ), important );

            string_vector tokens;
            split_string( val, tokens, _t( " " ), _t( "" ), _t( "(" ) );
            for ( string_vector::iterator tok = tokens.begin(); tok != tokens.end(); tok++ )
            {
                int idx = value_index( tok->c_str(), list_style_type_strings, -1 );
                if ( idx >= 0 )
                {
                    add_parsed_property( _t( "list-style-type" ), *tok, important );
                }
                else
                {
                    idx = value_index( tok->c_str(), list_style_position_strings, -1 );
                    if ( idx >= 0 )
                    {
                        add_parsed_property( _t( "list-style-position" ), *tok, important );
                    }
                    else if ( !t_strncmp( val, _t( "url" ), 3 ) )
                    {
                        add_parsed_property( _t( "list-style-image" ), *tok, important );
                        if ( baseurl )
                        {
                            add_parsed_property( _t( "list-style-image-baseurl" ), baseurl, important );
                        }
                    }
                }
            }
        }
        break;

        // Add baseurl for background image
        case "list-style-image"_hash:
        {
            add_parsed_property( name, val, important );
            if ( baseurl )
            {
                add_parsed_property( _t( "list-style-image-baseurl" ), baseurl, important );
            }
        }
        break;

        // Parse background shorthand properties
        case "background"_hash:
        {
            parse_short_background( val, baseurl, important );

        }
        break;

        // Parse margin and padding shorthand properties
        case "margin"_hash:
        case "padding"_hash:
        {
            string_vector tokens;
            split_string( val, tokens, _t( " " ) );

            static const string_hash margin_table[ 4 ] =
            {
                _t( "margin-top" ),
                _t( "margin-right" ),
                _t( "margin-bottom" ),
                _t( "margin-left" )
            };

            static const string_hash padding_table[ 4 ] =
            {
                _t( "padding-top" ),
                _t( "padding-right" ),
                _t( "padding-bottom" ),
                _t( "padding-left" )
            };

            const string_hash( &name_table )[ 4 ] = ( name.GetHashCode() == "margin"_hash  ) ? margin_table : padding_table;

            expand_shorthand_properties( tokens, name_table, important );
        }
        break;

        // Parse border-* shorthand properties
        case "border-left"_hash:
        {
            parse_short_border( _t( "border-left" ), val, important );
        }
        break;

        case "border-right"_hash:
        {
            parse_short_border( _t( "border-right" ), val, important );
        }
        break;

        case "border-top"_hash:
        {
            parse_short_border( _t( "border-top" ), val, important );
        }
        break;

        case "border-bottom"_hash:
        {
            parse_short_border( _t( "border-bottom" ), val, important );
        }
        break;

        // Parse border-width/style/color shorthand properties
        case "border-width"_hash:
        case "border-style"_hash:
        case "border-color"_hash:
        {
            static const string_hash border_width_table[ 4 ] =
            {
                _t( "border-top-width" ),
                _t( "border-right-width" ),
                _t( "border-bottom-width" ),
                _t( "border-left-width" ),
            };

            static const string_hash border_style_table[ 4 ] =
            {
                _t( "border-top-style" ),
                _t( "border-right-style" ),
                _t( "border-bottom-style" ),
                _t( "border-left-style" ),
            };

            static const string_hash border_color_table[ 4 ] =
            {
                _t( "border-top-color" ),
                _t( "border-right-color" ),
                _t( "border-bottom-color" ),
                _t( "border-left-color" ),
            };

            string_vector tokens;
            split_string( val, tokens, _t( " " ) );

            if ( name.GetHashCode() == "border-width"_hash )
            {
                expand_shorthand_properties( tokens, border_width_table, important );
            }
            else if ( name.GetHashCode() == "border-style"_hash )
            {
                expand_shorthand_properties( tokens, border_style_table, important );
            }
            else
            {
                expand_shorthand_properties( tokens, border_color_table, important );
            }
        }
        break;

        // Parse font shorthand properties
        case "font"_hash:
        {
            parse_short_font( val, important );
        }
        break;

        default:
        {
            add_parsed_property( name, val, important );
        }
    }
}

void litehtml::style::expand_shorthand_properties( const string_vector &tokens, const string_hash (& name_array)[4], bool important )
{
	static const int index_table_table[ 4 ][ 4 ] =
	{
		{ 0, 0, 0, 0 },
		{ 0, 1, 0, 1 },
		{ 0, 1, 2, 1 },
		{ 0, 1, 2, 3 }
	};

	const int ( &index_table )[ 4 ] = index_table_table[ std::min< string_vector::size_type >( tokens.size() - 1, 3 ) ];

	add_parsed_property( name_array[ 0 ], tokens[ index_table[ 0 ] ], important);
	add_parsed_property( name_array[ 1 ], tokens[ index_table[ 1 ] ], important);
	add_parsed_property( name_array[ 2 ], tokens[ index_table[ 2 ] ], important);
	add_parsed_property( name_array[ 3 ], tokens[ index_table[ 3 ] ], important);
}

void litehtml::style::parse_short_border( const tstring& prefix, const tstring& val, bool important )
{
	string_vector tokens;
	split_string(val, tokens, _t(" "), _t(""), _t("("));
	if(tokens.size() >= 3)
	{
		add_parsed_property(prefix + _t("-width"),	tokens[0], important);
		add_parsed_property(prefix + _t("-style"),	tokens[1], important);
		add_parsed_property(prefix + _t("-color"),	tokens[2], important);
	} else if(tokens.size() == 2)
	{
		if(iswdigit(tokens[0][0]) || value_index(val.c_str(), border_width_strings) >= 0)
		{
			add_parsed_property(prefix + _t("-width"),	tokens[0], important);
			add_parsed_property(prefix + _t("-style"),	tokens[1], important);
		} else
		{
			add_parsed_property(prefix + _t("-style"),	tokens[0], important);
			add_parsed_property(prefix + _t("-color"),	tokens[1], important);
		}
	}
}

static std::vector<litehtml::string_hash> position_string = { _t( "left" ), _t( "right" ), _t( "top" ), _t( "bottom" ), _t( "center" ) };

void litehtml::style::parse_short_background( const tstring& val, const tchar_t* baseurl, bool important )
{
	add_parsed_property(_t("background-color"),			_t("transparent"),	important);
	add_parsed_property(_t("background-image"),			_t(""),				important);
	add_parsed_property(_t("background-image-baseurl"), _t(""),				important);
	add_parsed_property(_t("background-repeat"),		_t("repeat"),		important);
	add_parsed_property(_t("background-origin"),		_t("padding-box"),	important);
	add_parsed_property(_t("background-clip"),			_t("border-box"),	important);
	add_parsed_property(_t("background-attachment"),	_t("scroll"),		important);

	if(val == _t("none"))
	{
		return;
	}

	string_vector tokens;
	split_string(val, tokens, _t(" "), _t(""), _t("("));
	bool origin_found = false;
	for(string_vector::iterator tok = tokens.begin(); tok != tokens.end(); tok++)
	{
		if(web_color::is_color(tok->c_str()))
		{
			add_parsed_property(_t("background-color"), *tok, important);
		} else if(tok->substr(0, 3) == _t("url"))
		{
			add_parsed_property(_t("background-image"), *tok, important);
			if(baseurl)
			{
				add_parsed_property(_t("background-image-baseurl"), baseurl, important);
			}

		} else if( value_in_list(tok->c_str(), background_repeat_strings) )
		{
			add_parsed_property(_t("background-repeat"), *tok, important);
		} else if( value_in_list(tok->c_str(), background_attachment_strings) )
		{
			add_parsed_property(_t("background-attachment"), *tok, important);
		} else if( value_in_list(tok->c_str(), background_box_strings) )
		{
			if(!origin_found)
			{
				add_parsed_property(_t("background-origin"), *tok, important);
				origin_found = true;
			} else
			{
				add_parsed_property(_t("background-clip"),*tok, important);
			}
		} else if( value_in_list(tok->c_str(), position_string) ||
					iswdigit((*tok)[0]) ||
					(*tok)[0] == _t('-')	||
					(*tok)[0] == _t('.')	||
					(*tok)[0] == _t('+'))
		{
			if(m_properties.find(_t("background-position")) != m_properties.end())
			{
				m_properties[_t("background-position")].m_value = m_properties[_t("background-position")].m_value + _t(" ") + *tok;
			} else
			{
				add_parsed_property(_t("background-position"), *tok, important);
			}
		}
	}
}

void litehtml::style::parse_short_font( const tstring& val, bool important )
{
	add_parsed_property(_t("font-style"),	_t("normal"),	important);
	add_parsed_property(_t("font-variant"),	_t("normal"),	important);
	add_parsed_property(_t("font-weight"),	_t("normal"),	important);
	add_parsed_property(_t("font-size"),		_t("medium"),	important);
	add_parsed_property(_t("line-height"),	_t("normal"),	important);

	string_vector tokens;
	split_string(val, tokens, _t(" "), _t(""), _t("\""));

	int idx = 0;
	bool was_normal = false;
	bool is_family = false;
	tstring font_family;
	for(string_vector::iterator tok = tokens.begin(); tok != tokens.end(); tok++)
	{
		idx = value_index(tok->c_str(), font_style_strings);
		if(!is_family)
		{
			if(idx >= 0)
			{
				if(idx == 0 && !was_normal)
				{
					add_parsed_property(_t("font-weight"),		*tok, important);
					add_parsed_property(_t("font-variant"),		*tok, important);
					add_parsed_property(_t("font-style"),		*tok, important);
				} else
				{
					add_parsed_property(_t("font-style"),		*tok, important);
				}
			} else
			{
				if(value_in_list(tok->c_str(), font_weight_strings))
				{
					add_parsed_property(_t("font-weight"),		*tok, important);
				} else
				{
					if(value_in_list(tok->c_str(), font_variant_strings))
					{
						add_parsed_property(_t("font-variant"),	*tok, important);
					} else if( iswdigit((*tok)[0]) )
					{
						string_vector szlh;
						split_string(*tok, szlh, _t("/"));

						if(szlh.size() == 1)
						{
							add_parsed_property(_t("font-size"),	szlh[0], important);
						} else	if(szlh.size() >= 2)
						{
							add_parsed_property(_t("font-size"),	szlh[0], important);
							add_parsed_property(_t("line-height"),	szlh[1], important);
						}
					} else
					{
						is_family = true;
						font_family += *tok;
					}
				}
			}
		} else
		{
			font_family += *tok;
		}
	}
	add_parsed_property(_t("font-family"), font_family, important);
}

static litehtml::string_hash _white_space = _t( "white-space" );

void litehtml::style::add_parsed_property( const string_hash & name, const tstring& val, bool important )
{
	bool is_valid = true;
	if (name == _white_space)
	{
		if (!value_in_list(val.c_str(), white_space_strings))
		{
			is_valid = false;
		}
	}

	if (is_valid)
	{
		props_map::iterator prop = m_properties.find(name);
		if (prop != m_properties.end())
		{
			if (!prop->second.m_important || (important && prop->second.m_important))
			{
				prop->second.m_value = val;
				prop->second.m_important = important;
			}
		}
		else
		{
			m_properties.insert( std::make_pair( name, property_value(val, important) ) );
		}
	}
}

void litehtml::style::remove_property( const tstring& name, bool important )
{
	props_map::iterator prop = m_properties.find(name);
	if(prop != m_properties.end())
	{
		if( !prop->second.m_important || (important && prop->second.m_important) )
		{
			m_properties.erase(prop);
		}
	}
}
