#pragma once

class CRemotedFile
{
	WCHAR	m_tmpFile[MAX_PATH];
	HANDLE	m_hFile;
public:
	CRemotedFile();
	~CRemotedFile();

	HANDLE Open(LPCWSTR url);
};

LPWSTR load_text_file( LPCWSTR path );
