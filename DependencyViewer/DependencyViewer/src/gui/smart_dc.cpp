#include "smart_dc.h"

#include "../nogui/cassert_my.h"


smart_dc::smart_dc(HWND const& wnd, HDC const& dc) noexcept :
	m_wnd(wnd),
	m_dc(dc)
{
}

smart_dc::~smart_dc() noexcept
{
	int const released = ReleaseDC(m_wnd, m_dc);
	assert(released == 1);
}
