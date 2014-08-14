#include "globals.h"
#include "web_page.h"
#include "HtmlViewWnd.h"


web_page::web_page(CHTMLViewWnd* parent)
{
	m_refCount		= 1;
	m_parent		= parent;
	m_http.open(L"litebrowser/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS);
}

web_page::~web_page()
{
	m_http.stop();
}

void web_page::set_caption( const wchar_t* caption )
{
	m_caption = caption;
}

void web_page::set_base_url( const wchar_t* base_url )
{
	if(base_url)
	{
		if(PathIsRelative(base_url) && !PathIsURL(base_url))
		{
			make_url(base_url, m_url.c_str(), m_base_path);
		} else
		{
			m_base_path = base_url;
		}
	} else
	{
		m_base_path = m_url;
	}
}

void web_page::make_url( LPCWSTR url, LPCWSTR basepath, std::wstring& out )
{
	if(PathIsRelative(url) && !PathIsURL(url))
	{
		WCHAR abs_url[512];
		DWORD dl = 512;
		if(basepath && basepath[0])
		{
			UrlCombine(basepath, url, abs_url, &dl, 0);
		} else
		{
			UrlCombine(m_base_path.c_str(), url, abs_url, &dl, 0);
		}
		out = abs_url;
	} else
	{
		if(PathIsURL(url))
		{
			out = url;
		} else
		{
			WCHAR abs_url[512];
			DWORD dl = 512;
			UrlCreateFromPath(url, abs_url, &dl, 0);
			out = abs_url;
		}
	}
	if(out.substr(0, 8) == L"file:///")
	{
		out.erase(5, 1);
	}
	if(out.substr(0, 7) == L"file://")
	{
		out.erase(0, 7);
	}
}

void web_page::link( litehtml::document* doc, litehtml::element::ptr el )
{
/*
	const wchar_t* rel = el->get_attr(L"rel");
	if(rel && !wcscmp(rel, L"stylesheet"))
	{
		const wchar_t* media = el->get_attr(L"media", L"screen");
		if(media && (wcsstr(media, L"screen") || wcsstr(media, L"all")))
		{
			const wchar_t* href = el->get_attr(L"href");
			if(href && href[0])
			{
				std::wstring url;
				make_url(href, NULL, url);
				if(download_and_wait(url.c_str()))
				{
					LPWSTR css = load_text_file(m_waited_file.c_str(), L"UTF-8");
					if(css)
					{
						doc->add_stylesheet(css, url.c_str());
						delete css;
					}
				}
			}
		}
	}
*/
}

void web_page::import_css( std::wstring& text, const std::wstring& url, std::wstring& baseurl )
{
	std::wstring css_url;
	make_url(url.c_str(), baseurl.c_str(), css_url);

	if(download_and_wait(css_url.c_str()))
	{
		LPWSTR css = load_text_file(m_waited_file.c_str(), L"UTF-8");
		if(css)
		{
			baseurl = css_url;
			text = css;
			delete css;
		}
	}
}

void web_page::on_anchor_click( const wchar_t* url, litehtml::element::ptr el )
{
	std::wstring anchor;
	make_url(url, NULL, anchor);
	m_parent->open(anchor.c_str());
}

void web_page::set_cursor( const wchar_t* cursor )
{
	m_cursor = cursor;
}

CTxDIB* web_page::get_image( LPCWSTR url, bool redraw_on_ready )
{
	CTxDIB* img = NULL;
	if(PathIsURL(url))
	{
		if(redraw_on_ready)
		{
			m_http.download_file( url, new web_file(this, web_file_image_redraw) );
		} else
		{
			m_http.download_file( url, new web_file(this, web_file_image_rerender) );
		}
	} else
	{
		img = new CTxDIB;
		if(!img->load(url))
		{
			delete img;
			img = NULL;
		}
	}
	return img;
}

void web_page::load( LPCWSTR url )
{
	m_url		= url;
	m_base_path	= m_url;
	if(PathIsURL(url))
	{
		m_http.download_file( url, new web_file(this, web_file_document) );
	} else
	{
		on_document_loaded(url, L"UTF-8");
	}
}

void web_page::on_document_loaded( LPCWSTR file, LPCWSTR encoding )
{
	LPWSTR html_text = load_text_file(file, true, encoding);

	if(!html_text)
	{
		LPCWSTR txt = L"<h1>Something Wrong</h1>";
		html_text = new WCHAR[lstrlen(txt) + 1];
		lstrcpy(html_text, txt);
	}

	m_doc = litehtml::document::createFromString(html_text, this, m_parent->get_html_context());
	delete html_text;

	PostMessage(m_parent->wnd(), WM_PAGE_LOADED, 0, 0);
}

LPWSTR web_page::load_text_file( LPCWSTR path, bool is_html, LPCWSTR defEncoding )
{
	CoInitialize(NULL);

	LPWSTR strW = NULL;

	HANDLE fl = CreateFile(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(fl != INVALID_HANDLE_VALUE)
	{
		DWORD size = GetFileSize(fl, NULL);
		LPSTR str = (LPSTR) malloc(size + 1);

		DWORD cbRead = 0;
		ReadFile(fl, str, size, &cbRead, NULL);
		str[cbRead] = 0;
		CloseHandle(fl);

		int bom = 0;
		if(str[0] == '\xEF' && str[1] == '\xBB' && str[2] == '\xBF')
		{
			bom = 3;
		}

		if(is_html)
		{
			std::wstring encoding;
			char* begin = StrStrIA(str, "<meta");
			while(begin && encoding.empty())
			{
				char* end = StrStrIA(begin, ">");
				char* s1 = StrStrIA(begin, "Content-Type");
				if(s1 && s1 < end)
				{
					s1 = StrStrIA(begin, "charset");
					if(s1)
					{
						s1 += strlen("charset");
						while(!isalnum(s1[0]) && s1 < end)
						{
							s1++;
						}
						while((isalnum(s1[0]) || s1[0] == '-') && s1 < end)
						{
							encoding += s1[0];
							s1++;
						}
					}
				}
				if(encoding.empty())
				{
					begin = StrStrIA(begin + strlen("<meta"), "<meta");
				}
			}

			if(encoding.empty() && defEncoding)
			{
				encoding = defEncoding;
			}

			if(!encoding.empty())
			{
				IMultiLanguage* ml = NULL;
				HRESULT hr = CoCreateInstance(CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER, IID_IMultiLanguage, (LPVOID*) &ml);	

				MIMECSETINFO charset_src = {0};
				MIMECSETINFO charset_dst = {0};

				BSTR bstrCharSet = SysAllocString(encoding.c_str());
				ml->GetCharsetInfo(bstrCharSet, &charset_src);
				SysFreeString(bstrCharSet);

				bstrCharSet = SysAllocString(L"utf-8");
				ml->GetCharsetInfo(bstrCharSet, &charset_dst);
				SysFreeString(bstrCharSet);

				DWORD dwMode = 0;
				UINT  szDst = (UINT) strlen(str) * 4;
				LPSTR dst = new char[szDst];

				if(ml->ConvertString(&dwMode, charset_src.uiInternetEncoding, charset_dst.uiInternetEncoding, (LPBYTE) str + bom, NULL, (LPBYTE) dst, &szDst) == S_OK)
				{
					dst[szDst] = 0;
					cbRead = szDst;
					delete str;
					str = dst;
					bom = 0;
				} else
				{
					delete dst;
				}
			}
		}

		if(!strW)
		{
			strW = new WCHAR[cbRead + 1];
			MultiByteToWideChar(CP_UTF8, 0, str + bom, -1, strW, cbRead + 1);
		}

		free(str);
	}

	CoUninitialize();

	return strW;
}

void web_page::on_document_error()
{
	//delete this;
}

void web_page::on_image_loaded( LPCWSTR file, LPCWSTR url, bool redraw_only )
{
	CTxDIB* img = new CTxDIB;
	if(img->load(file))
	{
		cairo_container::add_image(litehtml::tstring(url), img);
		if(m_doc)
		{
			PostMessage(m_parent->wnd(), WM_IMAGE_LOADED, (WPARAM) (redraw_only ? 1 : 0), 0);
		}
	} else
	{
		delete img;
	}
}

BOOL web_page::download_and_wait( LPCWSTR url )
{
	if(PathIsURL(url))
	{
		m_waited_file = L"";
		m_hWaitDownload = CreateEvent(NULL, TRUE, FALSE, NULL);
		if(m_http.download_file( url, new web_file(this, web_file_waited) ))
		{
			WaitForSingleObject(m_hWaitDownload, INFINITE);
			CloseHandle(m_hWaitDownload);
			if(m_waited_file.empty())
			{
				return FALSE;
			}
			return TRUE;
		}
	} else
	{
		m_waited_file = url;
		return TRUE;
	}
	return FALSE;
}

void web_page::on_waited_finished( DWORD dwError, LPCWSTR file )
{
	if(dwError)
	{
		m_waited_file = L"";
	} else
	{
		m_waited_file = file;
	}
	SetEvent(m_hWaitDownload);
}

void web_page::get_client_rect( litehtml::position& client )
{
	m_parent->get_client_rect(client);
}

void web_page::add_ref()
{
	InterlockedIncrement(&m_refCount);
}

void web_page::release()
{
	LONG lRefCount;
	lRefCount = InterlockedDecrement(&m_refCount);
	if (lRefCount == 0)
	{
		delete this;
	}
}

void web_page::get_url( std::wstring& url )
{
	url = m_url;
	if(!m_hash.empty())
	{
		url += L"#";
		url += m_hash;
	}
}

//////////////////////////////////////////////////////////////////////////

web_file::web_file( web_page* page, web_file_type type, LPVOID data )
{
	m_data	= data;
	m_page	= page;
	m_type	= type;
	WCHAR path[MAX_PATH];
	GetTempPath(MAX_PATH, path);
	GetTempFileName(path, L"lbr", 0, m_file);
	m_hFile = CreateFile(m_file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	m_page->add_ref();
}

web_file::~web_file()
{
	if(m_hFile)
	{
		CloseHandle(m_hFile);
		m_hFile = NULL;
	}
	if(m_type != web_file_waited)
	{
		DeleteFile(m_file);
	}
	if(m_page)
	{
		m_page->release();
	}
}

void web_file::OnFinish( DWORD dwError, LPCWSTR errMsg )
{
	if(m_hFile)
	{
		CloseHandle(m_hFile);
		m_hFile = NULL;
	}
	if(dwError)
	{
		std::wstring fileName = m_file;
		switch(m_type)
		{
		case web_file_document:
			m_page->on_document_error();
			break;
		case web_file_waited:
			m_page->on_waited_finished(dwError, m_file);
			break;
		}
	} else
	{
		switch(m_type)
		{
		case web_file_document:
			m_page->on_document_loaded(m_file, L"UTF-8");
			break;
		case web_file_image_redraw:
			m_page->on_image_loaded(m_file, m_url.c_str(), true);
			break;
		case web_file_image_rerender:
			m_page->on_image_loaded(m_file, m_url.c_str(), false);
			break;
		case web_file_waited:
			m_page->on_waited_finished(dwError, m_file);
			break;
		}		
	}
}

void web_file::OnData( LPCBYTE data, DWORD len, ULONG64 downloaded, ULONG64 total )
{
	if(m_hFile)
	{
		DWORD cbWritten = 0;
		WriteFile(m_hFile, data, len, &cbWritten, NULL);
	}
}

void web_file::OnHeadersReady( HINTERNET hRequest )
{

}
