#include "globals.h"
#include "TxThread.h"

CTxThread::CTxThread(void)
{
	m_hThread	= NULL;
	m_hStop		= CreateEvent(NULL, FALSE, FALSE, NULL);
	m_trdID		= NULL;
}

CTxThread::~CTxThread(void)
{
	Stop();
	if(m_hThread) CloseHandle(m_hThread);
	if(m_hStop)   CloseHandle(m_hStop);
}

void CTxThread::Run()
{
	Stop();
	m_hThread = CreateThread(NULL, 0, sThreadProc, (LPVOID) this, 0, &m_trdID);
}

void CTxThread::Stop()
{
	if(m_hThread)
	{
		SetEvent(m_hStop);
		WaitForSingleObject(m_hThread, INFINITE);
		CloseHandle(m_hThread);
		m_hThread = NULL;
	}
}

DWORD WINAPI CTxThread::sThreadProc( LPVOID lpParameter )
{
	CTxThread* pThis = (CTxThread*) lpParameter;
	return pThis->ThreadProc();
}

BOOL CTxThread::WaitForStop( DWORD ms )
{
	if(WaitForSingleObject(m_hStop, ms) != WAIT_TIMEOUT)
	{
		return TRUE;
	}
	return FALSE;
}

int CTxThread::WaitForStop2( HANDLE hWait, DWORD ms )
{
	HANDLE arr[2];
	arr[0]	= m_hStop;
	arr[1]	= hWait;
	return WaitForMultipleObjects(2, arr, FALSE, ms);
}

int CTxThread::WaitForStop3( LPHANDLE hWait, int count, DWORD ms )
{
	LPHANDLE arr = new HANDLE[count + 1];
	arr[0]	= m_hStop;
	for(int i=0; i < count; i++)
	{
		arr[i + 1] = hWait[i];
	}
	DWORD ret = WaitForMultipleObjects(count + 1, arr, FALSE, ms);
	delete [] arr;
	return ret;
}

void CTxThread::postMessage( UINT Msg, WPARAM wParam, LPARAM lParam )
{
	if(m_hThread)
	{
		PostThreadMessage(m_trdID, Msg, wParam, lParam);
	}
}