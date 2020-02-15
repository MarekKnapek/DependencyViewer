#include "com_dlg.h"

#include "../nogui/cassert_my.h"
#include "../nogui/smart_library.h"


static smart_library g_comdlg32 = nullptr;
static void* g_fn_GetOpenFileNameW = nullptr;


void com_dlg::load()
{
	assert((SetLastError(0), true));
	assert(GetModuleHandleW(L"comdlg32.dll") == nullptr);
	assert(GetLastError() == ERROR_MOD_NOT_FOUND);

	assert(!g_comdlg32);
	g_comdlg32 = load_library(L"comdlg32.dll");
	assert(g_comdlg32);

	auto const proc_GetOpenFileNameW = GetProcAddress(g_comdlg32.get(), "GetOpenFileNameW");
	assert(proc_GetOpenFileNameW != nullptr);
	g_fn_GetOpenFileNameW = proc_GetOpenFileNameW;
}

void com_dlg::unload()
{
	assert(g_comdlg32);
	g_comdlg32.reset();
	assert(!g_comdlg32);
}

BOOL com_dlg::GetOpenFileNameW(LPOPENFILENAMEW ofn)
{
	assert(g_fn_GetOpenFileNameW);
	auto const fn = reinterpret_cast<decltype(&::GetOpenFileNameW)>(g_fn_GetOpenFileNameW);
	return fn(ofn);
}
