#pragma once

class CTxThread
{
	HANDLE	m_hThread;
	HANDLE	m_hStop;
	DWORD	m_trdID;

	static DWORD WINAPI sThreadProc(LPVOID lpParameter);
public:
	CTxThread(void);
	virtual ~CTxThread(void);
	virtual DWORD ThreadProc() = 0;

	DWORD	getID() { return m_trdID; }
	void	Run();
	void	Stop();
	BOOL	WaitForStop(DWORD ms);
	int		WaitForStop2(HANDLE hWait, DWORD ms);
	int		WaitForStop3(LPHANDLE hWait, int count, DWORD ms);
	void	postMessage(UINT Msg, WPARAM wParam, LPARAM lParam);
};
