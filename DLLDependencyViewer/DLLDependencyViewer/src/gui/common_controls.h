#pragma once


#include "../nogui/my_windows.h"

#include <commctrl.h>


namespace common_controls
{
	void load();
	void unload();
	void InitCommonControls();
	HIMAGELIST ImageList_LoadImageW(HINSTANCE hi, LPCWSTR lpbmp, int cx, int cGrow, COLORREF crMask, UINT uType, UINT uFlags);
	void DrawStatusTextW(HDC hDC, LPCRECT lprc, LPCWSTR pszText, UINT uFlags);
}
