#include "com.h"

#include "cassert_my.h"

#include "windows_my.h"

#include <objbase.h>


com::com()
{
	HRESULT const initialized = CoInitialize(nullptr);
	assert(initialized == S_OK || initialized == S_FALSE && initialized != RPC_E_CHANGED_MODE);
}

com::~com()
{
	CoUninitialize();
}
