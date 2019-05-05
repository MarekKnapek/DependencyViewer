#include "main_window.h"

#include "main.h"

#include <cassert>
#include <cstdlib>
#include <cstdint>

#include <commctrl.h>


static constexpr wchar_t const s_window_class_name[] = L"main_window";
static constexpr wchar_t const s_window_title[] = L"DLLDependencyViewer";
static constexpr int s_tree_id = 1000;
static constexpr int s_import_list = 1001;
static constexpr int s_export_list = 1002;


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
	m_hwnd(CreateWindowExW(0, reinterpret_cast<wchar_t const*>(m_s_class), s_window_title, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, get_instance(), nullptr)),
	m_splitter_hor(m_hwnd),
	m_tree(CreateWindowExW(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE, WC_TREEVIEWW, nullptr, WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, m_splitter_hor.get_hwnd(), reinterpret_cast<HMENU>(static_cast<std::uintptr_t>(s_tree_id)), get_instance(), nullptr)),
	m_splitter_ver(m_splitter_hor.get_hwnd()),
	m_import_list(CreateWindowExW(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE, WC_LISTVIEWW, nullptr, WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, m_splitter_ver.get_hwnd(), reinterpret_cast<HMENU>(static_cast<std::uintptr_t>(s_import_list)), get_instance(), nullptr)),
	m_export_list(CreateWindowExW(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE, WC_LISTVIEWW, nullptr, WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, m_splitter_ver.get_hwnd(), reinterpret_cast<HMENU>(static_cast<std::uintptr_t>(s_export_list)), get_instance(), nullptr))
{
	LONG_PTR const set = SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

	LRESULT const set_tree = SendMessageW(m_tree, TVM_SETEXTENDEDSTYLE, TVS_EX_DOUBLEBUFFER, TVS_EX_DOUBLEBUFFER);
	LRESULT const set_import = SendMessageW(m_import_list, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_DOUBLEBUFFER, LVS_EX_DOUBLEBUFFER);
	LRESULT const set_export = SendMessageW(m_export_list, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_DOUBLEBUFFER, LVS_EX_DOUBLEBUFFER);

	m_splitter_ver.set_elements(m_import_list, m_export_list);
	m_splitter_hor.set_elements(m_tree, m_splitter_ver.get_hwnd());
	RECT r;
	BOOL const got_rect = GetClientRect(m_hwnd, &r);
	LRESULT const moved = on_wm_size(0, ((static_cast<unsigned>(r.bottom) & 0xFFFFu) << 16) | (static_cast<unsigned>(r.right) & 0xFFFFu));
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
		case WM_SIZE:
		{
			return on_wm_size(wparam, lparam);
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

	return DefWindowProcW(m_hwnd, WM_DESTROY, wparam, lparam);
}

LRESULT main_window::on_wm_size(WPARAM wparam, LPARAM lparam)
{
	int const w = LOWORD(lparam);
	int const h = HIWORD(lparam);
	BOOL const moved = MoveWindow(m_splitter_hor.get_hwnd(), 0, 0, w, h, TRUE);
	return DefWindowProcW(m_hwnd, WM_SIZE, wparam, lparam);
}

ATOM main_window::m_s_class;
