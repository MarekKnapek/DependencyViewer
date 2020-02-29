#include "splitter_window2_impl.h"

#include "main.h"

#include "../nogui/cassert_my.h"

#include <algorithm>

#include "../nogui/windows_my.h"

#include <windowsx.h>


template<> wchar_t const* const splitter_window2_impl<splitter_window2_orientation::horizontal>::s_class_name = L"splitter_window_hor";
template<> wchar_t const* const splitter_window2_impl<splitter_window2_orientation::vertical>::s_class_name = L"splitter_window_ver";
template<> wchar_t const* const splitter_window2_impl<splitter_window2_orientation::horizontal>::s_cursor_id = reinterpret_cast<wchar_t const*>(IDC_SIZENS);
template<> wchar_t const* const splitter_window2_impl<splitter_window2_orientation::vertical>::s_cursor_id = reinterpret_cast<wchar_t const*>(IDC_SIZEWE);


template<> ATOM splitter_window2_impl<splitter_window2_orientation::horizontal>::g_class;
template<> ATOM splitter_window2_impl<splitter_window2_orientation::vertical>::g_class;


template<splitter_window2_orientation orientation>
splitter_window2_impl<orientation>::splitter_window2_impl(HWND const& self) :
	m_self(self),
	m_child_a(),
	m_child_b(),
	m_position(0.5f),
	m_sizing(false)
{
	assert(self != nullptr);
}

template<splitter_window2_orientation orientation>
splitter_window2_impl<orientation>::~splitter_window2_impl()
{
}

template<splitter_window2_orientation orientation>
void splitter_window2_impl<orientation>::init()
{
	register_class();
}

template<splitter_window2_orientation orientation>
void splitter_window2_impl<orientation>::deinit()
{
	unregister_class();
}

template<splitter_window2_orientation orientation>
wchar_t const* splitter_window2_impl<orientation>::get_class_atom()
{
	assert(g_class != 0);
	return reinterpret_cast<wchar_t const*>(g_class);
}

template<splitter_window2_orientation orientation>
void splitter_window2_impl<orientation>::register_class()
{
	WNDCLASSEXW wc{};
	wc.cbSize = sizeof(wc);
	wc.style = 0;
	wc.lpfnWndProc = &class_proc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = get_instance();
	wc.hIcon = nullptr;
	wc.hCursor = LoadCursorW(nullptr, s_cursor_id);
	wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_MENU + 1);
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = s_class_name;
	wc.hIconSm = nullptr;

	assert(g_class == 0);
	g_class = RegisterClassExW(&wc);
	assert(g_class != 0);
}

template<splitter_window2_orientation orientation>
void splitter_window2_impl<orientation>::unregister_class()
{
	assert(g_class != 0);
	BOOL const unregistered = UnregisterClassW(reinterpret_cast<wchar_t const*>(g_class), get_instance());
	assert(unregistered != 0);
	g_class = 0;
}

template<splitter_window2_orientation orientation>
LRESULT CALLBACK splitter_window2_impl<orientation>::class_proc(HWND const hwnd, UINT const msg, WPARAM const wparam, LPARAM const lparam)
{
	if(msg == WM_CREATE)
	{
		LRESULT const ret = on_wm_create(hwnd, wparam, lparam);
		return ret;
	}
	else if(msg == WM_DESTROY)
	{
		LRESULT const ret = on_wm_destroy(hwnd, wparam, lparam);
		return ret;
	}
	else
	{
		LONG_PTR const self_ptr = GetWindowLongPtrW(hwnd, GWLP_USERDATA);
		if(self_ptr != 0)
		{
			splitter_window2_impl* const self = reinterpret_cast<splitter_window2_impl*>(self_ptr);
			LRESULT const ret = self->on_message(msg, wparam, lparam);
			return ret;
		}
		else
		{
			LRESULT const ret = DefWindowProcW(hwnd, msg, wparam, lparam);
			return ret;
		}
	}
}

template<splitter_window2_orientation orientation>
LRESULT splitter_window2_impl<orientation>::on_wm_create(HWND const& hwnd, WPARAM const& wparam, LPARAM const& lparam)
{
	splitter_window2_impl* const self = new splitter_window2_impl{hwnd};
	assert((SetLastError(0), true));
	LONG_PTR const prev = SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
	assert(prev == 0 && GetLastError() == 0);

	LRESULT const ret = DefWindowProcW(hwnd, WM_CREATE, wparam, lparam);
	return ret;
}

template<splitter_window2_orientation orientation>
LRESULT splitter_window2_impl<orientation>::on_wm_destroy(HWND const& hwnd, WPARAM const& wparam, LPARAM const& lparam)
{
	LONG_PTR const prev = SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
	assert(prev != 0);
	splitter_window2_impl* const self = reinterpret_cast<splitter_window2_impl*>(prev);
	delete self;

	LRESULT const ret = DefWindowProcW(hwnd, WM_DESTROY, wparam, lparam);
	return ret;
}

template<splitter_window2_orientation orientation>
LRESULT splitter_window2_impl<orientation>::on_message(UINT const& msg, WPARAM const& wparam, LPARAM const& lparam)
{
	switch(msg)
	{
		case WM_SIZE:
		{
			LRESULT const ret = on_wm_size(wparam, lparam);
			return ret;
		}
		break;
		case WM_MOUSEMOVE:
		{
			LRESULT const ret = on_wm_mousemove(wparam, lparam);
			return ret;
		}
		break;
		case WM_LBUTTONDOWN:
		{
			LRESULT const ret = on_wm_lbuttondown(wparam, lparam);
			return ret;
		}
		break;
		case WM_LBUTTONUP:
		{
			LRESULT const ret = on_wm_lbuttonup(wparam, lparam);
			return ret;
		}
		break;
		case static_cast<std::uint32_t>(splitter_window2<orientation>::wm::wm_setchildren):
		{
			LRESULT const ret = on_wm_setchildren(wparam, lparam);
			return ret;
		}
		break;
		default:
		{
			LRESULT const ret = DefWindowProcW(m_self, msg, wparam, lparam);
			return ret;
		}
		break;
	}
}

template<>
LRESULT splitter_window2_impl<splitter_window2_orientation::horizontal>::on_wm_size(WPARAM const& wparam, LPARAM const& lparam)
{
	int const w = LOWORD(lparam);
	int const h = HIWORD(lparam);
	int const border_h = GetSystemMetrics(SM_CYEDGE);

	int const splitter_pos = static_cast<int>(static_cast<float>(h) * m_position);

	int const first_new_x = 0;
	int const first_new_y = 0;
	int const first_new_w = w;
	int const first_new_h = splitter_pos;

	int const second_new_x = 0;
	int const second_new_y = splitter_pos + border_h;
	int const second_new_w = w;
	int const second_new_h = h - splitter_pos - border_h;

	BOOL const moved_1 = MoveWindow(m_child_a, first_new_x, first_new_y, first_new_w, first_new_h, TRUE);
	BOOL const moved_2 = MoveWindow(m_child_b, second_new_x, second_new_y, second_new_w, second_new_h, TRUE);

	LRESULT const ret = DefWindowProcW(m_self, WM_SIZE, wparam, lparam);
	return ret;
}

template<>
LRESULT splitter_window2_impl<splitter_window2_orientation::vertical>::on_wm_size(WPARAM const& wparam, LPARAM const& lparam)
{
	int const w = LOWORD(lparam);
	int const h = HIWORD(lparam);
	int const border_w = GetSystemMetrics(SM_CXEDGE);

	int const splitter_pos = static_cast<int>(static_cast<float>(w) * m_position);

	int const first_new_x = 0;
	int const first_new_y = 0;
	int const first_new_w = splitter_pos;
	int const first_new_h = h;

	int const second_new_x = splitter_pos + border_w;
	int const second_new_y = 0;
	int const second_new_w = w - splitter_pos - border_w;
	int const second_new_h = h;

	BOOL const moved_1 = MoveWindow(m_child_a, first_new_x, first_new_y, first_new_w, first_new_h, TRUE);
	BOOL const moved_2 = MoveWindow(m_child_b, second_new_x, second_new_y, second_new_w, second_new_h, TRUE);

	LRESULT const ret = DefWindowProcW(m_self, WM_SIZE, wparam, lparam);
	return ret;
}

template<>
LRESULT splitter_window2_impl<splitter_window2_orientation::horizontal>::on_wm_mousemove(WPARAM const& wparam, LPARAM const& lparam)
{
	auto const on_wm_mousemove_fn = [&]()
	{
		if(!m_sizing)
		{
			return;
		}
		int const y = GET_Y_LPARAM(lparam);
		RECT r;
		BOOL const got_rect = GetClientRect(m_self, &r);
		assert(got_rect != 0);
		m_position = (std::min)((std::max)(static_cast<float>(y) / static_cast<float>(r.bottom), 0.01f), 0.99f);
	};
	on_wm_mousemove_fn();

	LRESULT const ret = DefWindowProcW(m_self, WM_MOUSEMOVE, wparam, lparam);
	return ret;
}

template<>
LRESULT splitter_window2_impl<splitter_window2_orientation::vertical>::on_wm_mousemove(WPARAM const& wparam, LPARAM const& lparam)
{
	auto const on_wm_mousemove_fn = [&]()
	{
		if(!m_sizing)
		{
			return;
		}
		int const x = GET_X_LPARAM(lparam);
		RECT r;
		BOOL const got_rect = GetClientRect(m_self, &r);
		assert(got_rect != 0);
		m_position = (std::min)((std::max)(static_cast<float>(x) / static_cast<float>(r.right), 0.01f), 0.99f);
	};
	on_wm_mousemove_fn();

	LRESULT const ret = DefWindowProcW(m_self, WM_MOUSEMOVE, wparam, lparam);
	return ret;
}

template<splitter_window2_orientation orientation>
LRESULT splitter_window2_impl<orientation>::on_wm_lbuttondown(WPARAM const& wparam, LPARAM const& lparam)
{
	auto const on_wm_lbuttondown_fn = [&]()
	{
		if(wparam != MK_LBUTTON)
		{
			return;
		}
		[[maybe_unused]] HWND const prev = SetCapture(m_self);
		m_sizing = true;
	};
	on_wm_lbuttondown_fn();

	LRESULT const ret = DefWindowProcW(m_self, WM_SIZE, wparam, lparam);
	return ret;
}

template<splitter_window2_orientation orientation>
LRESULT splitter_window2_impl<orientation>::on_wm_lbuttonup(WPARAM const& wparam, LPARAM const& lparam)
{
	m_sizing = false;
	BOOL const released = ReleaseCapture();
	assert(released != 0);

	LRESULT const ret = DefWindowProcW(m_self, WM_LBUTTONUP, wparam, lparam);
	return ret;
}

template<splitter_window2_orientation orientation>
LRESULT splitter_window2_impl<orientation>::on_wm_setchildren(WPARAM const& wparam, LPARAM const& lparam)
{
	HWND const child_a = reinterpret_cast<HWND>(wparam);
	HWND const child_b = reinterpret_cast<HWND>(lparam);
	m_child_a = child_a;
	m_child_b = child_b;

	UINT const msg = static_cast<std::uint32_t>(splitter_window2<orientation>::wm::wm_setchildren);
	LRESULT const ret = DefWindowProcW(m_self, msg, wparam, lparam);
	return ret;
}


template class splitter_window2_impl<splitter_window2_orientation::horizontal>;
template class splitter_window2_impl<splitter_window2_orientation::vertical>;
