#include "css_calc.h"
#include "css_length.h"
#include "css_tokenizer.h"
#include <memory>

bool litehtml::css_calc::from_token(const css_token& token, int options)
{
	if(token.type == CV_FUNCTION && token.name == "calc")
	{
		m_root = std::make_unique<css_calc_node>(css_calc_node_type::root);

		for(const auto& tok : token.value)
		{
			switch(tok.type)
			{
			case PERCENTAGE:
			case NUMBER:
			case DIMENSION:
				{
					css_length len;
					if(!len.from_token(tok, options))
						return false;
					m_root
				}
				break;
            }
        }
	}
	return false;
}
