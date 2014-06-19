#ifndef __MARKUP
#define __MARKUP

//| 
//| simple and fast XML/HTML scanner/tokenizer
//|
//| (C) Andrew Fedoniouk @ terrainformatica.com
//|

namespace litehtml 
{
  struct instream 
  {
    virtual tchar_t get_char() = 0;
  };

	struct html_entities
	{
		tchar_t		szCode[20];
		wchar_t 	Code;
	};

  class scanner
  {
  public:
    enum token_type 
    {
      TT_ERROR = -1,
      TT_EOF = 0,

      TT_TAG_START,   // <tag ...
                      //     ^-- happens here
      TT_TAG_END,     // </tag>
                      //       ^-- happens here 
	  TT_TAG_END_EMPTY, // <tag ... />
                      //            ^-- or here 
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

    enum $ { MAX_TOKEN_SIZE = 1024, MAX_NAME_SIZE = 128 };

  public:
  
    scanner(instream& is): 
    	token(TT_EOF),
        value_length(0), 
        tag_name_length(0), 
        attr_name_length(0),
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
    virtual tchar_t   resolve_entity(const tchar_t* buf, int buf_size);
        
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

    tchar_t       skip_whitespace();
    void        push_back(tchar_t c);
  
    tchar_t       get_char();
    tchar_t       scan_entity();

    bool        is_whitespace(tchar_t c);
      
    void        append_value(tchar_t c);
    void        append_attr_name(tchar_t c);
    void        append_tag_name(tchar_t c);

  private: /* data */

    //enum state { TEXT = 0, MARKUP = 1, COMMENT = 2, CDATA = 3, PI = 4 };
    //state       where;
    token_type  token;

    tchar_t		value[MAX_TOKEN_SIZE];
    int			value_length;

    tchar_t		tag_name[MAX_NAME_SIZE];
    int			tag_name_length;

    tchar_t		attr_name[MAX_NAME_SIZE];
    int         attr_name_length;
  
    instream&   input;
    tchar_t		input_char;

    bool        got_tail; // aux flag used in scan_comment, etc. 

    static html_entities m_HTMLCodes[];

  };
}
 
#endif
