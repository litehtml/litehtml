//| 
//| simple and fast XML/HTML scanner/tokenizer
//|
//| (C) Andrew Fedoniouk @ terrainformatica.com
//|

#pragma once
#include "instream.h"

namespace litehtml 
{

	struct html_entities
	{
		tchar_t		szCode[20];
		ucode_t 	Code;
	};

	class scanner
	{
	public:
		enum token_type 
		{
			TT_ERROR = -1,
			TT_EOF = 0,

			TT_TAG_START,   // <tag ...
			//                  ^-- happens here
			TT_TAG_END,     // </tag>
			//                   ^-- happens here 
			TT_TAG_END_EMPTY, // <tag ... />
			//                    ^-- or here 
			TT_ATTR,        // <tag attr="value" >      
			//                  ^-- happens here   
			TT_WORD,
			TT_SPACE,

			TT_DATA,        // content of followings:

			TT_COMMENT_START, TT_COMMENT_END, // after "<!--" and "-->"
			TT_CDATA_START, TT_CDATA_END,     // after "<![CDATA[" and "]]>"
			TT_PI_START, TT_PI_END,           // after "<?" and "?>"
			TT_ENTITY_START, TT_ENTITY_END,   // after "<!ENTITY" and ">"
			TT_DOCTYPE_START, TT_DOCTYPE_END,   // after "<!DOCTYPE" and ">"
		};

	public:

		scanner(instream& is): 
		  token(TT_EOF),
			  input(is),
			  input_char(0),
			  got_tail(false) { c_scan = &scanner::scan_body; }

		  // get next token
		  token_type      get_token() { return (this->*c_scan)(); } 

		  // get value of TT_WORD, TT_SPACE, TT_ATTR and TT_DATA
		  const tchar_t*    get_value();

		  // get attribute name
		  const tchar_t*    get_attr_name();

		  // get tag name
		  const tchar_t*    get_tag_name();

		  // should be override to resolve entities, e.g. &nbsp;
		  virtual ucode_t   resolve_entity(const tchar_t* buf, int buf_size);

	private: /* methods */

		typedef token_type (scanner::*scan)();
		scan        c_scan; // current 'reader'

		// content 'readers'
		token_type  scan_body();
		token_type  scan_head();
		token_type  scan_comment();
		token_type  scan_cdata();
		token_type  scan_pi();
		token_type  scan_tag();
		token_type  scan_entity_decl();
		token_type  scan_doctype_decl();
		token_type  scan_raw_body();

		ucode_t     skip_whitespace();
		void        push_back(ucode_t c);

		ucode_t		get_char();
		ucode_t		scan_entity();

		bool        is_whitespace(ucode_t c);

		void        append_value(ucode_t c);
		void        append_attr_name(ucode_t c);
		void        append_tag_name(ucode_t c);
		bool		equal(const tchar_t* s, const tchar_t* s1, size_t length);

	private:

		token_type  token;

		tstring		m_value;
		tstring		m_tag_name;
		tstring		m_attr_name;

		instream&   input;
		ucode_t		input_char;

		bool        got_tail; // aux flag used in scan_comment, etc. 

		static html_entities m_HTMLCodes[];

	};

	// case sensitive string equality test
	// s_lowcase shall be lowercase string
	inline bool scanner::equal(const tchar_t* s, const tchar_t* s1, size_t length)
	{
		switch(length)
		{
		case 8: if(s1[7] != s[7]) return false;
		case 7: if(s1[6] != s[6]) return false;
		case 6: if(s1[5] != s[5]) return false;
		case 5: if(s1[4] != s[4]) return false;
		case 4: if(s1[3] != s[3]) return false;
		case 3: if(s1[2] != s[2]) return false;
		case 2: if(s1[1] != s[1]) return false;
		case 1: if(s1[0] != s[0]) return false;
		case 0: return true;
		default: return t_strncmp(s,s1,length) == 0;
		}
	}
}
