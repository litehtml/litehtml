#pragma once

#include "../containers/cairo/cairo_container.h"
#include "tordexhttp.h"

class CHTMLViewWnd;

class web_page :	public cairo_container
{
	CHTMLViewWnd*				m_parent;
	LONG						m_refCount;
public:
	tordex::http				m_http;
	std::wstring				m_url;
	litehtml::document::ptr		m_doc;
	std::wstring				m_caption;
	std::wstring				m_cursor;
	std::wstring				m_base_path;
	HANDLE						m_hWaitDownload;
	std::wstring				m_waited_file;
	std::wstring				m_hash;
public:
	web_page(CHTMLViewWnd* parent);
	virtual ~web_page();

	void load(LPCWSTR url);
	void on_document_loaded(LPCWSTR file, LPCWSTR encoding);
	void on_image_loaded(LPCWSTR file, LPCWSTR url, bool redraw_only);
	void on_document_error();
	void on_waited_finished(DWORD dwError, LPCWSTR file);
	void add_ref();
	void release();
	void get_url(std::wstring& url);

	// litehtml::document_container members
	virtual	void		set_caption(const wchar_t* caption);
	virtual	void		set_base_url(const wchar_t* base_url);
	virtual	void		link(litehtml::document* doc, litehtml::element::ptr el);
	virtual void		import_css(std::wstring& text, const std::wstring& url, std::wstring& baseurl);
	virtual	void		on_anchor_click(const wchar_t* url, litehtml::element::ptr el);
	virtual	void		set_cursor(const wchar_t* cursor);

	virtual void		make_url(LPCWSTR url, LPCWSTR basepath, std::wstring& out);
	virtual CTxDIB*		get_image(LPCWSTR url, bool redraw_on_ready);
	virtual void		get_client_rect(litehtml::position& client);
private:
	LPWSTR	load_text_file( LPCWSTR path, bool is_html, LPCWSTR defEncoding = L"UTF-8");
	BOOL	download_and_wait(LPCWSTR url);
};

enum web_file_type
{
	web_file_document,
	web_file_image_redraw,
	web_file_image_rerender,
	web_file_waited,
};

class web_file : public tordex::http_request
{
	WCHAR			m_file[MAX_PATH];
	web_page*		m_page;
	web_file_type	m_type;
	HANDLE			m_hFile;
	LPVOID			m_data;
public:
	web_file(web_page* page, web_file_type type, LPVOID data = NULL);
	virtual ~web_file();

	virtual void OnFinish(DWORD dwError, LPCWSTR errMsg);
	virtual void OnData(LPCBYTE data, DWORD len, ULONG64 downloaded, ULONG64 total);
	virtual void OnHeadersReady(HINTERNET hRequest);

};
