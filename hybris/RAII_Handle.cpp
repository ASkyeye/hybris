#include "raii.h"

namespace RAII
{
	Handle::Handle(HANDLE inputHandle)
	{
		_internalHandle = inputHandle;
	}

	Handle::~Handle()
	{
		::CloseHandle(_internalHandle);
	}

	HANDLE Handle::GetHandle()
	{
		return _internalHandle;
	}
}