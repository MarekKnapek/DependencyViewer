#pragma once


#include "../nogui/windows_my.h"


class smart_dc
{
public:
	smart_dc(HWND const& wnd, HDC const& dc) noexcept;
	~smart_dc() noexcept;
private:
	HWND m_wnd;
	HDC m_dc;
};
