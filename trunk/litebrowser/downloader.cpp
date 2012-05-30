#include "globals.h"
#include "downloader.h"
#include <strsafe.h>

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
	LPVOID		szTemp	= malloc(10240);
	HINTERNET  hConnect;

	hConnect = InternetOpenUrl( hOpen, szUrl, szHead, lstrlen (szHead), INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_RELOAD, 0);

	if (!hConnect)
	{
	   return FALSE;
	}

	HANDLE hFile = CreateFile(szFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	
	if(hFile == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	do
	{
		if( !InternetReadFile (hConnect, szTemp, 10240,  &dwSize) )
		{
			CloseHandle(hFile);
			DeleteFile(szFileName);
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

LPWSTR load_text_file( LPCWSTR path )
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

		strW = new WCHAR[cbRead + 1];
		MultiByteToWideChar(CP_UTF8, 0, str, cbRead + 1, strW, cbRead + 1);

		free(str);
	}

	return strW;
}
