#include "globals.h"
#include "downloader.h"
#include <strsafe.h>
#include <shlwapi.h>
#include <Mlang.h>

#pragma comment(lib, "wininet.lib")

CRemotedFile::CRemotedFile()
{
	m_tmpFile[0]	= 0;
	m_hFile			= NULL;
}

CRemotedFile::~CRemotedFile()
{
	if(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hFile);
		DeleteFile(m_tmpFile);
	}
}

BOOL GetFile (HINTERNET IN hOpen, LPCWSTR szUrl, LPCWSTR szFileName)
{
	DWORD		dwSize;
	WCHAR		szHead[] = L"Accept: */*\r\n\r\n";
	HINTERNET  hConnect;

	hConnect = InternetOpenUrl( hOpen, szUrl, szHead, lstrlen (szHead), /*INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_RELOAD*/0, 0);

	if (!hConnect)
	{
	   return FALSE;
	}

	HANDLE hFile = CreateFile(szFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	
	if(hFile == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	LPVOID		szTemp	= malloc(10240);

	do
	{
		if( !InternetReadFile (hConnect, szTemp, 10240,  &dwSize) )
		{
			CloseHandle(hFile);
			DeleteFile(szFileName);
			free(szTemp);
			return FALSE;
		}
		if (!dwSize)
			break;  // Condition of dwSize=0 indicate EOF. Stop.
		else
		{
			DWORD written = 0;
			WriteFile(hFile, szTemp, dwSize, &written, NULL);
		}
	} while (TRUE);

	InternetCloseHandle(hConnect);

	CloseHandle(hFile);
	free(szTemp);

	return TRUE;
}

HANDLE CRemotedFile::Open( LPCWSTR url )
{
	HINTERNET hInternet = InternetOpen(L"Lite HTML Browser", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);

	WCHAR tmpPath[MAX_PATH];
	GetTempPath(MAX_PATH, tmpPath);
	GetTempFileName(tmpPath, L"lhtml", 0, m_tmpFile);
	if(GetFile(hInternet, url, m_tmpFile))
	{
		InternetCloseHandle(hInternet);
		m_hFile = CreateFile(m_tmpFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		return m_hFile;
	}
	InternetCloseHandle(hInternet);
	return INVALID_HANDLE_VALUE;
}

LPWSTR load_text_file( LPCWSTR path, bool is_html )
{
	LPWSTR strW = NULL;

	CRemotedFile rf;

	HANDLE fl = rf.Open(path);
	if(fl != INVALID_HANDLE_VALUE)
	{
		DWORD size = GetFileSize(fl, NULL);
		LPSTR str = (LPSTR) malloc(size + 1);

		DWORD cbRead = 0;
		ReadFile(fl, str, size, &cbRead, NULL);
		str[cbRead] = 0;

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

				if(ml->ConvertString(&dwMode, charset_src.uiInternetEncoding, charset_dst.uiInternetEncoding, (LPBYTE) str, NULL, (LPBYTE) dst, &szDst) == S_OK)
				{
					dst[szDst] = 0;
					cbRead = szDst;
					delete str;
					str = dst;
				} else
				{
					delete dst;
				}
			}
		}

		if(!strW)
		{
			strW = new WCHAR[cbRead + 1];
			MultiByteToWideChar(CP_UTF8, 0, str, cbRead, strW, cbRead + 1);
		}

		free(str);
	}

	return strW;
}
