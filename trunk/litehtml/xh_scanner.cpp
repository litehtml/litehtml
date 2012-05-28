#include "xh_scanner.h"
#include "string.h"
#include <ctype.h>

namespace litehtml 
{

  
    // case sensitive string equality test
    // s_lowcase shall be lowercase string
    inline bool equal(const wchar* s, const wchar* s1, size_t length)
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
        default: return wcsncmp(s,s1,length) == 0;
      }
    }

    const wchar* scanner::get_value() 
    {
      value[value_length] = 0;
      return value;
    }

    const wchar* scanner::get_attr_name() 
    {
      attr_name[attr_name_length] = 0;
      return attr_name;
    }

    const wchar* scanner::get_tag_name() 
    {
      tag_name[tag_name_length] = 0;
      return tag_name;
    }
        
    scanner::token_type scanner::scan_body() 
    {
      wchar c = get_char();

      value_length = 0;
         
      bool ws = false;

      if(c == 0) return TT_EOF;
      else if(c == '<') return scan_tag();
      else if(c == '&')
         c = scan_entity();
      else
         ws = is_whitespace(c);
        
      while(true) 
      {
        append_value(c);
        c = input.get_char();
        if(c == 0)  { push_back(c); break; }
        if(c == '<') { push_back(c); break; }
        if(c == '&') { push_back(c); break; }
          
        if(is_whitespace(c) != ws) 
        {
          push_back(c);
          break;
        }

      }
      return ws? TT_SPACE:TT_WORD;
    }

    scanner::token_type scanner::scan_head()
    {
      wchar c = skip_whitespace();

      if(c == '>') { c_scan = &scanner::scan_body; return scan_body(); }
      if(c == '/')
      {
         wchar t = get_char();
         if(t == '>')   { c_scan = &scanner::scan_body; return TT_TAG_END; }
         else { push_back(t); return TT_ERROR; } // erroneous situtation - standalone '/'
      }

      attr_name_length = 0;
      value_length = 0;

      // attribute name...
      while(c != '=') 
      {
        if( c == 0) return TT_EOF;
        if( c == '>' ) { push_back(c); return TT_ATTR; } // attribute without value (HTML style)
        if( is_whitespace(c) )
        {
          c = skip_whitespace();
          if(c != '=') { push_back(c); return TT_ATTR; } // attribute without value (HTML style)
          else break;
        }
        if( c == '<') return TT_ERROR;
        append_attr_name(c);
        c = get_char();
      }

      c = skip_whitespace();
      // attribute value...
      
      if(c == '\"')
        while(c = get_char())
        {
            if(c == '\"') return TT_ATTR;
            if(c == '&') c = scan_entity();
            append_value(c);
        }
      else if(c == '\'') // allowed in html
        while(c = get_char())
        {
            if(c == '\'') return TT_ATTR;
            if(c == '&') c = scan_entity();
            append_value(c);
        }
      else  // scan token, allowed in html: e.g. align=center
        do
        {
            if( is_whitespace(c) ) return TT_ATTR;
            /* these two removed in favour of better html support:
            if( c == '/' || c == '>' ) { push_back(c); return TT_ATTR; }
            if( c == '&' ) c = scan_entity();*/
            if( c == '>' ) { push_back(c); return TT_ATTR; }
            append_value(c);
        } while(c = get_char());

      return TT_ERROR;
    }

    // caller already consumed '<'
    // scan header start or tag tail
    scanner::token_type scanner::scan_tag() 
    {
      tag_name_length = 0;

      wchar c = get_char();

      bool is_tail = c == '/';
      if(is_tail) c = get_char();
      
      while(c) 
      {
        if(is_whitespace(c)) { c = skip_whitespace(); break; }
        if(c == '/' || c == '>') break;
        append_tag_name(c);

        switch(tag_name_length)
        {
        case 3: 
          if(equal(tag_name,L"!--",3))  { c_scan = &scanner::scan_comment; return TT_COMMENT_START; }
          break;
        case 8:
          if( equal(tag_name,L"![CDATA[",8) ) { c_scan = &scanner::scan_cdata; return TT_CDATA_START; }
		  if( equal(tag_name,L"!DOCTYPE",8) ) { c_scan = &scanner::scan_entity_decl; return TT_DOCTYPE_START; }
          break;
        case 7:
          if( equal(tag_name,L"!ENTITY",8) ) { c_scan = &scanner::scan_entity_decl; return TT_ENTITY_START; }
          break;
        }

        c = get_char();
      }
 
      if(c == 0) return TT_ERROR;    
              
      if(is_tail)
      {
          if(c == '>') return TT_TAG_END;
          return TT_ERROR;
      }
      else 
           push_back(c);
      
      c_scan = &scanner::scan_head;
      return TT_TAG_START;
    }

    // skip whitespaces.
    // returns first non-whitespace char
    wchar scanner::skip_whitespace() 
    {
        while(wchar c = get_char()) 
        {
            if(!is_whitespace(c)) return c;
        }
        return 0;
    }

    void    scanner::push_back(wchar c) { input_char = c; }

    wchar scanner::get_char() 
    { 
      if(input_char) { wchar t(input_char); input_char = 0; return t; }
      return input.get_char();
    }


    // caller consumed '&'
    wchar scanner::scan_entity() 
    {
      wchar buf[32];
      int i = 0;
      wchar t;
      for(; i < 31 ; ++i )
      {
        t = get_char();

		if(t == ';')
			break;

        if(t == 0) return TT_EOF;
        if( !isalnum(t) )
        {
          push_back(t);
          break; // appears a erroneous entity token.
                 // but we try to use it.
        }
        buf[i] = char(t); 
      }
      buf[i] = 0;
      if(i == 2)  
      {
        if(equal(buf,L"gt",2)) return '>';
        if(equal(buf,L"lt",2)) return '<';
      }
      else if(i == 3 && equal(buf,L"amp",3)) 
        return '&';
      else if(i == 4) 
      {
        if(equal(buf,L"apos",4)) return '\'';
        if(equal(buf,L"quot",4)) return '\"';
      }
      t = resolve_entity(buf,i);
      if(t) return t;
      // no luck ...
      append_value('&');
      for(int n = 0; n < i; ++n)
        append_value(buf[n]);
      return ';';
    }

    bool scanner::is_whitespace(wchar c)
    {
        return c <= ' ' 
            && (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f');
    }

    void scanner::append_value(wchar c) 
    { 
      if(value_length < (MAX_TOKEN_SIZE - 1)) 
        value[value_length++] = c;
    }

    void scanner::append_attr_name(wchar c)
    {
      if(attr_name_length < (MAX_NAME_SIZE - 1)) 
        attr_name[attr_name_length++] = c;
    }

    void scanner::append_tag_name(wchar c)
    {
      if(tag_name_length < (MAX_NAME_SIZE - 1)) 
        tag_name[tag_name_length++] = c;
    }

    scanner::token_type scanner::scan_comment()
    {
      if(got_tail)
      {
        c_scan = &scanner::scan_body;
        got_tail = false;
        return TT_COMMENT_END;
      }
      for(value_length = 0; value_length < (MAX_TOKEN_SIZE - 1); ++value_length)
      {
        wchar c = get_char();
        if( c == 0) return TT_EOF;
        value[value_length] = c;
        
        if(value_length >= 2 
          && value[value_length] == '>' 
          && value[value_length - 1] == '-' 
          && value[value_length - 2] == '-')
        {
          got_tail = true;
          value_length -= 2;
          break;
        }
      }
      return TT_DATA;
    }

    scanner::token_type scanner::scan_cdata()
    {
      if(got_tail)
      {
        c_scan = &scanner::scan_body;
        got_tail = false;
        return TT_CDATA_END;
      }
      for(value_length = 0; value_length < (MAX_TOKEN_SIZE - 1); ++value_length)
      {
        wchar c = get_char();
        if( c == 0) return TT_EOF;
        value[value_length] = c;

        if(value_length >= 2 
          && value[value_length] == '>' 
          && value[value_length - 1] == ']' 
          && value[value_length - 2] == ']')
        {
          got_tail = true;
          value_length -= 2;
          break;
        }
      }
      return TT_DATA;
    }

    scanner::token_type scanner::scan_pi()
    {
      if(got_tail)
      {
        c_scan = &scanner::scan_body;
        got_tail = false;
        return TT_PI_END;
      }
      for(value_length = 0; value_length < (MAX_TOKEN_SIZE - 1); ++value_length)
      {
        wchar c = get_char();
        if( c == 0) return TT_EOF;
        value[value_length] = c;

        if(value_length >= 1 
          && value[value_length] == '>' 
          && value[value_length - 1] == '?')
        {
          got_tail = true;
          value_length -= 1;
          break;
        }
      }
      return TT_DATA;
    }

	scanner::token_type scanner::scan_entity_decl()
	{
		if(got_tail)
		{
			c_scan = &scanner::scan_body;
			got_tail = false;
			return TT_ENTITY_END;
		}
		wchar t;
		unsigned int tc = 0;
		for(value_length = 0; value_length < (MAX_TOKEN_SIZE - 1); ++value_length)
		{
			t = get_char();
			if( t == 0 ) return TT_EOF;
			value[value_length] = t;
			if(t == '\"') tc++;
			else if( t == '>' && (tc & 1) == 0 )
			{
				got_tail = true;
				break;
			}
		}
		return TT_DATA;
	}

	scanner::token_type scanner::scan_doctype_decl()
	{
		if(got_tail)
		{
			c_scan = &scanner::scan_body;
			got_tail = false;
			return TT_DOCTYPE_END;
		}
		wchar t;
		unsigned int tc = 0;
		for(value_length = 0; value_length < (MAX_TOKEN_SIZE - 1); ++value_length)
		{
			t = get_char();
			if( t == 0 ) return TT_EOF;
			value[value_length] = t;
			if(t == '\"') tc++;
			else if( t == '>' && (tc & 1) == 0 )
			{
				got_tail = true;
				break;
			}
		}
		return TT_DATA;
	}

	struct
	{
		wchar szCode[10];
		wchar Code;
	} g_HTMLCodes[] = 
	{
		{L"&quot;",L'"'},
		{L"&amp;",L'&'},
		{L"&lt;",L'<'},
		{L"&gt;",L'>'},
		{L"&nbsp;",L'�'},
		{L"&iexcl;",L'�'},
		{L"&cent;",L'�'},
		{L"&pound;",L'�'},
		{L"&curren;",L'�'},
		{L"&yen;",L'�'},
		{L"&brvbar;",L'�'},
		{L"&sect;",L'�'},
		{L"&uml;",L'�'},
		{L"&copy;",L'�'},
		{L"&ordf;",L'�'},
		{L"&laquo;",L'�'},
		{L"&not;",L'�'},
		{L"&shy;",L'�'},
		{L"&reg;",L'�'},
		{L"&macr;",L'�'},
		{L"&deg;",L'�'},
		{L"&plusmn;",L'�'},
		{L"&sup2;",L'�'},
		{L"&sup3;",L'�'},
		{L"&acute;",L'�'},
		{L"&micro;",L'�'},
		{L"&para;",L'�'},
		{L"&middot;",L'�'},
		{L"&cedil;",L'�'},
		{L"&sup1;",L'�'},
		{L"&ordm;",L'�'},
		{L"&raquo;",L'�'},
		{L"&frac14;",L'�'},
		{L"&frac12;",L'�'},
		{L"&frac34;",L'�'},
		{L"&iquest;",L'�'},
		{L"&Agrave;",L'�'},
		{L"&Aacute;",L'�'},
		{L"&Acirc;",L'�'},
		{L"&Atilde;",L'�'},
		{L"&Auml;",L'�'},
		{L"&Aring;",L'�'},
		{L"&AElig;",L'�'},
		{L"&Ccedil;",L'�'},
		{L"&Egrave;",L'�'},
		{L"&Eacute;",L'�'},
		{L"&Ecirc;",L'�'},
		{L"&Euml;",L'�'},
		{L"&Igrave;",L'�'},
		{L"&Iacute;",L'�'},
		{L"&Icirc;",L'�'},
		{L"&Iuml;",L'�'},
		{L"&ETH;",L'�'},
		{L"&Ntilde;",L'�'},
		{L"&Ograve;",L'�'},
		{L"&Oacute;",L'�'},
		{L"&Ocirc;",L'�'},
		{L"&Otilde;",L'�'},
		{L"&Ouml;",L'�'},
		{L"&times;",L'�'},
		{L"&Oslash;",L'�'},
		{L"&Ugrave;",L'�'},
		{L"&Uacute;",L'�'},
		{L"&Ucirc;",L'�'},
		{L"&Uuml;",L'�'},
		{L"&Yacute;",L'�'},
		{L"&THORN;",L'�'},
		{L"&szlig;",L'�'},
		{L"&agrave;",L'�'},
		{L"&aacute;",L'�'},
		{L"&acirc;",L'�'},
		{L"&atilde;",L'�'},
		{L"&auml;",L'�'},
		{L"&aring;",L'�'},
		{L"&aelig;",L'�'},
		{L"&ccedil;",L'�'},
		{L"&egrave;",L'�'},
		{L"&eacute;",L'�'},
		{L"&ecirc;",L'�'},
		{L"&euml;",L'�'},
		{L"&igrave;",L'�'},
		{L"&iacute;",L'�'},
		{L"&icirc;",L'�'},
		{L"&iuml;",L'�'},
		{L"&eth;",L'�'},
		{L"&ntilde;",L'�'},
		{L"&ograve;",L'�'},
		{L"&oacute;",L'�'},
		{L"&ocirc;",L'�'},
		{L"&otilde;",L'�'},
		{L"&ouml;",L'�'},
		{L"&divide;",L'�'},
		{L"&oslash;",L'�'},
		{L"&ugrave;",L'�'},
		{L"&uacute;",L'�'},
		{L"&ucirc;",L'�'},
		{L"&uuml;",L'�'},
		{L"&yacute;",L'�'},
		{L"&thorn;",L'�'},
		{L"&yuml;",L'�'},
		{L"&OElig;",L'E'},
		{L"&oelig;",L'e'},
		{L"&Scaron;",L'S'},
		{L"&scaron;",L's'},
		{L"&Yuml;",L'�'},
		{L"&fnof;",L'f'},
		{L"&circ;",L'^'},
		{L"&tilde;",L'~'},
		{L"&ndash;",L'-'},
		{L"&mdash;",L'-'},
		{L"&lsquo;",L'�'},
		{L"&rsquo;",L'�'},
		{L"&sbquo;",L','},
		{L"&ldquo;",L'\"'},
		{L"&rdquo;",L'\"'},
		{L"&bdquo;",L'�'},
		{L"&dagger;",L'�'},
		{L"&Dagger;",L'�'},
		{L"&bull;",L'�'},
		{L"&hellip;",L'�'},
		{L"&permil;",L'�'},
		{L"&lsaquo;",L'�'},
		{L"&rsaquo;",L'�'},
		{L"&euro;",L'�'},
		{L"&trade;",L'�'},
		{{(char)0x97,(char)0x00},L'-'},
		{{(char)0xA0,(char)0x00},L' '},
		{L"",0}
	};

	litehtml::wchar scanner::resolve_entity( const wchar* buf, int buf_size )
	{
		for(int i=0; g_HTMLCodes[i].szCode[0]; i++)
		{
			if(!wcsncmp(g_HTMLCodes[i].szCode + 1, buf, buf_size))
			{
				return g_HTMLCodes[i].Code;
			}
		}
		return 0;
	}



}
 
