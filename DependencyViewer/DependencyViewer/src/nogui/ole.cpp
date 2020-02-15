#include "ole.h"

#include "cassert_my.h"

#include "my_windows.h"

#include <ole2.h>


ole::ole()
{
	HRESULT const initialized = OleInitialize(nullptr);
	assert(initialized == S_OK || initialized == S_FALSE && initialized != OLE_E_WRONGCOMPOBJ && initialized != RPC_E_CHANGED_MODE);
}

ole::~ole()
{
	OleUninitialize();
}
