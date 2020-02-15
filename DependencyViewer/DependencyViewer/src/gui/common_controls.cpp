#include "common_controls.h"

#include "../nogui/cassert_my.h"
#include "../nogui/smart_library.h"


static smart_library g_comctl32 = nullptr;
static void* g_fn_InitCommonControls = nullptr;
static void* g_fn_ImageList_LoadImageW = nullptr;
static void* g_fn_DrawStatusTextW = nullptr;


void common_controls::load()
{
	assert((SetLastError(0), true));
	assert(GetModuleHandleW(L"comctl32.dll") == nullptr);
	assert(GetLastError() == ERROR_MOD_NOT_FOUND);

	assert(!g_comctl32);
	g_comctl32 = load_library(L"comctl32.dll");
	assert(g_comctl32);

	auto const proc_InitCommonControls = GetProcAddress(g_comctl32.get(), "InitCommonControls");
	assert(proc_InitCommonControls != nullptr);
	g_fn_InitCommonControls = proc_InitCommonControls;

	auto const proc_ImageList_LoadImageW = GetProcAddress(g_comctl32.get(), "ImageList_LoadImageW");
	assert(proc_ImageList_LoadImageW != nullptr);
	g_fn_ImageList_LoadImageW = proc_ImageList_LoadImageW;

	auto const proc_DrawStatusTextW = GetProcAddress(g_comctl32.get(), "DrawStatusTextW");
	assert(proc_DrawStatusTextW != nullptr);
	g_fn_DrawStatusTextW = proc_DrawStatusTextW;
}

void common_controls::unload()
{
	assert(g_comctl32);
	g_comctl32.reset();
	assert(!g_comctl32);
}

void common_controls::InitCommonControls()
{
	assert(g_fn_InitCommonControls);
	auto const fn = reinterpret_cast<decltype(&::InitCommonControls)>(g_fn_InitCommonControls);
	fn();
}

HIMAGELIST common_controls::ImageList_LoadImageW(HINSTANCE hi, LPCWSTR lpbmp, int cx, int cGrow, COLORREF crMask, UINT uType, UINT uFlags)
{
	assert(g_fn_ImageList_LoadImageW);
	auto const fn = reinterpret_cast<decltype(&::ImageList_LoadImageW)>(g_fn_ImageList_LoadImageW);
	return fn(hi, lpbmp, cx, cGrow, crMask, uType, uFlags);
}

void common_controls::DrawStatusTextW(HDC hDC, LPCRECT lprc, LPCWSTR pszText, UINT uFlags)
{
	assert(g_fn_DrawStatusTextW);
	auto const fn = reinterpret_cast<decltype(&::DrawStatusTextW)>(g_fn_DrawStatusTextW);
	fn(hDC, lprc, pszText, uFlags);
}
