#include "splitter_window.h"

#include "main.h"

#include "../nogui/cassert_my.h"

#include <algorithm>

#include "../nogui/my_windows.h"

#include <windowsx.h>


template<splitter_window_orientation orientation>
void splitter_window<orientation>::register_class()
{
	assert(g_class == 0);

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
	wc.lpszClassName = s_window_class_name;
	wc.hIconSm = nullptr;

	ATOM const klass = RegisterClassExW(&wc);
	assert(klass != 0);

	g_class = klass;
}

template<splitter_window_orientation orientation>
splitter_window<orientation>::splitter_window(HWND const& parent) :
	m_hwnd(CreateWindowExW(0, reinterpret_cast<wchar_t const*>(g_class), nullptr, WS_CLIPCHILDREN | WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, parent, nullptr, get_instance(), nullptr)),
	m_first(),
	m_second(),
	m_position(0.50f),
	m_sizing(false)
{
	assert(parent != nullptr);
	assert(m_hwnd != nullptr);
	LONG_PTR const set = SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
}

template<splitter_window_orientation orientation>
splitter_window<orientation>::~splitter_window()
{
	assert(m_hwnd == nullptr);
}

template<splitter_window_orientation orientation>
HWND const& splitter_window<orientation>::get_hwnd() const
{
	return m_hwnd;
}

template<splitter_window_orientation orientation>
void splitter_window<orientation>::set_elements(HWND const& first, HWND const& second)
{
	m_first = first;
	m_second = second;
}

template<splitter_window_orientation orientation>
void splitter_window<orientation>::set_position(float const& position)
{
	assert(position >= 0.01f && position <= 0.99f);
	m_position = position;
	refresh_children();
}

template<splitter_window_orientation orientation>
LRESULT CALLBACK splitter_window<orientation>::class_proc(HWND const hwnd, UINT const msg, WPARAM const wparam, LPARAM const lparam)
{
	LONG_PTR const ptr = GetWindowLongPtrW(hwnd, GWLP_USERDATA);
	splitter_window* const self = reinterpret_cast<splitter_window*>(ptr);
	if(self)
	{
		LRESULT const ret = self->on_message(msg, wparam, lparam);
		return ret;
	}
	else
	{
		LRESULT const ret = DefWindowProcW(hwnd, msg, wparam, lparam);
		return ret;
	}
}

template<splitter_window_orientation orientation>
LRESULT splitter_window<orientation>::on_message(UINT const& msg, WPARAM const& wparam, LPARAM const& lparam)
{
	switch(msg)
	{
		case WM_DESTROY:
		{
			LRESULT const ret = on_wm_destroy(wparam, lparam);
			return ret;
		}
		break;
		case WM_SIZE:
		{
			LRESULT const ret = on_wm_size(wparam, lparam);
			return ret;
		}
		break;
		case  WM_NOTIFY:
		{
			LRESULT const ret = on_wm_notify(wparam, lparam);
			return ret;
		}
		break;
		case WM_CONTEXTMENU:
		{
			LRESULT const ret = on_wm_contextmenu(wparam, lparam);
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
		default:
		{
			LRESULT const ret = DefWindowProcW(m_hwnd, msg, wparam, lparam);
			return ret;
		}
		break;
	}
}

template<splitter_window_orientation orientation>
LRESULT splitter_window<orientation>::on_wm_destroy(WPARAM const& wparam, LPARAM const& lparam)
{
	HWND const old_hwnd = m_hwnd;
	LONG_PTR const set = SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(nullptr));
	m_hwnd = nullptr;

	LRESULT const ret = DefWindowProcW(old_hwnd, WM_DESTROY, wparam, lparam);
	return ret;
}

template<>
LRESULT splitter_window<splitter_window_orientation::horizontal>::on_wm_size(WPARAM const& wparam, LPARAM const& lparam)
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

	BOOL const moved_1 = MoveWindow(m_first, first_new_x, first_new_y, first_new_w, first_new_h, TRUE);
	BOOL const moved_2 = MoveWindow(m_second, second_new_x, second_new_y, second_new_w, second_new_h, TRUE);

	LRESULT const ret = DefWindowProcW(m_hwnd, WM_SIZE, wparam, lparam);
	return ret;
}

template<>
LRESULT splitter_window<splitter_window_orientation::vertical>::on_wm_size(WPARAM const& wparam, LPARAM const& lparam)
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

	BOOL const moved_1 = MoveWindow(m_first, first_new_x, first_new_y, first_new_w, first_new_h, TRUE);
	BOOL const moved_2 = MoveWindow(m_second, second_new_x, second_new_y, second_new_w, second_new_h, TRUE);

	LRESULT const ret = DefWindowProcW(m_hwnd, WM_SIZE, wparam, lparam);
	return ret;
}

template<splitter_window_orientation orientation>
LRESULT splitter_window<orientation>::on_wm_notify(WPARAM const& wparam, LPARAM const& lparam)
{
	LONG_PTR const ptr = GetWindowLongPtrW(m_hwnd, GWLP_HWNDPARENT);
	HWND const parent_hwnd = reinterpret_cast<HWND>(ptr);

	LRESULT const ret = SendMessageW(parent_hwnd, WM_NOTIFY, wparam, lparam);
	return ret;
}

template<splitter_window_orientation orientation>
LRESULT splitter_window<orientation>::on_wm_contextmenu(WPARAM const& wparam, LPARAM const& lparam)
{
	LONG_PTR const ptr = GetWindowLongPtrW(m_hwnd, GWLP_HWNDPARENT);
	HWND const parent_hwnd = reinterpret_cast<HWND>(ptr);

	LRESULT const ret = SendMessageW(parent_hwnd, WM_CONTEXTMENU, wparam, lparam);
	return ret;
}

template<>
LRESULT splitter_window<splitter_window_orientation::horizontal>::on_wm_mousemove(WPARAM const& wparam, LPARAM const& lparam)
{
	if(!m_sizing)
	{
		LRESULT const ret = DefWindowProcW(m_hwnd, WM_MOUSEMOVE, wparam, lparam);
		return ret;
	}

	int const y = GET_Y_LPARAM(lparam);
	RECT r;
	BOOL const got_rect = GetClientRect(m_hwnd, &r);
	assert(got_rect != 0);
	m_position = (std::min)((std::max)(static_cast<float>(y) / static_cast<float>(r.bottom), 0.01f), 0.99f);
	refresh_children();

	LRESULT const ret = DefWindowProcW(m_hwnd, WM_MOUSEMOVE, wparam, lparam);
	return ret;
}

template<>
LRESULT splitter_window<splitter_window_orientation::vertical>::on_wm_mousemove(WPARAM const& wparam, LPARAM const& lparam)
{
	if(!m_sizing)
	{
		LRESULT const ret = DefWindowProcW(m_hwnd, WM_MOUSEMOVE, wparam, lparam);
		return ret;
	}

	int const x = GET_X_LPARAM(lparam);
	RECT r;
	BOOL const got_rect = GetClientRect(m_hwnd, &r);
	assert(got_rect != 0);
	m_position = (std::min)((std::max)(static_cast<float>(x) / static_cast<float>(r.right), 0.01f), 0.99f);
	refresh_children();

	LRESULT const ret = DefWindowProcW(m_hwnd, WM_MOUSEMOVE, wparam, lparam);
	return ret;
}

template<splitter_window_orientation orientation>
LRESULT splitter_window<orientation>::on_wm_lbuttondown(WPARAM const& wparam, LPARAM const& lparam)
{
	if(wparam != MK_LBUTTON)
	{
		LRESULT const ret = DefWindowProcW(m_hwnd, WM_LBUTTONDOWN, wparam, lparam);
		return ret;
	}

	[[maybe_unused]] HWND const prev = SetCapture(m_hwnd);
	m_sizing = true;

	LRESULT const ret = DefWindowProcW(m_hwnd, WM_LBUTTONDOWN, wparam, lparam);
	return ret;
}

template<splitter_window_orientation orientation>
LRESULT splitter_window<orientation>::on_wm_lbuttonup(WPARAM const& wparam, LPARAM const& lparam)
{
	m_sizing = false;
	BOOL const released = ReleaseCapture();
	assert(released != 0);

	LRESULT const ret = DefWindowProcW(m_hwnd, WM_LBUTTONUP, wparam, lparam);
	return ret;
}

template<splitter_window_orientation orientation>
void splitter_window<orientation>::refresh_children()
{
	RECT r;
	BOOL const got_rect = GetClientRect(m_hwnd, &r);
	assert(got_rect != 0);
	unsigned const w = static_cast<unsigned>(r.right) & 0xFFFFu;
	unsigned const h = static_cast<unsigned>(r.bottom) & 0xFFFFu;
	LPARAM const size = h << 16 | w;
	LRESULT const sized = on_wm_size(0, size);
}

template<> wchar_t const* const splitter_window<splitter_window_orientation::horizontal>::s_window_class_name = L"splitter_window_hor";
template<> wchar_t const* const splitter_window<splitter_window_orientation::vertical>::s_window_class_name = L"splitter_window_ver";
template<> wchar_t const* const splitter_window<splitter_window_orientation::horizontal>::s_cursor_id = reinterpret_cast<wchar_t const*>(IDC_SIZENS);
template<> wchar_t const* const splitter_window<splitter_window_orientation::vertical>::s_cursor_id = reinterpret_cast<wchar_t const*>(IDC_SIZEWE);
template<> ATOM splitter_window<splitter_window_orientation::horizontal>::g_class = 0;
template<> ATOM splitter_window<splitter_window_orientation::vertical>::g_class = 0;

template class splitter_window<splitter_window_orientation::horizontal>;
template class splitter_window<splitter_window_orientation::vertical>;
