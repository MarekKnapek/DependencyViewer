#include "main_window.h"

#include "main.h"

#include <cassert>
#include <cstdlib>


static constexpr wchar_t const s_window_class_name[] = L"main_window";
static constexpr wchar_t const s_window_title[] = L"DLLDependencyViewer";


void main_window::register_class()
{
	WNDCLASSEXW wc;
	wc.cbSize = sizeof(wc);
	wc.style = 0;
	wc.lpfnWndProc = &main_window::class_proc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = get_instance();
	wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
	wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = s_window_class_name;
	wc.hIconSm = LoadIconW(nullptr, IDI_APPLICATION);

	ATOM const klass = RegisterClassExW(&wc);

	m_s_class = klass;
}

main_window::main_window() :
	m_hwnd(CreateWindowExW(0, reinterpret_cast<wchar_t const*>(m_s_class), s_window_title, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, get_instance(), this))
{
}

main_window::~main_window()
{
}

HWND main_window::get_hwnd() const
{
	return m_hwnd;
}

LRESULT CALLBACK main_window::class_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if(msg == WM_CREATE)
	{
		CREATESTRUCTW const& ct = *reinterpret_cast<CREATESTRUCTW*>(lparam);
		main_window* const win = static_cast<main_window*>(ct.lpCreateParams);
		win->m_hwnd = hwnd;
		LONG_PTR const set = SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(win));
	}
	if(LONG_PTR const ptr = GetWindowLongPtrW(hwnd, GWLP_USERDATA))
	{
		main_window& win = *reinterpret_cast<main_window*>(ptr);
		LRESULT const ret = win.on_message(msg, wparam, lparam);
		return ret;
	}
	LRESULT const ret = DefWindowProcW(hwnd, msg, wparam, lparam);
	return ret;
}

LRESULT main_window::on_message(UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch(msg)
	{
		case WM_DESTROY:
		{
			return on_wm_destroy(wparam, lparam);
		}
		break;
	}
	LRESULT const ret = DefWindowProcW(m_hwnd, msg, wparam, lparam);
	return ret;
}

LRESULT main_window::on_wm_destroy(WPARAM wparam, LPARAM lparam)
{
	LONG_PTR const set = SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(nullptr));
	m_hwnd = nullptr;

	PostQuitMessage(EXIT_SUCCESS);

	return 0;
}

ATOM main_window::m_s_class;
