#include "wow.h"

#include "cassert_my.h"

#include "my_windows.h"


#if defined _M_IX86
bool is_wow64()
{
	auto const IsWow64Process_proc = GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "IsWow64Process");
	if(!IsWow64Process_proc)
	{
		return false;
	}
	auto const IsWow64Process_fn = reinterpret_cast<decltype(&IsWow64Process)>(IsWow64Process_proc);
	BOOL iswow64;
	BOOL const queried = IsWow64Process_fn(GetCurrentProcess(), &iswow64);
	assert(queried != 0);
	if(iswow64 != FALSE)
	{
		return true;
	}
	else
	{
		return false;
	}
}
#elif defined _M_X64
#else
#error Unknown architecture.
#endif
