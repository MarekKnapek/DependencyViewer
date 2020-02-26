#include "export_window_impl.h"

#include "common_controls.h"
#include "list_view_base.h"
#include "main.h"
#include "processor.h"

#include "../nogui/cassert_my.h"
#include "../nogui/int_to_string.h"
#include "../nogui/pe_getters_export.h"
#include "../nogui/scope_exit.h"

#include "../res/resources.h"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <numeric>
#include <tuple>

#include "../nogui/windows_my.h"

#include <commctrl.h>


enum class e_export_menu_id___2 : std::uint16_t
{
	e_matching,
};
static constexpr wchar_t const s_export_menu_orig_str___2[] = L"&Highlight Matching Import Function\tCtrl+M";

enum class e_export_column___2 : std::uint16_t
{
	e_e,
	e_type,
	e_ordinal,
	e_hint,
	e_name,
	e_entry_point,
};
static constexpr wchar_t const* const s_export_headers___2[] =
{
	L"E",
	L"type",
	L"ordinal",
	L"hint",
	L"name",
	L"entry point",
};
static constexpr wchar_t const s_export_type_true___2[] = L"address";
static constexpr wchar_t const s_export_type_false___2[] = L"forwarder";
static constexpr wchar_t const s_export_hint_na___2[] = L"N/A";
static constexpr wchar_t const s_export_name_processing___2[] = L"Processing...";
static constexpr wchar_t const s_export_name_na___2[] = L"N/A";
static constexpr wchar_t const s_export_name_undecorating___2[] = L"Undecorating...";


ATOM export_window_impl::g_class;
int export_window_impl::g_column_type_max_width;


export_window_impl::export_window_impl(HWND const& self) :
	m_self(self),
	m_list_view(),
	m_fi(),
	m_undecorate(),
	m_sort_col(0xFF),
	m_sort(),
	m_string_converter(),
	m_context_menu(),
	m_cmd_matching_fn(),
	m_cmd_matching_ctx()
{
	assert(self != nullptr);

	DWORD const ex_style = WS_EX_CLIENTEDGE;
	wchar_t const* const class_name = WC_LISTVIEWW;
	wchar_t const* const window_name = nullptr;
	DWORD const style = (WS_VISIBLE | WS_CHILD | WS_TABSTOP) | (LVS_REPORT | LVS_SHOWSELALWAYS | LVS_OWNERDATA);
	int const x_pos = 0;
	int const y_pos = 0;
	int const width = 0;
	int const height = 0;
	HWND const parent = m_self;
	HMENU const menu = nullptr;
	HINSTANCE const instance = get_instance();
	LPVOID const param = nullptr;
	m_list_view = CreateWindowExW(ex_style, class_name, window_name, style, x_pos, y_pos, width, height, parent, menu, instance, param);
	assert(m_list_view != nullptr);
	unsigned const extended_lv_styles = LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP | LVS_EX_DOUBLEBUFFER;
	LRESULT const prev_style = SendMessageW(m_list_view, LVM_SETEXTENDEDLISTVIEWSTYLE, extended_lv_styles, extended_lv_styles);
	for(int i = 0; i != static_cast<int>(std::size(s_export_headers___2)); ++i)
	{
		LVCOLUMNW cl;
		cl.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		cl.fmt = LVCFMT_LEFT;
		cl.cx = 200;
		cl.pszText = const_cast<LPWSTR>(s_export_headers___2[i]);
		cl.cchTextMax = 0;
		cl.iSubItem = i;
		cl.iImage = 0;
		cl.iOrder = i;
		cl.cxMin = 0;
		cl.cxDefault = 0;
		cl.cxIdeal = 0;
		auto const inserted = SendMessageW(m_list_view, LVM_INSERTCOLUMNW, i, reinterpret_cast<LPARAM>(&cl));
		assert(inserted != -1);
		assert(inserted == i);
	}
	HIMAGELIST const img_list = common_controls::ImageList_LoadImageW(get_instance(), MAKEINTRESOURCEW(s_res_icons_import_export), 30, 0, CLR_DEFAULT, IMAGE_BITMAP, LR_DEFAULTCOLOR);
	assert(img_list);
	LRESULT const prev_imgs = SendMessageW(m_list_view, LVM_SETIMAGELIST, LVSIL_SMALL, reinterpret_cast<LPARAM>(img_list));

	refresh();
}

export_window_impl::~export_window_impl()
{
}

void export_window_impl::init()
{
	register_class();
}

void export_window_impl::deinit()
{
	unregister_class();
}

wchar_t const* export_window_impl::get_class_atom()
{
	assert(g_class != 0);
	return reinterpret_cast<wchar_t const*>(g_class);
}

void export_window_impl::register_class()
{
	WNDCLASSEXW wc{};
	wc.cbSize = sizeof(wc);
	wc.style = 0;
	wc.lpfnWndProc = &class_proc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = get_instance();
	wc.hIcon = nullptr;
	wc.hCursor = LoadCursorW(nullptr, reinterpret_cast<wchar_t const*>(IDC_ARROW));
	wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = L"export_window";
	wc.hIconSm = nullptr;

	assert(g_class == 0);
	g_class = RegisterClassExW(&wc);
	assert(g_class != 0);
}

void export_window_impl::unregister_class()
{
	assert(g_class != 0);
	BOOL const unregistered = UnregisterClassW(reinterpret_cast<wchar_t const*>(g_class), get_instance());
	assert(unregistered != 0);
	g_class = 0;
}

LRESULT CALLBACK export_window_impl::class_proc(HWND const hwnd, UINT const msg, WPARAM const wparam, LPARAM const lparam)
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
			export_window_impl* const self = reinterpret_cast<export_window_impl*>(self_ptr);
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

LRESULT export_window_impl::on_wm_create(HWND const& hwnd, WPARAM const& wparam, LPARAM const& lparam)
{
	export_window_impl* const self = new export_window_impl{hwnd};
	assert((SetLastError(0), true));
	LONG_PTR const prev = SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
	assert(prev == 0 && GetLastError() == 0);

	LRESULT const ret = DefWindowProcW(hwnd, WM_CREATE, wparam, lparam);
	return ret;
}

LRESULT export_window_impl::on_wm_destroy(HWND const& hwnd, WPARAM const& wparam, LPARAM const& lparam)
{
	LONG_PTR const prev = SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
	assert(prev != 0);
	export_window_impl* const self = reinterpret_cast<export_window_impl*>(prev);
	delete self;

	LRESULT const ret = DefWindowProcW(hwnd, WM_DESTROY, wparam, lparam);
	return ret;
}

LRESULT export_window_impl::on_message(UINT const& msg, WPARAM const& wparam, LPARAM const& lparam)
{
	switch(msg)
	{
		case WM_SIZE:
		{
			LRESULT const ret = on_wm_size(wparam, lparam);
			return ret;
		}
		break;
		case WM_NOTIFY:
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
		case WM_COMMAND:
		{
			LRESULT const ret = on_wm_command(wparam, lparam);
			return ret;
		}
		break;
		case static_cast<std::uint32_t>(export_window::wm::wm_repaint):
		{
			LRESULT const ret = on_wm_repaint(wparam, lparam);
			return ret;
		}
		break;
		case static_cast<std::uint32_t>(export_window::wm::wm_setfi):
		{
			LRESULT const ret = on_wm_setfi(wparam, lparam);
			return ret;
		}
		break;
		case static_cast<std::uint32_t>(export_window::wm::wm_setundecorate):
		{
			LRESULT const ret = on_wm_setundecorate(wparam, lparam);
			return ret;
		}
		break;
		case static_cast<std::uint32_t>(export_window::wm::wm_setcmdmatching):
		{
			LRESULT const ret = on_wm_setcmdmatching(wparam, lparam);
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

LRESULT export_window_impl::on_wm_size(WPARAM const& wparam, LPARAM const& lparam)
{
	RECT rect;
	BOOL const got = GetClientRect(m_self, &rect);
	assert(got != 0);
	assert(rect.left == 0 && rect.top == 0);
	BOOL const moved = MoveWindow(m_list_view, 0, 0, rect.right, rect.bottom, TRUE);
	assert(moved != 0);

	LRESULT const ret = DefWindowProcW(m_self, WM_SIZE, wparam, lparam);
	return ret;
}

LRESULT export_window_impl::on_wm_notify(WPARAM const& wparam, LPARAM const& lparam)
{
	NMHDR& nmhdr = *reinterpret_cast<NMHDR*>(lparam);
	if(nmhdr.hwndFrom == m_list_view)
	{
		switch(nmhdr.code)
		{
			case LVN_GETDISPINFOW:
			{
				on_getdispinfow(nmhdr);
			}
			break;
			case LVN_COLUMNCLICK:
			{
				on_columnclick(nmhdr);
			}
			break;
		}
	}

	LRESULT const ret = DefWindowProcW(m_self, WM_NOTIFY, wparam, lparam);
	return ret;
}

LRESULT export_window_impl::on_wm_contextmenu(WPARAM const& wparam, LPARAM const& lparam)
{
	auto const fn_on_wm_contextmenu = [&]()
	{
		int item_idx_;
		POINT screen_pos;
		bool const context_found = list_view_base::get_context_menu(&m_list_view, &lparam, &m_sort, &item_idx_, &screen_pos);
		if(!context_found)
		{
			return;
		}
		assert(item_idx_ >= 0 && item_idx_ <= 0xFFFF);
		std::uint16_t const item_idx = static_cast<std::uint16_t>(item_idx_);

		if(!m_context_menu)
		{
			m_context_menu = create_context_menu();
		}
		HMENU const menu = reinterpret_cast<HMENU>(m_context_menu.get());
		auto const matching_available = command_matching_available(item_idx, nullptr);
		BOOL const prev = EnableMenuItem(menu, static_cast<std::uint16_t>(e_export_menu_id___2::e_matching), MF_BYCOMMAND | (matching_available ? MF_ENABLED : (MF_GRAYED | MF_DISABLED)));
		assert(prev != -1 && (prev == MF_ENABLED || prev == (MF_GRAYED | MF_DISABLED)));
		BOOL const tracked = TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON | TPM_NOANIMATION, screen_pos.x, screen_pos.y, 0, m_self, nullptr);
		assert(tracked != 0);
	};
	fn_on_wm_contextmenu();

	LRESULT const ret = DefWindowProcW(m_self, WM_CONTEXTMENU, wparam, lparam);
	return ret;
}

LRESULT export_window_impl::on_wm_command(WPARAM const& wparam, LPARAM const& lparam)
{
	LRESULT const ret = DefWindowProcW(m_self, WM_COMMAND, wparam, lparam);
	return ret;
}

LRESULT export_window_impl::on_wm_repaint(WPARAM const& wparam, LPARAM const& lparam)
{
	repaint();

	UINT const msg = static_cast<std::uint32_t>(export_window::wm::wm_repaint);
	LRESULT const ret = DefWindowProcW(m_self, msg, wparam, lparam);
	return ret;
}

LRESULT export_window_impl::on_wm_setfi(WPARAM const& wparam, LPARAM const& lparam)
{
	file_info const* const fi = reinterpret_cast<file_info const*>(lparam);
	m_fi = fi;
	refresh();

	UINT const msg = static_cast<std::uint32_t>(export_window::wm::wm_setfi);
	LRESULT const ret = DefWindowProcW(m_self, msg, wparam, lparam);
	return ret;
}

LRESULT export_window_impl::on_wm_setundecorate(WPARAM const& wparam, LPARAM const& lparam)
{
	assert(wparam == 0 || wparam == 1);
	bool const undecorate = wparam != 0;
	m_undecorate = undecorate;
	repaint();

	UINT const msg = static_cast<std::uint32_t>(export_window::wm::wm_setundecorate);
	LRESULT const ret = DefWindowProcW(m_self, msg, wparam, lparam);
	return ret;
}

LRESULT export_window_impl::on_wm_setcmdmatching(WPARAM const& wparam, LPARAM const& lparam)
{
	static_assert(sizeof(wparam) == sizeof(export_window::cmd_matching_fn_t), "");
	static_assert(sizeof(lparam) == sizeof(export_window::cmd_matching_ctx_t), "");
	auto const cmd_matching_fn = reinterpret_cast<export_window::cmd_matching_fn_t>(wparam);
	auto const cmd_matching_ctx = reinterpret_cast<export_window::cmd_matching_ctx_t>(lparam);
	m_cmd_matching_fn = cmd_matching_fn;
	m_cmd_matching_ctx = cmd_matching_ctx;

	UINT const msg = static_cast<std::uint32_t>(export_window::wm::wm_setcmdmatching);
	LRESULT const ret = DefWindowProcW(m_self, msg, wparam, lparam);
	return ret;
}

void export_window_impl::on_getdispinfow(NMHDR& nmhdr)
{
	NMLVDISPINFOW& nm = reinterpret_cast<NMLVDISPINFOW&>(nmhdr);
	file_info const* const tmp_fi = m_fi;
	if(!tmp_fi)
	{
		return;
	}
	file_info const* const fi = tmp_fi->m_orig_instance ? tmp_fi->m_orig_instance : tmp_fi;
	assert(fi);
	pe_export_table_info const& eti = fi->m_export_table;
	int const row = nm.item.iItem;
	assert(row >= 0 && row <= 0xFFFF);
	std::uint16_t const exp_idx = static_cast<std::uint16_t>(row);
	if((nm.item.mask & LVIF_TEXT) != 0)
	{
		int const col_ = nm.item.iSubItem;
		assert(col_ >= static_cast<std::uint16_t>(e_export_column___2::e_e));
		assert(col_ <= static_cast<std::uint16_t>(e_export_column___2::e_entry_point));
		e_export_column___2 const col = static_cast<e_export_column___2>(col_);
		switch(col)
		{
			case e_export_column___2::e_e:
			{
				nm.item.pszText = const_cast<wchar_t*>(L"");
			}
			break;
			case e_export_column___2::e_type:
			{
				nm.item.pszText = const_cast<wchar_t*>(get_col_type(eti, exp_idx));
			}
			break;
			case e_export_column___2::e_ordinal:
			{
				nm.item.pszText = const_cast<wchar_t*>(get_col_ordinal(eti, exp_idx));
			}
			break;
			case e_export_column___2::e_hint:
			{
				nm.item.pszText = const_cast<wchar_t*>(get_col_hint(eti, exp_idx));
			}
			break;
			case e_export_column___2::e_name:
			{
				nm.item.pszText = const_cast<wchar_t*>(get_col_name(eti, exp_idx));
			}
			break;
			case e_export_column___2::e_entry_point:
			{
				nm.item.pszText = const_cast<wchar_t*>(get_col_address(eti, exp_idx));
			}
			break;
			default:
			{
				assert(false);
			}
			break;
		}
	}
	if((nm.item.mask & LVIF_IMAGE) != 0)
	{
		nm.item.iImage = get_col_icon(eti, tmp_fi, exp_idx);
	}
}

void export_window_impl::on_columnclick(NMHDR& nmhdr)
{
	NMLISTVIEW const& nmlv = reinterpret_cast<NMLISTVIEW&>(nmhdr);
	int const new_sort = list_view_base::on_columnclick(&nmlv, static_cast<int>(std::size(s_export_headers___2)), m_sort_col);
	assert(new_sort >= 0 && new_sort <= 0xFF);
	m_sort_col = static_cast<std::uint8_t>(new_sort);
	sort_view();
	list_view_base::refresh_headers(&m_list_view, static_cast<int>(std::size(s_export_headers___2)), m_sort_col);
	repaint();
}

wchar_t const* export_window_impl::get_col_type(pe_export_table_info const& eti, std::uint16_t const exp_idx)
{
	std::uint16_t const& exp_idx_sorted = m_sort.empty() ? exp_idx : m_sort[exp_idx];
	bool const is_rva = pe_get_export_type(eti, exp_idx_sorted);
	if(is_rva)
	{
		return s_export_type_true___2;
	}
	else
	{
		return s_export_type_false___2;
	}
}

wchar_t const* export_window_impl::get_col_ordinal(pe_export_table_info const& eti, std::uint16_t const exp_idx)
{
	std::uint16_t const& exp_idx_sorted = m_sort.empty() ? exp_idx : m_sort[exp_idx];
	std::uint16_t const ordinal = pe_get_export_ordinal(eti, exp_idx_sorted);
	wchar_t const* const ret = ordinal_to_string(ordinal, m_string_converter);
	return ret;
}

wchar_t const* export_window_impl::get_col_hint(pe_export_table_info const& eti, std::uint16_t const exp_idx)
{
	std::uint16_t const& exp_idx_sorted = m_sort.empty() ? exp_idx : m_sort[exp_idx];
	auto const hint_opt = pe_get_export_hint(eti, exp_idx_sorted);
	if(hint_opt.m_is_valid)
	{
		std::uint16_t const& hint = hint_opt.m_value;
		wchar_t const* const ret = ordinal_to_string(hint, m_string_converter);
		return ret;
	}
	else
	{
		return s_export_hint_na___2;
	}
}

wchar_t const* export_window_impl::get_col_name(pe_export_table_info const& eti, std::uint16_t const exp_idx)
{
	std::uint16_t const& exp_idx_sorted = m_sort.empty() ? exp_idx : m_sort[exp_idx];
	bool const undecorate = m_undecorate;
	if(undecorate)
	{
		string_handle const name = pe_get_export_name_undecorated(eti, exp_idx_sorted);
		if(!name.m_string)
		{
			return s_export_name_na___2;
		}
		else if(name.m_string == get_export_name_processing().m_string)
		{
			return s_export_name_processing___2;
		}
		else if(name.m_string == get_name_undecorating().m_string)
		{
			return s_export_name_undecorating___2;
		}
		else
		{
			wchar_t const* const ret = m_string_converter.convert(name);
			return ret;
		}
	}
	else
	{
		string_handle const name = pe_get_export_name(eti, exp_idx_sorted);
		if(!name.m_string)
		{
			return s_export_name_na___2;
		}
		else if(name.m_string == get_export_name_processing().m_string)
		{
			return s_export_name_processing___2;
		}
		else
		{
			wchar_t const* const ret = m_string_converter.convert(name);
			return ret;
		}
	}
}

wchar_t const* export_window_impl::get_col_address(pe_export_table_info const& eti, std::uint16_t const exp_idx)
{
	std::uint16_t const& exp_idx_sorted = m_sort.empty() ? exp_idx : m_sort[exp_idx];
	pe_rva_or_forwarder const entry_point = pe_get_export_entry_point(eti, exp_idx_sorted);
	bool const is_rva = pe_get_export_type(eti, exp_idx_sorted);
	if(is_rva)
	{
		wchar_t const* const ret = rva_to_string(entry_point.m_rva, m_string_converter);
		return ret;
	}
	else
	{
		wchar_t const* const ret = m_string_converter.convert(entry_point.m_forwarder);
		return ret;
	}
}

std::uint8_t export_window_impl::get_col_icon(pe_export_table_info const& eti, file_info const* const fi, std::uint16_t const exp_idx)
{
	std::uint16_t const& exp_idx_sorted = m_sort.empty() ? exp_idx : m_sort[exp_idx];
	std::uint8_t const img_idx = pe_get_export_icon_id(eti, fi->m_matched_imports, exp_idx_sorted);
	return img_idx;
}

smart_menu export_window_impl::create_context_menu()
{
	HMENU const menu = CreatePopupMenu();
	assert(menu);
	smart_menu menu_sp{menu};
	MENUITEMINFOW mi{};
	mi.cbSize = sizeof(mi);
	mi.fMask = MIIM_ID | MIIM_STRING | MIIM_FTYPE;
	mi.fType = MFT_STRING;
	mi.wID = static_cast<std::uint16_t>(e_export_menu_id___2::e_matching);
	mi.dwTypeData = const_cast<wchar_t*>(s_export_menu_orig_str___2);
	BOOL const inserted = InsertMenuItemW(menu, 0, TRUE, &mi);
	assert(inserted != 0);
	return menu_sp;
}

bool export_window_impl::command_matching_available(std::uint16_t const& item_idx, std::uint16_t* const out_item_idx)
{
	file_info const* const tmp_fi = m_fi;
	if(!tmp_fi)
	{
		return false;
	}
	if(tmp_fi->m_matched_imports == nullptr)
	{
		return false;
	}
	std::uint16_t const& matched = tmp_fi->m_matched_imports[item_idx];
	bool const available = matched != 0xFFFF;
	if(available && out_item_idx)
	{
		*out_item_idx = matched;
	}
	return available;
}

void export_window_impl::command_matching()
{
	int const sel = list_view_base::get_selection(&m_list_view);
	if(sel == -1)
	{
		return;
	}
	assert(sel >= 0 && sel <= 0xFFFF);
	std::uint16_t const line_idx = static_cast<std::uint16_t>(sel);
	std::uint16_t const item_idx = m_sort.empty() ? line_idx : m_sort[line_idx];

	std::uint16_t other_item_idx;
	bool const available = command_matching_available(item_idx, &other_item_idx);
	if(!available)
	{
		return;
	}

	if(!m_cmd_matching_fn)
	{
		return;
	}
	m_cmd_matching_fn(m_cmd_matching_ctx, other_item_idx);
}

void export_window_impl::refresh()
{
	LRESULT const redr_off = SendMessageW(m_list_view, WM_SETREDRAW, FALSE, 0);
	assert(redr_off == 0);
	auto const fn_redraw = mk::make_scope_exit([&]()
	{
		sort_view();

		LRESULT const auto_sized_e = SendMessageW(m_list_view, LVM_SETCOLUMNWIDTH, static_cast<int>(e_export_column___2::e_e), LVSCW_AUTOSIZE);
		assert(auto_sized_e == TRUE);
		LRESULT const type_sized = SendMessageW(m_list_view, LVM_SETCOLUMNWIDTH, static_cast<int>(e_export_column___2::e_type), get_column_type_max_width());
		assert(type_sized == TRUE);
		LRESULT const ordinal_sized = SendMessageW(m_list_view, LVM_SETCOLUMNWIDTH, static_cast<int>(e_export_column___2::e_ordinal), list_view_base::get_column_uint16_max_width(&m_list_view));
		assert(ordinal_sized == TRUE);
		LRESULT const hint_sized = SendMessageW(m_list_view, LVM_SETCOLUMNWIDTH, static_cast<int>(e_export_column___2::e_hint), list_view_base::get_column_uint16_max_width(&m_list_view));
		assert(hint_sized == TRUE);

		LRESULT const redr_on = SendMessageW(m_list_view, WM_SETREDRAW, TRUE, 0);
		assert(redr_on == 0);
		repaint();
	});

	LRESULT const deleted = SendMessageW(m_list_view, LVM_DELETEALLITEMS, 0, 0);
	assert(deleted == TRUE);
	file_info const* const tmp_fi = m_fi;
	if(!tmp_fi)
	{
		return;
	}
	file_info const* const fi = tmp_fi->m_orig_instance ? tmp_fi->m_orig_instance : tmp_fi;
	assert(fi);
	std::uint16_t const& count = fi->m_export_table.m_count;
	LRESULT const set_size = SendMessageW(m_list_view, LVM_SETITEMCOUNT, count, 0);
	assert(set_size != 0);
}

void export_window_impl::repaint()
{
	BOOL const redrawn = RedrawWindow(m_list_view, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_FRAME);
	assert(redrawn != 0);
}

int export_window_impl::get_column_type_max_width()
{
	if(g_column_type_max_width != 0)
	{
		return g_column_type_max_width;
	}
	static constexpr std::pair<wchar_t const* const, int const> const s_strings[] =
	{
		{s_export_type_true___2, static_cast<int>(std::size(s_export_type_true___2)) - 1},
		{s_export_type_false___2, static_cast<int>(std::size(s_export_type_false___2)) - 1},
	};
	g_column_type_max_width = list_view_base::get_column_max_width(&m_list_view, &s_strings, static_cast<int>(std::size(s_strings)));
	return g_column_type_max_width;
}

void export_window_impl::sort_view()
{
	m_sort.clear();
	std::uint8_t const cur_sort_raw = m_sort_col;
	if(cur_sort_raw == 0xFF)
	{
		return;
	}
	bool const asc = (cur_sort_raw & (1u << 7u)) == 0u;
	std::uint8_t const col_ = cur_sort_raw &~ (1u << 7u);
	file_info const* const tmp_fi = m_fi;
	if(!tmp_fi)
	{
		return;
	}
	file_info const* const fi = tmp_fi->m_orig_instance ? tmp_fi->m_orig_instance : tmp_fi;
	assert(fi);
	pe_export_table_info const& eti = fi->m_export_table;
	std::uint16_t const n_items = eti.m_count;
	assert(n_items <= 0xFFFF / 2);
	m_sort.resize(n_items * 2);
	std::iota(m_sort.begin(), m_sort.begin() + n_items, std::uint16_t{0});
	std::uint16_t* const sort = m_sort.data();
	assert(col_ >= static_cast<std::uint16_t>(e_export_column___2::e_e));
	assert(col_ <= static_cast<std::uint16_t>(e_export_column___2::e_entry_point));
	e_export_column___2 const col = static_cast<e_export_column___2>(col_);
	switch(col)
	{
		case e_export_column___2::e_e:
		{
			auto const fn_compare_icon = [&](std::uint16_t const a, std::uint16_t const b) -> bool
			{
				std::uint8_t const a2 = pe_get_export_icon_id(eti, tmp_fi->m_matched_imports, a);
				std::uint8_t const b2 = pe_get_export_icon_id(eti, tmp_fi->m_matched_imports, b);
				bool const ret = a2 < b2;
				return ret;
			};
			if(asc)
			{
				std::sort(sort, sort + n_items, [&](auto const& a, auto const& b){ return fn_compare_icon(a, b); });
			}
			else
			{
				std::sort(sort, sort + n_items, [&](auto const& a, auto const& b){ return fn_compare_icon(b, a); });
			}
		}
		break;
		case e_export_column___2::e_type:
		{
			auto const fn_compare_type = [&](std::uint16_t const a, std::uint16_t const b) -> bool
			{
				bool const a2 = pe_get_export_type(eti, a);
				bool const b2 = pe_get_export_type(eti, b);
				bool const ret = a2 < b2;
				return ret;
			};
			if(asc)
			{
				std::sort(sort, sort + n_items, [&](auto const& a, auto const& b){ return fn_compare_type(a, b); });
			}
			else
			{
				std::sort(sort, sort + n_items, [&](auto const& a, auto const& b){ return fn_compare_type(b, a); });
			}
		}
		break;
		case e_export_column___2::e_ordinal:
		{
			auto const fn_compare_ordinal = [&](std::uint16_t const a, std::uint16_t const b) -> bool
			{
				std::uint16_t const a2 = pe_get_export_ordinal(eti, a);
				std::uint16_t const b2 = pe_get_export_ordinal(eti, b);
				bool const ret = a2 < b2;
				return ret;
			};
			if(asc)
			{
				std::sort(sort, sort + n_items, [&](auto const& a, auto const& b){ return fn_compare_ordinal(a, b); });
			}
			else
			{
				std::sort(sort, sort + n_items, [&](auto const& a, auto const& b){ return fn_compare_ordinal(b, a); });
			}
		}
		break;
		case e_export_column___2::e_hint:
		{
			auto const fn_compare_hint = [&](std::uint16_t const a, std::uint16_t const b) -> bool
			{
				auto const a2 = pe_get_export_hint(eti, a);
				auto const b2 = pe_get_export_hint(eti, b);
				bool const ret = std::tie(a2.m_is_valid, a2.m_value) < std::tie(b2.m_is_valid, b2.m_value);
				return ret;
			};
			if(asc)
			{
				std::sort(sort, sort + n_items, [&](auto const& a, auto const& b){ return fn_compare_hint(a, b); });
			}
			else
			{
				std::sort(sort, sort + n_items, [&](auto const& a, auto const& b){ return fn_compare_hint(b, a); });
			}
		}
		break;
		case e_export_column___2::e_name:
		{
			auto const fn_compare_name = [&](std::uint16_t const a, std::uint16_t const b) -> bool
			{
				string_handle const a2 = pe_get_export_name(eti, a);
				string_handle const b2 = pe_get_export_name(eti, b);
				int const a3 = (a2.m_string != nullptr) ? 2 : (a2.m_string == get_export_name_processing().m_string ? 1 : 0);
				int const b3 = (b2.m_string != nullptr) ? 2 : (b2.m_string == get_export_name_processing().m_string ? 1 : 0);
				bool const ret = std::tie(a3, a2) < std::tie(b3, b2);
				return ret;
			};
			if(asc)
			{
				std::sort(sort, sort + n_items, [&](auto const& a, auto const& b){ return fn_compare_name(a, b); });
			}
			else
			{
				std::sort(sort, sort + n_items, [&](auto const& a, auto const& b){ return fn_compare_name(b, a); });
			}
		}
		break;
		case e_export_column___2::e_entry_point:
		{
			auto const fn_compare_entry_point = [&](std::uint16_t const a, std::uint16_t const b) -> bool
			{
				static constexpr char const s_empty_string_literal[] = "";
				static constexpr string const s_empty_string = string{s_empty_string_literal, 0};
				static constexpr string_handle const s_empty_string_handle = string_handle{&s_empty_string};
				bool const a2 = pe_get_export_type(eti, a);
				bool const b2 = pe_get_export_type(eti, b);
				pe_rva_or_forwarder const a3 = pe_get_export_entry_point(eti, a);
				pe_rva_or_forwarder const b3 = pe_get_export_entry_point(eti, b);
				std::uint32_t const a4 = a2 ? a3.m_rva : 0;
				std::uint32_t const b4 = b2 ? b3.m_rva : 0;
				string_handle const a5 = !a2 ? a3.m_forwarder : s_empty_string_handle;
				string_handle const b5 = !b2 ? b3.m_forwarder : s_empty_string_handle;
				bool const ret = std::tie(a2, a4, a5) < std::tie(b2, b4, b5);
				return ret;
			};
			if(asc)
			{
				std::sort(sort, sort + n_items, [&](auto const& a, auto const& b){ return fn_compare_entry_point(a, b); });
			}
			else
			{
				std::sort(sort, sort + n_items, [&](auto const& a, auto const& b){ return fn_compare_entry_point(b, a); });
			}
		}
		break;
		default:
		{
			assert(false);
		}
		break;
	}
	for(std::uint16_t i = 0; i != n_items; ++i)
	{
		m_sort[n_items + m_sort[i]] = i;
	}
}
