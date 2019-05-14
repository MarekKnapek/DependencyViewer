#include "main_window.h"

#include "main.h"

#include "../nogui/memory_mapped_file.h"
#include "../nogui/pe.h"

#include <cassert>
#include <cstdlib>
#include <cstdint>
#include <iterator>
#include <cassert>


static constexpr wchar_t const s_window_class_name[] = L"main_window";
static constexpr wchar_t const s_window_title[] = L"DLLDependencyViewer";
static constexpr wchar_t const s_menu_file[] = L"&File";
static constexpr wchar_t const s_menu_open[] = L"&Open";
static constexpr wchar_t const s_menu_exit[] = L"E&xit";
static constexpr wchar_t const s_open_file_dialog_file_name_filter[] = L"Executable files and libraries (*.exe;*.dll;*.ocx)\0*.exe;*.dll;*.ocx\0All files\0*.*\0";
static constexpr wchar_t const s_msg_error[] = L"DLLDependencyViewer error.";
static constexpr wchar_t const* const s_import_headers[] = { L"type", L"ordinal", L"hint", L"function" };
static constexpr wchar_t const s_import_type_true[] = L"ordinal";
static constexpr wchar_t const s_import_type_false[] = L"name";
static constexpr wchar_t const s_import_ordinal_na[] = L"N/A";
static constexpr wchar_t const s_import_hint_na[] = L"N/A";
static constexpr wchar_t const s_import_function_na[] = L"N/A";
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
	m_import_list(CreateWindowExW(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE, WC_LISTVIEWW, nullptr, WS_VISIBLE | WS_CHILD | LVS_REPORT | LVS_SHOWSELALWAYS, 0, 0, 0, 0, m_splitter_ver.get_hwnd(), reinterpret_cast<HMENU>(static_cast<std::uintptr_t>(s_import_list)), get_instance(), nullptr)),
	m_export_list(CreateWindowExW(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE, WC_LISTVIEWW, nullptr, WS_VISIBLE | WS_CHILD | LVS_REPORT | LVS_SHOWSELALWAYS, 0, 0, 0, 0, m_splitter_ver.get_hwnd(), reinterpret_cast<HMENU>(static_cast<std::uintptr_t>(s_export_list)), get_instance(), nullptr)),
	m_tmp_strings(),
	m_tmp_string_idx(),
	m_file_info()
{
	LONG_PTR const set = SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

	LRESULT const set_tree = SendMessageW(m_tree, TVM_SETEXTENDEDSTYLE, TVS_EX_DOUBLEBUFFER, TVS_EX_DOUBLEBUFFER);
	LONG_PTR const prev = SetWindowLongPtrW(m_tree, GWL_STYLE, GetWindowLongPtrW(m_tree, GWL_STYLE) | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_SHOWSELALWAYS);

	unsigned const extended_lv_styles = LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP | LVS_EX_DOUBLEBUFFER;
	LRESULT const set_import = SendMessageW(m_import_list, LVM_SETEXTENDEDLISTVIEWSTYLE, extended_lv_styles, extended_lv_styles);
	for(int i = 0; i != static_cast<int>(std::size(s_import_headers)); ++i)
	{
		LVCOLUMNW cl;
		cl.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		cl.fmt = LVCFMT_LEFT;
		cl.cx = 100;
		cl.pszText = const_cast<LPWSTR>(s_import_headers[i]);
		cl.cchTextMax = 0;
		cl.iSubItem = i;
		cl.iImage = 0;
		cl.iOrder = i;
		cl.cxMin = 0;
		cl.cxDefault = 100;
		cl.cxIdeal = 100;
		auto const inserted = SendMessageW(m_import_list, LVM_INSERTCOLUMNW, i, reinterpret_cast<LPARAM>(&cl));
	}

	LRESULT const set_export = SendMessageW(m_export_list, LVM_SETEXTENDEDLISTVIEWSTYLE, extended_lv_styles, extended_lv_styles);

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
		case WM_NOTIFY:
		{
			return on_wm_notify(wparam, lparam);
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

LRESULT main_window::on_wm_notify(WPARAM wparam, LPARAM lparam)
{
	NMHDR& nmhdr = *reinterpret_cast<NMHDR*>(lparam);
	if(nmhdr.hwndFrom == m_tree)
	{
		on_tree_notify(reinterpret_cast<NMTREEVIEWW&>(nmhdr));
	}
	else if(nmhdr.hwndFrom == m_import_list)
	{
		on_import_notify(reinterpret_cast<NMLVDISPINFOW&>(nmhdr));
	}
	return DefWindowProcW(m_hwnd, WM_NOTIFY, wparam, lparam);
}

void main_window::on_tree_notify(NMTREEVIEWW& nm)
{
	if(nm.hdr.code == TVN_SELCHANGEDW)
	{
		LRESULT const redr_off = SendMessageW(m_import_list, WM_SETREDRAW, FALSE, 0);
		LRESULT const deleted = SendMessageW(m_import_list, LVM_DELETEALLITEMS, 0, 0);
		pe_import_dll_with_entries const* const dll_with_entries = reinterpret_cast<pe_import_dll_with_entries*>(nm.itemNew.lParam);
		if(dll_with_entries)
		{
			LRESULT const set_size = SendMessageW(m_import_list, LVM_SETITEMCOUNT, dll_with_entries->m_entries.size(), 0);
			for(int row = 0; row != dll_with_entries->m_entries.size(); ++row)
			{
				LVITEMW lv;
				lv.mask = LVIF_TEXT | LVIF_PARAM;
				lv.iItem = row;
				lv.iSubItem = 0;
				lv.state = 0;
				lv.stateMask = 0;
				lv.pszText = LPSTR_TEXTCALLBACKW;
				lv.cchTextMax = 0;
				lv.iImage = 0;
				lv.lParam = reinterpret_cast<LPARAM>(const_cast<pe_import_entry*>(&dll_with_entries->m_entries[row]));
				lv.iIndent = 0;
				lv.iGroupId = 0;
				lv.cColumns = 0;
				lv.puColumns = nullptr;
				lv.piColFmt = nullptr;
				lv.iGroup = 0;
				LRESULT const added = SendMessageW(m_import_list, LVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&lv));
			}
		}
		for(int i = 0; i != static_cast<int>(std::size(s_import_headers)); ++i)
		{
			LRESULT const auto_sized = SendMessageW(m_import_list, LVM_SETCOLUMNWIDTH, i, LVSCW_AUTOSIZE);
		}
		LRESULT const redr_on = SendMessageW(m_import_list, WM_SETREDRAW, TRUE, 0);
	}
}

void main_window::on_import_notify(NMLVDISPINFOW& nm)
{
	if(nm.hdr.code == LVN_GETDISPINFOW)
	{
		if((nm.item.mask | LVIF_TEXT) != 0)
		{
			pe_import_entry const& import_entry = *reinterpret_cast<pe_import_entry*>(nm.item.lParam);
			int const row = nm.item.iItem;
			int const col = nm.item.iSubItem;
			switch(col)
			{
				case 0:
				{
					if(import_entry.m_is_ordinal)
					{
						nm.item.pszText = const_cast<wchar_t*>(s_import_type_true);
					}
					else
					{
						nm.item.pszText = const_cast<wchar_t*>(s_import_type_false);
					}
				}
				break;
				case 1:
				{
					if(import_entry.m_is_ordinal)
					{
						static_assert(sizeof(std::uint16_t) == sizeof(unsigned short int), "");
						std::array<wchar_t, 32> buff;
						int const formatted = std::swprintf(buff.data(), buff.size(), L"%hu (%#04hx)", static_cast<unsigned short int>(import_entry.m_ordinal_or_hint), static_cast<unsigned short int>(import_entry.m_ordinal_or_hint));
						std::wstring& tmpstr = m_tmp_strings[m_tmp_string_idx++ % 4];
						tmpstr.assign(buff.data(), buff.data() + formatted);
						nm.item.pszText = const_cast<wchar_t*>(tmpstr.c_str());
					}
					else
					{
						nm.item.pszText = const_cast<wchar_t*>(s_import_ordinal_na);
					}
				}
				break;
				case 2:
				{
					if(import_entry.m_is_ordinal)
					{
						nm.item.pszText = const_cast<wchar_t*>(s_import_hint_na);
					}
					else
					{
						static_assert(sizeof(std::uint16_t) == sizeof(unsigned short int), "");
						std::array<wchar_t, 32> buff;
						int const formatted = std::swprintf(buff.data(), buff.size(), L"%hu (%#04hx)", static_cast<unsigned short int>(import_entry.m_ordinal_or_hint), static_cast<unsigned short int>(import_entry.m_ordinal_or_hint));
						std::wstring& tmpstr = m_tmp_strings[m_tmp_string_idx++ % 4];
						tmpstr.assign(buff.data(), buff.data() + formatted);
						nm.item.pszText = const_cast<wchar_t*>(tmpstr.c_str());
					}
				}
				break;
				case 3:
				{
					if(import_entry.m_is_ordinal)
					{
						nm.item.pszText = const_cast<wchar_t*>(s_import_function_na);
					}
					else
					{
						std::wstring& tmpstr = m_tmp_strings[m_tmp_string_idx++ % 4];
						tmpstr.resize(import_entry.m_name.size());
						std::transform(cbegin(import_entry.m_name), cend(import_entry.m_name), begin(tmpstr), [](char const& e) -> wchar_t { return static_cast<wchar_t>(e); });
						nm.item.pszText = const_cast<wchar_t*>(tmpstr.c_str());
					}
				}
				break;
				default:
				{
					assert(false);
				}
				break;
			}
		}
	}
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
	pe_import_table_info iti;
	try
	{
		memory_mapped_file const mmf = memory_mapped_file(file_name);
		pe_header_info const hi = pe_process_header(mmf.begin(), mmf.size());
		iti = pe_process_import_table(mmf.begin(), mmf.size(), hi);
	}
	catch(wchar_t const* const ex)
	{
		int const msgbox = MessageBoxW(m_hwnd, ex, s_msg_error, MB_OK | MB_ICONERROR);
	}
	m_file_info.m_file_name.assign(file_name);
	m_file_info.m_import_table = std::move(iti);
	refresh_view();
}

void main_window::refresh_view()
{
	LRESULT const deleted = SendMessageW(m_tree, TVM_DELETEITEM, 0, reinterpret_cast<LPARAM>(TVI_ROOT));

	TVINSERTSTRUCTW tvi;
	tvi.hParent = TVI_ROOT;
	tvi.hInsertAfter = TVI_ROOT;
	tvi.itemex.mask = TVIF_TEXT;
	tvi.itemex.hItem = nullptr;
	tvi.itemex.state = 0;
	tvi.itemex.stateMask = 0;
	tvi.itemex.pszText = const_cast<wchar_t*>(m_file_info.m_file_name.c_str());
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
	LRESULT const hroot = SendMessageW(m_tree, TVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&tvi));

	for(auto const& dll_with_entries : m_file_info.m_import_table.m_dlls)
	{
		std::wstring& tmpstr = m_tmp_strings[0];
		tmpstr.resize(dll_with_entries.m_dll.size());
		std::transform(cbegin(dll_with_entries.m_dll), cend(dll_with_entries.m_dll), begin(tmpstr), [](char const& e) -> wchar_t { return static_cast<wchar_t>(e); });
		TVINSERTSTRUCTW tvi;
		tvi.hParent = reinterpret_cast<HTREEITEM>(hroot);
		tvi.hInsertAfter = TVI_LAST;
		tvi.itemex.mask = TVIF_TEXT | TVIF_PARAM;
		tvi.itemex.hItem = nullptr;
		tvi.itemex.state = 0;
		tvi.itemex.stateMask = 0;
		tvi.itemex.pszText = const_cast<wchar_t*>(tmpstr.c_str());
		tvi.itemex.cchTextMax = 0;
		tvi.itemex.iImage = 0;
		tvi.itemex.iSelectedImage = 0;
		tvi.itemex.cChildren = 0;
		tvi.itemex.lParam = reinterpret_cast<LPARAM>(const_cast<pe_import_dll_with_entries*>(&dll_with_entries));
		tvi.itemex.iIntegral = 0;
		tvi.itemex.uStateEx = 0;
		tvi.itemex.hwnd = nullptr;
		tvi.itemex.iExpandedImage = 0;
		tvi.itemex.iReserved = 0;
		LRESULT const added_dll = SendMessageW(m_tree, TVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&tvi));
	}
	LRESULT const expanded = SendMessageW(m_tree, TVM_EXPAND, TVE_EXPAND, reinterpret_cast<LPARAM>(reinterpret_cast<HTREEITEM>(hroot)));
	LRESULT const selected = SendMessageW(m_tree, TVM_SELECTITEM, TVGN_CARET, reinterpret_cast<LPARAM>(reinterpret_cast<HTREEITEM>(hroot)));
}

ATOM main_window::m_s_class;
