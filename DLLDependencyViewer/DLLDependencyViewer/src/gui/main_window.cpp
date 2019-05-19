#include "main_window.h"

#include "main.h"

#include "../nogui/memory_mapped_file.h"
#include "../nogui/pe.h"
#include "../nogui/utils.h"

#include <cassert>
#include <cstdlib>
#include <cstdint>
#include <iterator>
#include <cassert>

#include <commctrl.h>
#include <shellapi.h>


static constexpr wchar_t const s_window_class_name[] = L"main_window";
static constexpr wchar_t const s_window_title[] = L"DLLDependencyViewer";
static constexpr wchar_t const s_menu_file[] = L"&File";
static constexpr wchar_t const s_menu_open[] = L"&Open";
static constexpr wchar_t const s_menu_exit[] = L"E&xit";
static constexpr wchar_t const s_open_file_dialog_file_name_filter[] = L"Executable files and libraries (*.exe;*.dll;*.ocx)\0*.exe;*.dll;*.ocx\0All files\0*.*\0";
static constexpr wchar_t const s_msg_error[] = L"DLLDependencyViewer error.";
static constexpr wchar_t const* const s_import_headers[] = { L"type", L"ordinal", L"hint", L"name" };
static constexpr wchar_t const s_import_type_true[] = L"ordinal";
static constexpr wchar_t const s_import_type_false[] = L"name";
static constexpr wchar_t const s_import_ordinal_na[] = L"N/A";
static constexpr wchar_t const s_import_hint_na[] = L"N/A";
static constexpr wchar_t const s_import_name_na[] = L"N/A";
static constexpr wchar_t const* const s_export_headers[] = { L"type", L"ordinal", L"hint", L"name", L"entry point" };
static constexpr wchar_t const s_export_type_true[] = L"address";
static constexpr wchar_t const s_export_type_false[] = L"forwarder";
static constexpr wchar_t const s_export_hint_na[] = L"N/A";
static constexpr wchar_t const s_export_name_na[] = L"N/A";
static constexpr int s_menu_open_id = 2000;
static constexpr int s_menu_exit_id = 2001;
static constexpr int s_tree_id = 1000;
static constexpr int s_import_list = 1001;
static constexpr int s_export_list = 1002;


enum class e_import_column
{
	e_type,
	e_ordinal,
	e_hint,
	e_name
};

enum class e_export_column
{
	e_type,
	e_ordinal,
	e_hint,
	e_name,
	e_entry_point
};


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
	m_mo()
{
	LONG_PTR const set = SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
	DragAcceptFiles(m_hwnd, TRUE);

	LRESULT const set_tree = SendMessageW(m_tree, TVM_SETEXTENDEDSTYLE, TVS_EX_DOUBLEBUFFER, TVS_EX_DOUBLEBUFFER);
	LONG_PTR const prev = SetWindowLongPtrW(m_tree, GWL_STYLE, GetWindowLongPtrW(m_tree, GWL_STYLE) | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_SHOWSELALWAYS);

	unsigned const extended_lv_styles = LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP | LVS_EX_DOUBLEBUFFER;
	LRESULT const set_import = SendMessageW(m_import_list, LVM_SETEXTENDEDLISTVIEWSTYLE, extended_lv_styles, extended_lv_styles);
	for(int i = 0; i != static_cast<int>(std::size(s_import_headers)); ++i)
	{
		LVCOLUMNW cl;
		cl.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		cl.fmt = LVCFMT_LEFT;
		cl.cx = 0;
		cl.pszText = const_cast<LPWSTR>(s_import_headers[i]);
		cl.cchTextMax = 0;
		cl.iSubItem = i;
		cl.iImage = 0;
		cl.iOrder = i;
		cl.cxMin = 0;
		cl.cxDefault = 0;
		cl.cxIdeal = 0;
		auto const inserted = SendMessageW(m_import_list, LVM_INSERTCOLUMNW, i, reinterpret_cast<LPARAM>(&cl));
	}

	LRESULT const set_export = SendMessageW(m_export_list, LVM_SETEXTENDEDLISTVIEWSTYLE, extended_lv_styles, extended_lv_styles);
	for(int i = 0; i != static_cast<int>(std::size(s_export_headers)); ++i)
	{
		LVCOLUMNW cl;
		cl.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		cl.fmt = LVCFMT_LEFT;
		cl.cx = 0;
		cl.pszText = const_cast<LPWSTR>(s_export_headers[i]);
		cl.cchTextMax = 0;
		cl.iSubItem = i;
		cl.iImage = 0;
		cl.iOrder = i;
		cl.cxMin = 0;
		cl.cxDefault = 0;
		cl.cxIdeal = 0;
		auto const inserted = SendMessageW(m_export_list, LVM_INSERTCOLUMNW, i, reinterpret_cast<LPARAM>(&cl));
	}

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
		case WM_DROPFILES:
		{
			return on_wm_dropfiles(wparam, lparam);
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
		on_tree_notify(nmhdr);
	}
	else if(nmhdr.hwndFrom == m_import_list)
	{
		on_import_notify(nmhdr);
	}
	else if(nmhdr.hwndFrom == m_export_list)
	{
		on_export_notify(nmhdr);
	}
	return DefWindowProcW(m_hwnd, WM_NOTIFY, wparam, lparam);
}

void main_window::on_tree_notify(NMHDR& nmhdr)
{
	if(nmhdr.code == TVN_GETDISPINFOW)
	{
		NMTVDISPINFOW& di = reinterpret_cast<NMTVDISPINFOW&>(nmhdr);
		if((di.item.mask | TVIF_TEXT) != 0)
		{
			file_info const& tmp_fi = *reinterpret_cast<file_info*>(di.item.lParam);
			file_info const& fi = tmp_fi.m_orig_instance ? *tmp_fi.m_orig_instance : tmp_fi;
			HTREEITEM const parent = reinterpret_cast<HTREEITEM>(SendMessageW(m_tree, TVM_GETNEXTITEM, TVGN_PARENT, reinterpret_cast<LPARAM>(di.item.hItem)));
			if(parent)
			{
				TVITEMEXW ti;
				ti.hItem = parent;
				ti.mask = TVIF_PARAM;
				LRESULT const got = SendMessageW(m_tree, TVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&ti));
				file_info const& parent_fi = *reinterpret_cast<file_info*>(ti.lParam);
				int const idx = static_cast<int>(&tmp_fi - parent_fi.m_sub_file_infos.data());
				string const* const my_name = parent_fi.m_import_table.m_dlls[idx].m_dll_name;
				m_mo.m_tmpw.resize(my_name->m_len);
				std::transform(my_name->m_str, my_name->m_str + my_name->m_len, m_mo.m_tmpw.begin(), [](char const& e) -> wchar_t { return static_cast<wchar_t>(e); });
				std::wstring& tmp = m_tmp_strings[m_tmp_string_idx++ % m_tmp_strings.size()];
				tmp.clear();
				tmp.append(m_mo.m_tmpw).append(L" (").append(fi.m_file_path->m_str).append(L")");
				di.item.pszText = const_cast<wchar_t*>(tmp.c_str());
			}
			else
			{
				std::wstring& tmp = m_tmp_strings[m_tmp_string_idx++ % m_tmp_strings.size()];
				tmp.clear();
				tmp.append(find_file_name(fi.m_file_path->m_str, fi.m_file_path->m_len)).append(L" (").append(fi.m_file_path->m_str).append(L")");
				di.item.pszText = const_cast<wchar_t*>(tmp.c_str());
			}
		}
	}
	else if(nmhdr.code == TVN_SELCHANGEDW)
	{
		NMTREEVIEWW& nm = reinterpret_cast<NMTREEVIEWW&>(nmhdr);
		LRESULT const redr_off_1 = SendMessageW(m_import_list, WM_SETREDRAW, FALSE, 0);
		LRESULT const deleted_1 = SendMessageW(m_import_list, LVM_DELETEALLITEMS, 0, 0);
		HTREEITEM const parent = reinterpret_cast<HTREEITEM>(SendMessageW(m_tree, TVM_GETNEXTITEM, TVGN_PARENT, reinterpret_cast<LPARAM>(nm.itemNew.hItem)));
		if(parent)
		{
			TVITEMEXW ti;
			ti.hItem = parent;
			ti.mask = TVIF_PARAM;
			LRESULT const got = SendMessageW(m_tree, TVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&ti));
			file_info const& parent_fi = *reinterpret_cast<file_info*>(ti.lParam);
			file_info const& fi = *reinterpret_cast<file_info*>(nm.itemNew.lParam);
			int const idx = static_cast<int>(&fi - parent_fi.m_sub_file_infos.data());
			LRESULT const set_size = SendMessageW(m_import_list, LVM_SETITEMCOUNT, parent_fi.m_import_table.m_dlls[idx].m_entries.size(), 0);
			for(int row = 0; row != static_cast<int>(parent_fi.m_import_table.m_dlls[idx].m_entries.size()); ++row)
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
				lv.lParam = reinterpret_cast<LPARAM>(&parent_fi.m_import_table.m_dlls[idx].m_entries[row]);
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
		LRESULT const redr_on_1 = SendMessageW(m_import_list, WM_SETREDRAW, TRUE, 0);
		LRESULT const redr_off_2 = SendMessageW(m_export_list, WM_SETREDRAW, FALSE, 0);
		LRESULT const deleted_2 = SendMessageW(m_export_list, LVM_DELETEALLITEMS, 0, 0);
		file_info const& tmp_fi = *reinterpret_cast<file_info*>(nm.itemNew.lParam);
		file_info const& fi = tmp_fi.m_orig_instance ? *tmp_fi.m_orig_instance : tmp_fi;
		LRESULT const set_size = SendMessageW(m_export_list, LVM_SETITEMCOUNT, fi.m_export_table.m_export_address_table.size(), 0);
		for(int row = 0; row != fi.m_export_table.m_export_address_table.size(); ++row)
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
			lv.lParam = reinterpret_cast<LPARAM>(& fi.m_export_table.m_export_address_table[row]);
			lv.iIndent = 0;
			lv.iGroupId = 0;
			lv.cColumns = 0;
			lv.puColumns = nullptr;
			lv.piColFmt = nullptr;
			lv.iGroup = 0;
			LRESULT const added = SendMessageW(m_export_list, LVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&lv));
		}
		for(int i = 0; i != static_cast<int>(std::size(s_export_headers)); ++i)
		{
			LRESULT const auto_sized = SendMessageW(m_export_list, LVM_SETCOLUMNWIDTH, i, LVSCW_AUTOSIZE);
		}
		LRESULT const redr_on_2 = SendMessageW(m_export_list, WM_SETREDRAW, TRUE, 0);
	}
}

void main_window::on_import_notify(NMHDR& nmhdr)
{
	if(nmhdr.code == LVN_GETDISPINFOW)
	{
		NMLVDISPINFOW& nm = reinterpret_cast<NMLVDISPINFOW&>(nmhdr);
		if((nm.item.mask | LVIF_TEXT) != 0)
		{
			pe_import_entry const& import_entry = *reinterpret_cast<pe_import_entry*>(nm.item.lParam);
			int const row = nm.item.iItem;
			int const col = nm.item.iSubItem;
			e_import_column const ecol = static_cast<e_import_column>(col);
			switch(ecol)
			{
				case e_import_column::e_type:
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
				case e_import_column::e_ordinal:
				{
					if(import_entry.m_is_ordinal)
					{
						static_assert(sizeof(std::uint16_t) == sizeof(unsigned short int), "");
						std::array<wchar_t, 32> buff;
						int const formatted = std::swprintf(buff.data(), buff.size(), L"%hu (0x%04hx)", static_cast<unsigned short int>(import_entry.m_ordinal_or_hint), static_cast<unsigned short int>(import_entry.m_ordinal_or_hint));
						std::wstring& tmpstr = m_tmp_strings[m_tmp_string_idx++ % m_tmp_strings.size()];
						tmpstr.assign(buff.data(), buff.data() + formatted);
						nm.item.pszText = const_cast<wchar_t*>(tmpstr.c_str());
					}
					else
					{
						nm.item.pszText = const_cast<wchar_t*>(s_import_ordinal_na);
					}
				}
				break;
				case e_import_column::e_hint:
				{
					if(import_entry.m_is_ordinal)
					{
						nm.item.pszText = const_cast<wchar_t*>(s_import_hint_na);
					}
					else
					{
						static_assert(sizeof(std::uint16_t) == sizeof(unsigned short int), "");
						std::array<wchar_t, 32> buff;
						int const formatted = std::swprintf(buff.data(), buff.size(), L"%hu (0x%04hx)", static_cast<unsigned short int>(import_entry.m_ordinal_or_hint), static_cast<unsigned short int>(import_entry.m_ordinal_or_hint));
						std::wstring& tmpstr = m_tmp_strings[m_tmp_string_idx++ % m_tmp_strings.size()];
						tmpstr.assign(buff.data(), buff.data() + formatted);
						nm.item.pszText = const_cast<wchar_t*>(tmpstr.c_str());
					}
				}
				break;
				case e_import_column::e_name:
				{
					if(import_entry.m_is_ordinal)
					{
						nm.item.pszText = const_cast<wchar_t*>(s_import_name_na);
					}
					else
					{
						std::wstring& tmpstr = m_tmp_strings[m_tmp_string_idx++ % m_tmp_strings.size()];
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

void main_window::on_export_notify(NMHDR& nmhdr)
{
	if(nmhdr.code == LVN_GETDISPINFOW)
	{
		NMLVDISPINFOW& nm = reinterpret_cast<NMLVDISPINFOW&>(nmhdr);
		if((nm.item.mask | LVIF_TEXT) != 0)
		{
			pe_export_address_entry const& export_entry = *reinterpret_cast<pe_export_address_entry*>(nm.item.lParam);
			int const row = nm.item.iItem;
			int const col = nm.item.iSubItem;
			e_export_column const ecol = static_cast<e_export_column>(col);
			switch(ecol)
			{
				case e_export_column::e_type:
				{
					if(export_entry.m_is_rva)
					{
						nm.item.pszText = const_cast<wchar_t*>(s_export_type_true);
					}
					else
					{
						nm.item.pszText = const_cast<wchar_t*>(s_export_type_false);
					}
				}
				break;
				case e_export_column::e_ordinal:
				{
					static_assert(sizeof(std::uint16_t) == sizeof(unsigned short int), "");
					std::array<wchar_t, 32> buff;
					int const formatted = std::swprintf(buff.data(), buff.size(), L"%hu (0x%04hx)", static_cast<unsigned short int>(export_entry.m_ordinal), static_cast<unsigned short int>(export_entry.m_ordinal));
					std::wstring& tmpstr = m_tmp_strings[m_tmp_string_idx++ % m_tmp_strings.size()];
					tmpstr.assign(buff.data(), buff.data() + formatted);
					nm.item.pszText = const_cast<wchar_t*>(tmpstr.c_str());
				}
				break;
				case e_export_column::e_hint:
				{
					if(export_entry.m_name.empty())
					{
						nm.item.pszText = const_cast<wchar_t*>(s_export_hint_na);
					}
					else
					{
						static_assert(sizeof(std::uint16_t) == sizeof(unsigned short int), "");
						std::array<wchar_t, 32> buff;
						int const formatted = std::swprintf(buff.data(), buff.size(), L"%hu (0x%04hx)", static_cast<unsigned short int>(export_entry.m_hint), static_cast<unsigned short int>(export_entry.m_hint));
						std::wstring& tmpstr = m_tmp_strings[m_tmp_string_idx++ % m_tmp_strings.size()];
						tmpstr.assign(buff.data(), buff.data() + formatted);
						nm.item.pszText = const_cast<wchar_t*>(tmpstr.c_str());
					}
				}
				break;
				case e_export_column::e_name:
				{
					if(export_entry.m_name.empty())
					{
						nm.item.pszText = const_cast<wchar_t*>(s_export_name_na);
					}
					else
					{
						std::wstring& tmpstr = m_tmp_strings[m_tmp_string_idx++ % m_tmp_strings.size()];
						tmpstr.resize(export_entry.m_name.size());
						std::transform(cbegin(export_entry.m_name), cend(export_entry.m_name), begin(tmpstr), [](char const& e) -> wchar_t { return static_cast<wchar_t>(e); });
						nm.item.pszText = const_cast<wchar_t*>(tmpstr.c_str());
					}
				}
				break;
				case e_export_column::e_entry_point:
				{
					if(export_entry.m_is_rva)
					{
						static_assert(sizeof(std::uint32_t) == sizeof(unsigned int), "");
						std::array<wchar_t, 32> buff;
						int const formatted = std::swprintf(buff.data(), buff.size(), L"0x%08x", static_cast<unsigned int>(export_entry.m_rva));
						std::wstring& tmpstr = m_tmp_strings[m_tmp_string_idx++ % m_tmp_strings.size()];
						tmpstr.assign(buff.data(), buff.data() + formatted);
						nm.item.pszText = const_cast<wchar_t*>(tmpstr.c_str());
					}
					else
					{
						std::wstring& tmpstr = m_tmp_strings[m_tmp_string_idx++ % m_tmp_strings.size()];
						tmpstr.resize(export_entry.m_forwarder.size());
						std::transform(cbegin(export_entry.m_forwarder), cend(export_entry.m_forwarder), begin(tmpstr), [](char const& e) -> wchar_t { return static_cast<wchar_t>(e); });
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

LRESULT main_window::on_wm_dropfiles(WPARAM wparam, LPARAM lparam)
{
	HDROP const hdrop = reinterpret_cast<HDROP>(wparam);
	struct drop_deleter{ void operator()(void* const& obj){ DragFinish(reinterpret_cast<HDROP>(obj)); } };
	std::unique_ptr<void, drop_deleter> const sp_drop(hdrop);
	UINT const queried_1 = DragQueryFileW(hdrop, 0xFFFFFFFF, nullptr, 0);
	if(queried_1 != 1)
	{
		return DefWindowProcW(m_hwnd, WM_DROPFILES, wparam, lparam);
	}
	std::array<wchar_t, 32 * 1024> buff;
	UINT const queried_2 = DragQueryFileW(hdrop, 0, buff.data(), static_cast<int>(buff.size()));
	open_file(buff.data());
	return DefWindowProcW(m_hwnd, WM_DROPFILES, wparam, lparam);
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

void main_window::open_file(wchar_t const* const file_path)
{
	main_type mo;
	try
	{
		mo = process(file_path);
	}
	catch(wchar_t const* const ex)
	{
		int const msgbox = MessageBoxW(m_hwnd, ex, s_msg_error, MB_OK | MB_ICONERROR);
		return;
	}
	refresh(std::move(mo));
}

void main_window::refresh(main_type&& mo)
{
	LRESULT const redr_off_1 = SendMessageW(m_tree, WM_SETREDRAW, FALSE, 0);
	LRESULT const redr_off_2 = SendMessageW(m_import_list, WM_SETREDRAW, FALSE, 0);
	LRESULT const redr_off_3 = SendMessageW(m_export_list, WM_SETREDRAW, FALSE, 0);

	LRESULT const deleted_1 = SendMessageW(m_tree, TVM_DELETEITEM, 0, reinterpret_cast<LPARAM>(TVI_ROOT));
	LRESULT const deleted_2 = SendMessageW(m_import_list, LVM_DELETEALLITEMS, 0, 0);
	LRESULT const deleted_3 = SendMessageW(m_export_list, LVM_DELETEALLITEMS, 0, 0);

	m_mo = std::move(mo);

	TVINSERTSTRUCTW tvi;
	tvi.hParent = TVI_ROOT;
	tvi.hInsertAfter = TVI_ROOT;
	tvi.itemex.mask = TVIF_TEXT | TVIF_PARAM;
	tvi.itemex.hItem = nullptr;
	tvi.itemex.state = 0;
	tvi.itemex.stateMask = 0;
	tvi.itemex.pszText = LPSTR_TEXTCALLBACK;
	tvi.itemex.cchTextMax = 0;
	tvi.itemex.iImage = 0;
	tvi.itemex.iSelectedImage = 0;
	tvi.itemex.cChildren = 0;
	tvi.itemex.lParam = reinterpret_cast<LPARAM>(&m_mo.m_fi.m_sub_file_infos[0]);
	tvi.itemex.iIntegral = 0;
	tvi.itemex.uStateEx = 0;
	tvi.itemex.hwnd = nullptr;
	tvi.itemex.iExpandedImage = 0;
	tvi.itemex.iReserved = 0;
	HTREEITEM const hroot = reinterpret_cast<HTREEITEM>(SendMessageW(m_tree, TVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&tvi)));
	refresh_view_recursive(m_mo.m_fi.m_sub_file_infos[0], hroot);
	LRESULT const expanded = SendMessageW(m_tree, TVM_EXPAND, TVE_EXPAND, reinterpret_cast<LPARAM>(hroot));
	LRESULT const selected = SendMessageW(m_tree, TVM_SELECTITEM, TVGN_CARET, reinterpret_cast<LPARAM>(hroot));

	LRESULT const redr_on_1 = SendMessageW(m_tree, WM_SETREDRAW, TRUE, 0);
	LRESULT const redr_on_2 = SendMessageW(m_import_list, WM_SETREDRAW, TRUE, 0);
	LRESULT const redr_on_3 = SendMessageW(m_export_list, WM_SETREDRAW, TRUE, 0);
}

void main_window::refresh_view_recursive(file_info const& parent_fi, HTREEITEM const& parent_ti)
{
	for(auto const& fi : parent_fi.m_sub_file_infos)
	{
		TVINSERTSTRUCTW tvi;
		tvi.hParent = parent_ti;
		tvi.hInsertAfter = TVI_LAST;
		tvi.itemex.mask = TVIF_TEXT | TVIF_PARAM;
		tvi.itemex.hItem = nullptr;
		tvi.itemex.state = 0;
		tvi.itemex.stateMask = 0;
		tvi.itemex.pszText = LPSTR_TEXTCALLBACK;
		tvi.itemex.cchTextMax = 0;
		tvi.itemex.iImage = 0;
		tvi.itemex.iSelectedImage = 0;
		tvi.itemex.cChildren = 0;
		tvi.itemex.lParam = reinterpret_cast<LPARAM>(&fi);
		tvi.itemex.iIntegral = 0;
		tvi.itemex.uStateEx = 0;
		tvi.itemex.hwnd = nullptr;
		tvi.itemex.iExpandedImage = 0;
		tvi.itemex.iReserved = 0;
		HTREEITEM const ti = reinterpret_cast<HTREEITEM>(SendMessageW(m_tree, TVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&tvi)));
		refresh_view_recursive(fi, ti);
	}
}

ATOM main_window::m_s_class;
