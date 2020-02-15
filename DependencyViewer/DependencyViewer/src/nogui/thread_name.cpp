#include "thread_name.h"

#include "cassert_my.h"

#include "my_windows.h"

#include <processthreadsapi.h>


#pragma pack(push, 8)
struct tagTHREADNAME_INFO
{
	DWORD dwType;
	LPCSTR szName;
	DWORD dwThreadID;
	DWORD dwFlags;
};
typedef struct tagTHREADNAME_INFO THREADNAME_INFO;
#pragma pack(pop)


static constexpr DWORD const MS_VC_EXCEPTION = 0x406D1388;


void old_style(char const* const name);
void new_style(wchar_t const* const name);


void name_current_thread(char const* const name_a, wchar_t const* const name_w)
{
	old_style(name_a);
	new_style(name_w);
}


void old_style(char const* const name)
{
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = name;
	info.dwThreadID = static_cast<DWORD>(-1);
	info.dwFlags = 0;
	__try
	{
		RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), reinterpret_cast<ULONG_PTR*>(&info));
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
	}
}

void new_style(wchar_t const* const name)
{
	auto const SetThreadDescription_proc = GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "SetThreadDescription");
	if(!SetThreadDescription_proc) { return; }
	auto const SetThreadDescription_fn = reinterpret_cast<decltype(&SetThreadDescription)>(SetThreadDescription_proc);
	HRESULT const hr = SetThreadDescription_fn(GetCurrentThread(), name);
	assert(!FAILED(hr));
}
