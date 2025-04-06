#ifndef LH_EL_CDATA_H
#define LH_EL_CDATA_H

#include "element.h"

namespace litehtml
{
	class el_cdata : public element
	{
		string	m_text;
	public:
		explicit el_cdata(const std::shared_ptr<document>& doc);

		void get_text(string& text) override;
		void set_data(const char* data) override;
	};
}

#endif  // LH_EL_CDATA_H
