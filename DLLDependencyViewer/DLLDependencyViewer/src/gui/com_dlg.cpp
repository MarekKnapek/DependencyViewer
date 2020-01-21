#include "com_dlg.h"

#include "../nogui/smart_library.h"

#include <cassert>


static smart_library g_comdlg32 = nullptr;
static void* g_fn_GetOpenFileNameW = nullptr;


void com_dlg::load()
{
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
