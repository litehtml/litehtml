#pragma once

class web_history
{
	litehtml::string_vector				m_items;
	litehtml::string_vector::size_type	m_current_item;
public:
	web_history();
	virtual ~web_history();

	void url_opened(const std::wstring& url);
	bool back(std::wstring& url);
	bool forward(std::wstring& url);
};