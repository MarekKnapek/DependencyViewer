#include "splitter_window.h"

#include "main.h"

#include <algorithm>

#include "../nogui/my_windows.h"

#include <windowsx.h>


template<splitter_window_orientation orientation>
void splitter_window<orientation>::register_class()
{
	WNDCLASSEXW wc{};
	wc.cbSize = sizeof(wc);
	wc.style = 0;
	wc.lpfnWndProc = &splitter_window::class_proc;
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

	g_class = klass;
}

template<splitter_window_orientation orientation>
splitter_window<orientation>::splitter_window(HWND const parent) :
	m_hwnd(CreateWindowExW(0, reinterpret_cast<wchar_t const*>(g_class), nullptr, WS_CLIPCHILDREN | WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, parent, nullptr, get_instance(), nullptr)),
	m_first(),
	m_second(),
	m_position(0.50f),
	m_sizing(false)
{
	LONG_PTR const set = SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
}

template<splitter_window_orientation orientation>
splitter_window<orientation>::~splitter_window()
{
}

template<splitter_window_orientation orientation>
HWND splitter_window<orientation>::get_hwnd() const
{
	return m_hwnd;
}

template<splitter_window_orientation orientation>
void splitter_window<orientation>::set_elements(HWND const first, HWND const second)
{
	m_first = first;
	m_second = second;
}

template<splitter_window_orientation orientation>
LRESULT CALLBACK splitter_window<orientation>::class_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if(LONG_PTR const ptr = GetWindowLongPtrW(hwnd, GWLP_USERDATA))
	{
		splitter_window& win = *reinterpret_cast<splitter_window*>(ptr);
		LRESULT const ret = win.on_message(msg, wparam, lparam);
		return ret;
	}
	LRESULT const ret = DefWindowProcW(hwnd, msg, wparam, lparam);
	return ret;
}

template<splitter_window_orientation orientation>
LRESULT splitter_window<orientation>::on_message(UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch(msg)
	{
		case WM_DESTROY:
		{
			return on_wm_destroy(wparam, lparam);
		}
		break;
		case WM_SIZE:
		{
			return on_wm_size(wparam, lparam);
		}
		break;
		case  WM_NOTIFY:
		{
			return on_wm_notify(wparam, lparam);
		}
		break;
		case WM_CONTEXTMENU:
		{
			return on_wm_contextmenu(wparam, lparam);
		}
		break;
		case WM_MOUSEMOVE:
		{
			return on_wm_mousemove(wparam, lparam);
		}
		break;
		case WM_LBUTTONDOWN:
		{
			return on_wm_lbuttondown(wparam, lparam);
		}
		break;
		case WM_LBUTTONUP:
		{
			return on_wm_lbuttonup(wparam, lparam);
		}
		break;
	}
	LRESULT const ret = DefWindowProcW(m_hwnd, msg, wparam, lparam);
	return ret;
}

template<splitter_window_orientation orientation>
LRESULT splitter_window<orientation>::on_wm_destroy(WPARAM wparam, LPARAM lparam)
{
	LONG_PTR const set = SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(nullptr));
	HWND const old_hwnd = m_hwnd;
	m_hwnd = nullptr;

	return DefWindowProcW(old_hwnd, WM_DESTROY, wparam, lparam);
}

template<>
LRESULT splitter_window<splitter_window_orientation::horizontal>::on_wm_size(WPARAM wparam, LPARAM lparam)
{
	int const w = LOWORD(lparam);
	int const h = HIWORD(lparam);
	int const edge_x = GetSystemMetrics(SM_CXEDGE);

	int const x_0 = 0;
	int const x_1 = static_cast<int>(static_cast<float>(w) * m_position) - edge_x;
	int const x_2 = x_1 + edge_x;
	int const x_3 = w - x_2;

	BOOL const moved_1 = MoveWindow(m_first, x_0, 0, x_1, h, TRUE);
	BOOL const moved_2 = MoveWindow(m_second, x_2, 0, x_3, h, TRUE);

	return DefWindowProcW(m_hwnd, WM_SIZE, wparam, lparam);
}

template<>
LRESULT splitter_window<splitter_window_orientation::vertical>::on_wm_size(WPARAM wparam, LPARAM lparam)
{
	int const w = LOWORD(lparam);
	int const h = HIWORD(lparam);
	int const edge_y = GetSystemMetrics(SM_CXEDGE);

	int const y_0 = 0;
	int const y_1 = static_cast<int>(static_cast<float>(h) * m_position) - edge_y;
	int const y_2 = y_1 + edge_y;
	int const y_3 = h - y_2;

	BOOL const moved_1 = MoveWindow(m_first, 0, y_0, w, y_1, TRUE);
	BOOL const moved_2 = MoveWindow(m_second, 0, y_2, w, y_3, TRUE);

	return DefWindowProcW(m_hwnd, WM_SIZE, wparam, lparam);
}

template<splitter_window_orientation orientation>
LRESULT splitter_window<orientation>::on_wm_notify(WPARAM wparam, LPARAM lparam)
{
	return SendMessageW(reinterpret_cast<HWND>(GetWindowLongPtrW(m_hwnd, GWLP_HWNDPARENT)), WM_NOTIFY, wparam, lparam);
}

template<splitter_window_orientation orientation>
LRESULT splitter_window<orientation>::on_wm_contextmenu(WPARAM wparam, LPARAM lparam)
{
	return SendMessageW(reinterpret_cast<HWND>(GetWindowLongPtrW(m_hwnd, GWLP_HWNDPARENT)), WM_CONTEXTMENU, wparam, lparam);
}

template<>
LRESULT splitter_window<splitter_window_orientation::horizontal>::on_wm_mousemove(WPARAM wparam, LPARAM lparam)
{
	if(!m_sizing)
	{
		return DefWindowProcW(m_hwnd, WM_MOUSEMOVE, wparam, lparam);
	}

	int const x = GET_X_LPARAM(lparam);
	RECT r;
	BOOL const got_rect_1 = GetClientRect(m_hwnd, &r);
	m_position = (std::min)((std::max)(static_cast<float>(x) / static_cast<float>(r.right), 0.01f), 0.99f);

	BOOL const got_rect_2 = GetClientRect(m_hwnd, &r);
	LRESULT const moved = on_wm_size(0, ((static_cast<unsigned>(r.bottom) & 0xFFFFu) << 16) | (static_cast<unsigned>(r.right) & 0xFFFFu));

	return DefWindowProcW(m_hwnd, WM_MOUSEMOVE, wparam, lparam);
}

template<>
LRESULT splitter_window<splitter_window_orientation::vertical>::on_wm_mousemove(WPARAM wparam, LPARAM lparam)
{
	if(!m_sizing)
	{
		return DefWindowProcW(m_hwnd, WM_MOUSEMOVE, wparam, lparam);
	}

	int const y = GET_Y_LPARAM(lparam);
	RECT r;
	BOOL const got_rect_1 = GetClientRect(m_hwnd, &r);
	m_position = (std::min)((std::max)(static_cast<float>(y) / static_cast<float>(r.bottom), 0.01f), 0.99f);

	BOOL const got_rect_2 = GetClientRect(m_hwnd, &r);
	LRESULT const moved = on_wm_size(0, ((static_cast<unsigned>(r.bottom) & 0xFFFFu) << 16) | (static_cast<unsigned>(r.right) & 0xFFFFu));

	return DefWindowProcW(m_hwnd, WM_MOUSEMOVE, wparam, lparam);
}

template<splitter_window_orientation orientation>
LRESULT splitter_window<orientation>::on_wm_lbuttondown(WPARAM wparam, LPARAM lparam)
{
	if(wparam != MK_LBUTTON)
	{
		return DefWindowProcW(m_hwnd, WM_LBUTTONDOWN, wparam, lparam);
	}
	SetCapture(m_hwnd);
	m_sizing = true;
	return DefWindowProcW(m_hwnd, WM_LBUTTONDOWN, wparam, lparam);
}

template<splitter_window_orientation orientation>
LRESULT splitter_window<orientation>::on_wm_lbuttonup(WPARAM wparam, LPARAM lparam)
{
	m_sizing = false;
	BOOL const released = ReleaseCapture();
	return DefWindowProcW(m_hwnd, WM_LBUTTONUP, wparam, lparam);
}

template<> wchar_t const splitter_window<splitter_window_orientation::horizontal>::s_window_class_name[] = L"splitter_window_hor";
template<> wchar_t const splitter_window<splitter_window_orientation::vertical>::s_window_class_name[] = L"splitter_window_ver";
template<> wchar_t const* const splitter_window<splitter_window_orientation::horizontal>::s_cursor_id = reinterpret_cast<wchar_t const*>(IDC_SIZEWE);
template<> wchar_t const* const splitter_window<splitter_window_orientation::vertical>::s_cursor_id = reinterpret_cast<wchar_t const*>(IDC_SIZENS);

template<splitter_window_orientation orientation>
ATOM splitter_window<orientation>::g_class = 0;

template class splitter_window<splitter_window_orientation::horizontal>;
template class splitter_window<splitter_window_orientation::vertical>;
