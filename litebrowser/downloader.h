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