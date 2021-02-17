// Header containing the RAII types (to safely manage WINAPI HANDLEs and HMODULEs)

#pragma once
#include "common.h"

namespace RAII
{
	class Handle
	{
	public:
		Handle(HANDLE inputHandle);
		~Handle();
		HANDLE GetHandle();

	private:
		HANDLE _internalHandle;
	};

	class Hmodule
	{
	public:
		Hmodule(HMODULE inputHmodule);
		~Hmodule();
		HMODULE GetHmodule();

	private:
		HMODULE _internalHmodule;
	};
};