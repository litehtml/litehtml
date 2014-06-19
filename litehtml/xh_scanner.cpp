#include "html.h"
#include "xh_scanner.h"
#if !defined(WIN32) && !defined(WINCE)
#include <iconv.h>
#endif

namespace litehtml 
{

  
    // case sensitive string equality test
    // s_lowcase shall be lowercase string
    inline bool equal(const tchar_t* s, const tchar_t* s1, size_t length)
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

    const tchar_t* scanner::get_value() 
    {
      value[value_length] = 0;
      return value;
    }

    const tchar_t* scanner::get_attr_name() 
    {
      attr_name[attr_name_length] = 0;
      return attr_name;
    }

    const tchar_t* scanner::get_tag_name() 
    {
      tag_name[tag_name_length] = 0;
      return tag_name;
    }
        
    scanner::token_type scanner::scan_body() 
    {
      tchar_t c = get_char();

      value_length = 0;
         
      bool ws = false;

      if(c == 0) return TT_EOF;
      else if(c == '<') return scan_tag();
      else if(c == '&')
         c = scan_entity();
      else
         ws = is_whitespace(c);
      if(!ws)
	  {
		  while(true) 
		  {
			  append_value(c);
			  c = get_char();
			  if(c == 0)  { push_back(c); break; }
			  if(c == '<') { push_back(c); break; }
			  if(c == '&') { push_back(c); break; }

			  if(is_whitespace(c) != ws) 
			  {
				  push_back(c);
				  break;
			  }

		  }
	  } else
	  {
		  append_value(c);
	  }

      return ws ? TT_SPACE : TT_WORD;
    }

    scanner::token_type scanner::scan_head()
    {
      tchar_t c = skip_whitespace();

      if(c == '>') 
	  { 
		  if(tag_name_length == 6 && !t_strncasecmp(tag_name, _t("script"), tag_name_length))
		  {
			  c_scan = &scanner::scan_raw_body; 
			  return scan_raw_body(); 
		  } else
		  {
			  c_scan = &scanner::scan_body; 
			  return scan_body(); 
		  }
	  }

      if(c == '/')
      {
         tchar_t t = get_char();
         if(t == '>')   { c_scan = &scanner::scan_body; return TT_TAG_END_EMPTY; }
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
        while( (c = get_char()) )
        {
            if(c == '\"') return TT_ATTR;
            if(c == '&') c = scan_entity();
            append_value(c);
        }
      else if(c == '\'') // allowed in html
        while( (c = get_char()) )
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
        } while( (c = get_char()) );

      return TT_ERROR;
    }

    // caller already consumed '<'
    // scan header start or tag tail
    scanner::token_type scanner::scan_tag() 
    {
      tag_name_length = 0;

      tchar_t c = get_char();

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
          if(equal(tag_name,_t("!--"),3))  { c_scan = &scanner::scan_comment; return TT_COMMENT_START; }
          break;
        case 8:
			if( equal(tag_name,_t("![CDATA["),8) ) { c_scan = &scanner::scan_cdata; return TT_CDATA_START; }
			if( equal(tag_name,_t("!DOCTYPE"),8) ) { c_scan = &scanner::scan_entity_decl; return TT_DOCTYPE_START; }
          break;
		case 7:
			if( equal(tag_name,_t("!ENTITY"),8) ) { c_scan = &scanner::scan_entity_decl; return TT_ENTITY_START; }
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
    tchar_t scanner::skip_whitespace() 
    {
        while(tchar_t c = get_char()) 
        {
            if(!is_whitespace(c)) return c;
        }
        return 0;
    }

    void    scanner::push_back(tchar_t c) { input_char = c; }

    tchar_t scanner::get_char() 
    { 
      if(input_char) { tchar_t t(input_char); input_char = 0; return t; }
      return input.get_char();
    }


    // caller consumed '&'
    tchar_t scanner::scan_entity() 
    {
      tchar_t buf[32];
      int i = 0;
      tchar_t t;
      for(; i < 31 ; ++i )
      {
        t = get_char();

		if(t == ';')
			break;

        if(t == 0) return TT_EOF;
		if( !isalnum(t) && t != '#' )
        {
          push_back(t);
		  t = 0;
          break; // appears a erroneous entity token.
                 // but we try to use it.
        }
        buf[i] = char(t); 
      }
      buf[i] = 0;
/*
	  if(t != ';')
	  {
		  append_value('&');
		  for(int n = 0; n < i; ++n)
			  append_value(buf[n]);
		  return t;
	  }
*/
      if(i == 2)  
      {
        if(equal(buf,_t("gt"),2)) return '>';
        if(equal(buf,_t("lt"),2)) return '<';
      }
      else if(i == 3 && equal(buf,_t("amp"),3)) 
        return '&';
      else if(i == 4) 
      {
        if(equal(buf,_t("apos"),4)) return '\'';
        if(equal(buf,_t("quot"),4)) return '\"';
      }
      tchar_t entity = resolve_entity(buf,i);
      if(entity)
	  {
		  return entity;
	  }
      // no luck ...
      append_value('&');
      for(int n = 0; n < i; ++n)
        append_value(buf[n]);
	  if(t) return t;
	  return get_char();
    }

    bool scanner::is_whitespace(tchar_t c)
    {
        return c <= ' ' 
            && (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f');
    }

    void scanner::append_value(tchar_t c) 
    { 
      if(value_length < (MAX_TOKEN_SIZE - 1)) 
        value[value_length++] = c;
    }

    void scanner::append_attr_name(tchar_t c)
    {
      if(attr_name_length < (MAX_NAME_SIZE - 1)) 
        attr_name[attr_name_length++] = c;
    }

    void scanner::append_tag_name(tchar_t c)
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
        tchar_t c = get_char();
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
        tchar_t c = get_char();
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
        tchar_t c = get_char();
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
		tchar_t t;
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
		tchar_t t;
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

	litehtml::html_entities  scanner::m_HTMLCodes[] =
	{

		{ _t("&quot;"),		0x0022},
		{ _t("&amp;"),		0x0026},
		{ _t("&lt;"),		0x003C},
		{ _t("&gt;"),		0x003E},
		{ _t("&nbsp;"),		0x00A0},
		{ _t("&iexcl;"),	0x00A1},
		{ _t("&cent;"),		0x00A2},
		{ _t("&pound;"),	0x00A3},
		{ _t("&curren;"),	0x00A4},
		{ _t("&yen;"),		0x00A5},
		{ _t("&brvbar;"),	0x00A6},
		{ _t("&sect;"),		0x00A7},
		{ _t("&uml;"),		0x00A8},
		{ _t("&copy;"),		0x00A9},
		{ _t("&ordf;"),		0x00AA},
		{ _t("&laquo;"),	0x00AB},
		{ _t("&lsaquo;"),	0x00AB},
		{ _t("&not;"),		0x00AC},
		{ _t("&shy;"),		0x00AD},
		{ _t("&reg;"),		0x00AE},
		{ _t("&macr;"),		0x00AF},
		{ _t("&deg;"),		0x00B0},
		{ _t("&plusmn;"),	0x00B1},
		{ _t("&sup2;"),		0x00B2},
		{ _t("&sup3;"),		0x00B3},
		{ _t("&acute;"),	0x00B4},
		{ _t("&micro;"),	0x00B5},
		{ _t("&para;"),		0x00B6},
		{ _t("&middot;"),	0x00B7},
		{ _t("&cedil;"),	0x00B8},
		{ _t("&sup1;"),		0x00B9},
		{ _t("&ordm;"),		0x00BA},
		{ _t("&raquo;"),	0x00BB},
		{ _t("&rsaquo;"),	0x00BB},
		{ _t("&frac14;"),	0x00BC},
		{ _t("&frac12;"),	0x00BD},
		{ _t("&frac34;"),	0x00BE},
		{ _t("&iquest;"),	0x00BF},
		{ _t("&Agrave;"),	0x00C0},
		{ _t("&Aacute;"),	0x00C1},
		{ _t("&Acirc;"),	0x00C2},
		{ _t("&Atilde;"),	0x00C3},
		{ _t("&Auml;"),		0x00C4},
		{ _t("&Aring;"),	0x00C5},
		{ _t("&AElig;"),	0x00C6},
		{ _t("&Ccedil;"),	0x00C7},
		{ _t("&Egrave;"),	0x00C8},
		{ _t("&Eacute;"),	0x00C9},
		{ _t("&Ecirc;"),	0x00CA},
		{ _t("&Euml;"),		0x00CB},
		{ _t("&Igrave;"),	0x00CC},
		{ _t("&Iacute;"),	0x00CD},
		{ _t("&Icirc;"),	0x00CE},
		{ _t("&Iuml;"),		0x00CF},
		{ _t("&ETH;"),		0x00D0},
		{ _t("&Ntilde;"),	0x00D1},
		{ _t("&Ograve;"),	0x00D2},
		{ _t("&Oacute;"),	0x00D3},
		{ _t("&Ocirc;"),	0x00D4},
		{ _t("&Otilde;"),	0x00D5},
		{ _t("&Ouml;"),		0x00D6},
		{ _t("&times;"),	0x00D7},
		{ _t("&Oslash;"),	0x00D8},
		{ _t("&Ugrave;"),	0x00D9},
		{ _t("&Uacute;"),	0x00DA},
		{ _t("&Ucirc;"),	0x00DB},
		{ _t("&Uuml;"),		0x00DC},
		{ _t("&Yacute;"),	0x00DD},
		{ _t("&THORN;"),	0x00DE},
		{ _t("&szlig;"),	0x00DF},
		{ _t("&agrave;"),	0x00E0},
		{ _t("&aacute;"),	0x00E1},
		{ _t("&acirc;"),	0x00E2},
		{ _t("&atilde;"),	0x00E3},
		{ _t("&auml;"),		0x00E4},
		{ _t("&aring;"),	0x00E5},
		{ _t("&aelig;"),	0x00E6},
		{ _t("&ccedil;"),	0x00E7},
		{ _t("&egrave;"),	0x00E8},
		{ _t("&eacute;"),	0x00E9},
		{ _t("&ecirc;"),	0x00EA},
		{ _t("&euml;"),		0x00EB},
		{ _t("&igrave;"),	0x00EC},
		{ _t("&iacute;"),	0x00ED},
		{ _t("&icirc;"),	0x00EE},
		{ _t("&iuml;"),		0x00EF},
		{ _t("&eth;"),		0x00F0},
		{ _t("&ntilde;"),	0x00F1},
		{ _t("&ograve;"),	0x00F2},
		{ _t("&oacute;"),	0x00F3},
		{ _t("&ocirc;"),	0x00F4},
		{ _t("&otilde;"),	0x00F5},
		{ _t("&ouml;"),		0x00F6},
		{ _t("&divide;"),	0x00F7},
		{ _t("&oslash;"),	0x00F8},
		{ _t("&ugrave;"),	0x00F9},
		{ _t("&uacute;"),	0x00FA},
		{ _t("&ucirc;"),	0x00FB},
		{ _t("&uuml;"),		0x00FC},
		{ _t("&yacute;"),	0x00FD},
		{ _t("&thorn;"),	0x00FE},
		{ _t("&yuml;"),		0x00FF},
		{ _t("&OElig;"),	0x0152},
		{ _t("&oelig;"),	0x0153},
		{ _t("&Scaron;"),	0x0160},
		{ _t("&scaron;"),	0x0161},
		{ _t("&Yuml;"),		0x0178},
		{ _t("&fnof;"),		0x0192},
		{ _t("&circ;"),		0x02C6},
		{ _t("&tilde;"),	0x02DC},
		{ _t("&Alpha;"),	0x0391},
		{ _t("&Beta;"),		0x0392},
		{ _t("&Gamma;"),	0x0393},
		{ _t("&Delta;"),	0x0394},
		{ _t("&Epsilon;"),	0x0395},
		{ _t("&Zeta;"),		0x0396},
		{ _t("&Eta;"),		0x0397},
		{ _t("&Theta;"),	0x0398},
		{ _t("&Iota;"),		0x0399},
		{ _t("&Kappa;"),	0x039A},
		{ _t("&Lambda;"),	0x039B},
		{ _t("&Mu;"),		0x039C},
		{ _t("&Nu;"),		0x039D},
		{ _t("&Xi;"),		0x039E},
		{ _t("&Omicron;"),	0x039F},
		{ _t("&Pi;"),		0x03A0},
		{ _t("&Rho;"),		0x03A1},
		{ _t("&Sigma;"),	0x03A3},
		{ _t("&Tau;"),		0x03A4},
		{ _t("&Upsilon;"),	0x03A5},
		{ _t("&Phi;"),		0x03A6},
		{ _t("&Chi;"),		0x03A7},
		{ _t("&Psi;"),		0x03A8},
		{ _t("&Omega;"),	0x03A9},
		{ _t("&alpha;"),	0x03B1},
		{ _t("&beta;"),		0x03B2},
		{ _t("&gamma;"),	0x03B3},
		{ _t("&delta;"),	0x03B4},
		{ _t("&epsilon;"),	0x03B5},
		{ _t("&zeta;"),		0x03B6},
		{ _t("&eta;"),		0x03B7},
		{ _t("&theta;"),	0x03B8},
		{ _t("&iota;"),		0x03B9},
		{ _t("&kappa;"),	0x03BA},
		{ _t("&lambda;"),	0x03BB},
		{ _t("&mu;"),		0x03BC},
		{ _t("&nu;"),		0x03BD},
		{ _t("&xi;"),		0x03BE},
		{ _t("&omicron;"),	0x03BF},
		{ _t("&pi;"),		0x03C0},
		{ _t("&rho;"),		0x03C1},
		{ _t("&sigmaf;"),	0x03C2},
		{ _t("&sigma;"),	0x03C3},
		{ _t("&tau;"),		0x03C4},
		{ _t("&upsilon;"),	0x03C5},
		{ _t("&phi;"),		0x03C6},
		{ _t("&chi;"),		0x03C7},
		{ _t("&psi;"),		0x03C8},
		{ _t("&omega;"),	0x03C9},
		{ _t("&thetasym;"),	0x03D1},
		{ _t("&upsih;"),	0x03D2},
		{ _t("&piv;"),		0x03D6},
		{ _t("&ensp;"),		0x2002},
		{ _t("&emsp;"),		0x2003},
		{ _t("&thinsp;"),	0x2009},
		{ _t("&zwnj;"),		0x200C},
		{ _t("&zwj;"),		0x200D},
		{ _t("&lrm;"),		0x200E},
		{ _t("&rlm;"),		0x200F},
		{ _t("&ndash;"),	0x2013},
		{ _t("&mdash;"),	0x2014},
		{ _t("&lsquo;"),	0x2018},
		{ _t("&rsquo;"),	0x2019},
		{ _t("&sbquo;"),	0x201A},
		{ _t("&ldquo;"),	0x201C},
		{ _t("&rdquo;"),	0x201D},
		{ _t("&bdquo;"),	0x201E},
		{ _t("&dagger;"),	0x2020},
		{ _t("&Dagger;"),	0x2021},
		{ _t("&bull;"),		0x2022},
		{ _t("&hellip;"),	0x2026},
		{ _t("&permil;"),	0x2030},
		{ _t("&prime;"),	0x2032},
		{ _t("&Prime;"),	0x2033},
		{ _t("&lsaquo;"),	0x2039},
		{ _t("&rsaquo;"),	0x203A},
		{ _t("&oline;"),	0x203E},
		{ _t("&frasl;"),	0x2044},
		{ _t("&euro;"),		0x20AC},
		{ _t("&image;"),	0x2111},
		{ _t("&weierp;"),	0x2118},
		{ _t("&real;"),		0x211C},
		{ _t("&trade;"),	0x2122},
		{ _t("&alefsym;"),	0x2135},
		{ _t("&larr;"),		0x2190},
		{ _t("&uarr;"),		0x2191},
		{ _t("&rarr;"),		0x2192},
		{ _t("&darr;"),		0x2193},
		{ _t("&harr;"),		0x2194},
		{ _t("&crarr;"),	0x21B5},
		{ _t("&lArr;"),		0x21D0},
		{ _t("&uArr;"),		0x21D1},
		{ _t("&rArr;"),		0x21D2},
		{ _t("&dArr;"),		0x21D3},
		{ _t("&hArr;"),		0x21D4},
		{ _t("&forall;"),	0x2200},
		{ _t("&part;"),		0x2202},
		{ _t("&exist;"),	0x2203},
		{ _t("&empty;"),	0x2205},
		{ _t("&nabla;"),	0x2207},
		{ _t("&isin;"),		0x2208},
		{ _t("&notin;"),	0x2209},
		{ _t("&ni;"),		0x220B},
		{ _t("&prod;"),		0x220F},
		{ _t("&sum;"),		0x2211},
		{ _t("&minus;"),	0x2212},
		{ _t("&lowast;"),	0x2217},
		{ _t("&radic;"),	0x221A},
		{ _t("&prop;"),		0x221D},
		{ _t("&infin;"),	0x221E},
		{ _t("&ang;"),		0x2220},
		{ _t("&and;"),		0x2227},
		{ _t("&or;"),		0x2228},
		{ _t("&cap;"),		0x2229},
		{ _t("&cup;"),		0x222A},
		{ _t("&int;"),		0x222B},
		{ _t("&there4;"),	0x2234},
		{ _t("&sim;"),		0x223C},
		{ _t("&cong;"),		0x2245},
		{ _t("&asymp;"),	0x2248},
		{ _t("&ne;"),		0x2260},
		{ _t("&equiv;"),	0x2261},
		{ _t("&le;"),		0x2264},
		{ _t("&ge;"),		0x2265},
		{ _t("&sub;"),		0x2282},
		{ _t("&sup;"),		0x2283},
		{ _t("&nsub;"),		0x2284},
		{ _t("&sube;"),		0x2286},
		{ _t("&supe;"),		0x2287},
		{ _t("&oplus;"),	0x2295},
		{ _t("&otimes;"),	0x2297},
		{ _t("&perp;"),		0x22A5},
		{ _t("&sdot;"),		0x22C5},
		{ _t("&lceil;"),	0x2308},
		{ _t("&rceil;"),	0x2309},
		{ _t("&lfloor;"),	0x230A},
		{ _t("&rfloor;"),	0x230B},
		{ _t("&lang;"),		0x2329},
		{ _t("&rang;"),		0x232A},
		{ _t("&loz;"),		0x25CA},
		{ _t("&spades;"),	0x2660},
		{ _t("&clubs;"),	0x2663},
		{ _t("&hearts;"),	0x2665},
		{ _t("&diams;"),	0x2666},

		{_t(""),0}
	};

#if !defined(WIN32) && !defined(WINCE)

	int wchar2utf8(wchar_t chr, char* out_buf, size_t out_len)
	{
		size_t ret = out_len;

		char* in_buf = (char*) &chr;
		size_t in_len = sizeof(chr);

		iconv_t cvt = iconv_open("UTF-8", "WCHAR_T");
		size_t cnt = iconv(cvt, &in_buf, &in_len, &out_buf, &out_len);
		if(cnt < 0)
		{
			ret = 0;
		} else
		{
			ret -= out_len;
		}
		return ret;
	}

#endif

	litehtml::tchar_t scanner::resolve_entity( const tchar_t* buf, int buf_size )
	{
		wchar_t wres = 0;
		if(buf[0] == '#')
		{
			if(buf[1] == 'x' || buf[1] == 'X')
			{
				tchar_t* end = 0;
				wres = (wchar_t) t_strtol(buf + 2, &end, 16);
			} else
			{
				wres = (wchar_t) t_atoi(buf + 1);
			}
		} else
		{
			tstring str = _t("&");
			str.append(buf, buf_size);
			str += _t(";");
			for(int i = 0; m_HTMLCodes[i].szCode[0]; i++)
			{
				if(!t_strcasecmp(m_HTMLCodes[i].szCode, str.c_str()))
				{
					wres = m_HTMLCodes[i].Code;
					break;
				}
			}
		}

#if defined(WIN32) || defined(WINCE)
		return wres;
#else
		if(!wres) return 0;
		tchar_t mb_str[20];
		int cnt = wchar2utf8(wres, mb_str, sizeof(mb_str));
		if(cnt > 0 && cnt <= 20)
		{
			for(int i = 0; i < cnt - 1; i++)
			{
				append_value(mb_str[i]);
			}
			return mb_str[cnt - 1];
		}
		return 0;
#endif
	}

	scanner::token_type scanner::scan_raw_body()
	{
		if(got_tail)
		{
			c_scan = &scanner::scan_body;
			got_tail = false;
			return TT_TAG_END;
		}
		for(value_length = 0; value_length < (MAX_TOKEN_SIZE - 1); ++value_length)
		{
			tchar_t c = get_char();
			if( c == 0) return TT_EOF;
			value[value_length] = c;

			if(value_length >= 8 && !t_strncasecmp(value + value_length - 8, _t("</script>"), 9))
			{
				got_tail = true;
				value_length -= 8;
				break;
			}
		}
		value[value_length] = 0;
		return TT_DATA;
	}

}
 
