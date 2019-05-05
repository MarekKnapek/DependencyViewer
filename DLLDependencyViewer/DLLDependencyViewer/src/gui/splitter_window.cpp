#include "splitter_window.h"

#include "main.h"


template<splitter_window_orientation orientation>
void splitter_window<orientation>::register_class()
{
	WNDCLASSEXW wc;
	wc.cbSize = sizeof(wc);
	wc.style = 0;
	wc.lpfnWndProc = &splitter_window::class_proc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = get_instance();
	wc.hIcon = nullptr;
	wc.hCursor = LoadCursorW(nullptr, s_cursor_id);
	wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = s_window_class_name;
	wc.hIconSm = nullptr;

	ATOM const klass = RegisterClassExW(&wc);

	m_s_class = klass;
}

template<splitter_window_orientation orientation>
splitter_window<orientation>::splitter_window(HWND const parent) :
	m_hwnd(CreateWindowExW(0, reinterpret_cast<wchar_t const*>(m_s_class), nullptr, WS_BORDER | WS_CLIPCHILDREN | WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, parent, nullptr, get_instance(), nullptr)),
	m_first(),
	m_second()
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
	}
	LRESULT const ret = DefWindowProcW(m_hwnd, msg, wparam, lparam);
	return ret;
}

template<splitter_window_orientation orientation>
LRESULT splitter_window<orientation>::on_wm_destroy(WPARAM wparam, LPARAM lparam)
{
	LONG_PTR const set = SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(nullptr));
	m_hwnd = nullptr;

	return DefWindowProcW(m_hwnd, WM_DESTROY, wparam, lparam);
}

template<>
LRESULT splitter_window<splitter_window_orientation::horizontal>::on_wm_size(WPARAM wparam, LPARAM lparam)
{
	int const w = LOWORD(lparam);
	int const h = HIWORD(lparam);
	BOOL const moved_1 = MoveWindow(m_first, 0, 0, w / 2, h, TRUE);
	BOOL const moved_2 = MoveWindow(m_second, w / 2, 0, w / 2, h, TRUE);
	return DefWindowProcW(m_hwnd, WM_SIZE, wparam, lparam);
}

template<>
LRESULT splitter_window<splitter_window_orientation::vertical>::on_wm_size(WPARAM wparam, LPARAM lparam)
{
	int const w = LOWORD(lparam);
	int const h = HIWORD(lparam);
	BOOL const moved_1 = MoveWindow(m_first, 0, 0, w, h / 2, TRUE);
	BOOL const moved_2 = MoveWindow(m_second, 0, h / 2, w, h / 2, TRUE);
	return DefWindowProcW(m_hwnd, WM_SIZE, wparam, lparam);
}

template<> wchar_t const splitter_window<splitter_window_orientation::horizontal>::s_window_class_name[] = L"splitter_window_hor";
template<> wchar_t const splitter_window<splitter_window_orientation::vertical>::s_window_class_name[] = L"splitter_window_ver";
template<> wchar_t const* const splitter_window<splitter_window_orientation::horizontal>::s_cursor_id = IDC_SIZEWE;
template<> wchar_t const* const splitter_window<splitter_window_orientation::vertical>::s_cursor_id = IDC_SIZENS;

template<splitter_window_orientation orientation>
ATOM splitter_window<orientation>::m_s_class;

template splitter_window<splitter_window_orientation::horizontal>;
template splitter_window<splitter_window_orientation::vertical>;
