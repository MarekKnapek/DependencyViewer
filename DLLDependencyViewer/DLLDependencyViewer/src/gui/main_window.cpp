#include "main_window.h"

#include "main.h"

#include "../nogui/memory_mapped_file.h"
#include "../nogui/pe.h"

#include <cassert>
#include <cstdlib>
#include <cstdint>
#include <iterator>

#include <commctrl.h>


static constexpr wchar_t const s_window_class_name[] = L"main_window";
static constexpr wchar_t const s_window_title[] = L"DLLDependencyViewer";
static constexpr wchar_t const s_menu_file[] = L"&File";
static constexpr wchar_t const s_menu_open[] = L"&Open";
static constexpr wchar_t const s_menu_exit[] = L"E&xit";
static constexpr wchar_t const s_open_file_dialog_file_name_filter[] = L"Executable files and libraries (*.exe;*.dll;*.ocx)\0*.exe;*.dll;*.ocx\0All files\0*.*\0";
static constexpr wchar_t const s_msg_error[] = L"DLLDependencyViewer error.";
static constexpr int s_menu_open_id = 2000;
static constexpr int s_menu_exit_id = 2001;
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
	m_hwnd(CreateWindowExW(0, reinterpret_cast<wchar_t const*>(m_s_class), s_window_title, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, create_menu(), get_instance(), nullptr)),
	m_splitter_hor(m_hwnd),
	m_tree(CreateWindowExW(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE, WC_TREEVIEWW, nullptr, WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, m_splitter_hor.get_hwnd(), reinterpret_cast<HMENU>(static_cast<std::uintptr_t>(s_tree_id)), get_instance(), nullptr)),
	m_splitter_ver(m_splitter_hor.get_hwnd()),
	m_import_list(CreateWindowExW(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE, WC_LISTVIEWW, nullptr, WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, m_splitter_ver.get_hwnd(), reinterpret_cast<HMENU>(static_cast<std::uintptr_t>(s_import_list)), get_instance(), nullptr)),
	m_export_list(CreateWindowExW(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE, WC_LISTVIEWW, nullptr, WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, m_splitter_ver.get_hwnd(), reinterpret_cast<HMENU>(static_cast<std::uintptr_t>(s_export_list)), get_instance(), nullptr))
{
	LONG_PTR const set = SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

	LRESULT const set_tree = SendMessageW(m_tree, TVM_SETEXTENDEDSTYLE, TVS_EX_DOUBLEBUFFER, TVS_EX_DOUBLEBUFFER);
	LONG_PTR const prev = SetWindowLongPtrW(m_tree, GWL_STYLE, GetWindowLongPtrW(m_tree, GWL_STYLE) | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_SHOWSELALWAYS);

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

HMENU main_window::create_menu()
{
	HMENU const menu_bar = CreateMenu();
	HMENU const menu_file = CreatePopupMenu();
	BOOL const menu_file_appended = AppendMenuW(menu_bar, MF_POPUP, reinterpret_cast<UINT_PTR>(menu_file), s_menu_file);
	BOOL const menu_open_appended = AppendMenuW(menu_file, MF_STRING, s_menu_open_id, s_menu_open);
	BOOL const menu_exit_appended = AppendMenuW(menu_file, MF_STRING, s_menu_exit_id, s_menu_exit);
	return menu_bar;
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
		case WM_COMMAND:
		{
			return on_wm_command(wparam, lparam);
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

LRESULT main_window::on_wm_command(WPARAM wparam, LPARAM lparam)
{
	if(HIWORD(wparam) == 0 && lparam == 0)
	{
		return on_menu(wparam, lparam);
	}
	return DefWindowProcW(m_hwnd, WM_COMMAND, wparam, lparam);
}

LRESULT main_window::on_menu(WPARAM wparam, LPARAM lparam)
{
	int const menu_id = LOWORD(wparam);
	switch(menu_id)
	{
		case s_menu_open_id:
		{
			on_menu_open();
		}
		break;
		case s_menu_exit_id:
		{
			on_menu_exit();
		}
		break;
	}
	return DefWindowProcW(m_hwnd, WM_COMMAND, wparam, lparam);
}

void main_window::on_menu_open()
{
	wchar_t buff[32 * 1024];
	buff[0] = L'\0';

	OPENFILENAMEW ofn;
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = m_hwnd;
	ofn.hInstance = nullptr;
	ofn.lpstrFilter = s_open_file_dialog_file_name_filter;
	ofn.lpstrCustomFilter = nullptr;
	ofn.nMaxCustFilter = 0;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = buff;
	ofn.nMaxFile = static_cast<int>(std::size(buff));
	ofn.lpstrFileTitle = nullptr;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = nullptr;
	ofn.lpstrTitle = nullptr;
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
	ofn.nFileOffset = 0;
	ofn.nFileExtension = 0;
	ofn.lpstrDefExt = nullptr;
	ofn.lCustData = 0;
	ofn.lpfnHook = nullptr;
	ofn.lpTemplateName = nullptr;
	ofn.pvReserved = nullptr;
	ofn.dwReserved = 0;
	ofn.FlagsEx = 0;

	BOOL const opened = GetOpenFileNameW(&ofn);
	if(opened == 0)
	{
		return;
	}

	open_file(buff);
}

void main_window::on_menu_exit()
{
	PostQuitMessage(EXIT_SUCCESS);
}

void main_window::open_file(wchar_t const* const file_name)
{
	LRESULT const deleted = SendMessageW(m_tree, TVM_DELETEITEM, 0, reinterpret_cast<LPARAM>(TVI_ROOT));

	TVINSERTSTRUCTW tvi;
	tvi.hParent = TVI_ROOT;
	tvi.hInsertAfter = TVI_ROOT;
	tvi.itemex.mask = TVIF_TEXT;
	tvi.itemex.hItem = nullptr;
	tvi.itemex.state = 0;
	tvi.itemex.stateMask = 0;
	tvi.itemex.pszText = const_cast<wchar_t*>(file_name);
	tvi.itemex.cchTextMax = 0;
	tvi.itemex.iImage = 0;
	tvi.itemex.iSelectedImage = 0;
	tvi.itemex.cChildren = 0;
	tvi.itemex.lParam = 0;
	tvi.itemex.iIntegral = 0;
	tvi.itemex.uStateEx = 0;
	tvi.itemex.hwnd = nullptr;
	tvi.itemex.iExpandedImage = 0;
	tvi.itemex.iReserved = 0;

	LRESULT const added = SendMessageW(m_tree, TVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&tvi));

	try
	{
		memory_mapped_file mmf(file_name);
		auto const hdr = pe_get_coff_header(mmf.begin(), mmf.size());
	}
	catch(wchar_t const* const ex)
	{
		int const msgbox = MessageBoxW(m_hwnd, ex, s_msg_error, MB_OK | MB_ICONERROR);
	}
}

ATOM main_window::m_s_class;
