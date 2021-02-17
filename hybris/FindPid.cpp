/*
 * This file contains the routine used to iterate through a snapshot of
 * the system and find the PID of a process given its image name
*/

#include "common.h"

DWORD FindPid(_In_ std::wstring imageName)
{

	// create snapshot of processes using RAII classes
	RAII::Handle snapshot(
		CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL)
	);

	if (!snapshot.GetHandle())
	{
		Error(::GetLastError());
		return ERROR_FILE_NOT_FOUND;
	}

	PROCESSENTRY32W processEntry;
	processEntry.dwSize = sizeof(PROCESSENTRY32W);
	auto status = Process32FirstW(snapshot.GetHandle(), &processEntry); // start enumerating from the first process
	if (!status)
	{
		Error(::GetLastError());
		return ERROR_FILE_NOT_FOUND;
	}

	do
	{
		std::wstring processImage = processEntry.szExeFile;
		std::transform(processImage.begin(), processImage.end(), processImage.begin(), towlower);
		if (processImage == imageName)
		{
			std::wcout << L"[+] Found process " << processEntry.szExeFile << " with PID " << processEntry.th32ProcessID << std::endl; // when lsass is found return its PID to the caller
			return processEntry.th32ProcessID;
		}
	} while (Process32NextW(snapshot.GetHandle(), &processEntry));

	return ERROR_FILE_NOT_FOUND;
}