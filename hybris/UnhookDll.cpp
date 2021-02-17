#include "common.h"

bool UnhookDll(_In_ std::string dllPath)
{
	// get handles to the current process, the DLL on disk, the memory mapping for the DLL on disk and the already loaded DLL to unhook
	MODULEINFO mi = {};
	RAII::Handle hCurrentProcess(::GetCurrentProcess());
	RAII::Handle hDllFile(::CreateFileA(dllPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL));
	RAII::Handle hDllMapping(::CreateFileMappingW(hDllFile.GetHandle(), NULL, PAGE_READONLY | SEC_IMAGE, 0, 0, NULL));
	RAII::Hmodule hDllModule(::GetModuleHandleA(dllPath.c_str()));

	// exit if unable to get the required handles
	if (
		hCurrentProcess.GetHandle() == NULL ||
		hDllFile.GetHandle() == NULL ||
		hDllMapping.GetHandle() == NULL ||
		hDllModule.GetHmodule() == NULL
		)
	{
		std::cout << "[-] Failed to get a handle to provided DLL or internal memory." << std::endl;
		::Error(::GetLastError());
		return false;
	}

	// try to get the hooked module information IOT get the header information, exit if it fails
	bool success = ::GetModuleInformation(hCurrentProcess.GetHandle(), hDllModule.GetHmodule(), &mi, sizeof(mi));
	if (!success)
	{
		std::cout << "[-] Could not get " << dllPath << " module information." << std::endl;
		::Error(::GetLastError());
		return false;
	}

	// extract the base address of the DLL, the address of the DOS and NT headers from the MODULEINFO structure
	LPVOID hookedDllBase = (LPVOID)mi.lpBaseOfDll;
	PIMAGE_DOS_HEADER hookedDosHeader = (PIMAGE_DOS_HEADER)hookedDllBase;
	PIMAGE_NT_HEADERS hookedNtHeader = (PIMAGE_NT_HEADERS)((DWORD_PTR)hookedDllBase + hookedDosHeader->e_lfanew);

	// re-map the original DLL in a new memory area and save the address
	LPVOID dllMappingAddress = ::MapViewOfFile(hDllMapping.GetHandle(), FILE_MAP_READ, 0, 0, 0);

	// iterate on all the sections in the header
	for (WORD i = 0; i < hookedNtHeader->FileHeader.NumberOfSections; i++) {
		PIMAGE_SECTION_HEADER hookedSectionHeader = (PIMAGE_SECTION_HEADER)((DWORD_PTR)IMAGE_FIRST_SECTION(hookedNtHeader) + ((DWORD_PTR)IMAGE_SIZEOF_SECTION_HEADER * i));

		// when .text is found, try to overwrite the hooked .text with the original .text 
		if (!strcmp((char*)hookedSectionHeader->Name, (char*)".text")) {
			DWORD oldProtection = 0;
			success = VirtualProtect((LPVOID)((DWORD_PTR)hookedDllBase + (DWORD_PTR)hookedSectionHeader->VirtualAddress), hookedSectionHeader->Misc.VirtualSize, PAGE_EXECUTE_READWRITE, &oldProtection);
			if (!success)
			{
				std::cout << "[-] Could not change the protection on the hooked DLL memory page." << std::endl;
				::Error(::GetLastError());
				return false;
			}

			memcpy((LPVOID)((DWORD_PTR)hookedDllBase + (DWORD_PTR)hookedSectionHeader->VirtualAddress), (LPVOID)((DWORD_PTR)dllMappingAddress + (DWORD_PTR)hookedSectionHeader->VirtualAddress), hookedSectionHeader->Misc.VirtualSize);
			success = VirtualProtect((LPVOID)((DWORD_PTR)hookedDllBase + (DWORD_PTR)hookedSectionHeader->VirtualAddress), hookedSectionHeader->Misc.VirtualSize, oldProtection, &oldProtection);
			if (!success)
			{
				std::cout << "[-] Could not restore the protection on the hooked DLL memory page." << std::endl;
				::Error(::GetLastError());
				return false;
			}
		}
	}

	return true;
}