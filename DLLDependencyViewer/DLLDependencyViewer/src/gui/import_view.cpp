#include "import_view.h"

#include "constants.h"
#include "list_view_base.h"
#include "main.h"
#include "main_window.h"
#include "smart_dc.h"

#include "../nogui/array_bool.h"
#include "../nogui/int_to_string.h"
#include "../nogui/pe.h"
#include "../nogui/pe_getters_import.h"
#include "../nogui/scope_exit.h"

#include "../res/resources.h"

#include <algorithm>
#include <cassert>
#include <functional>
#include <iterator>
#include <numeric>
#include <tuple>

#include <commctrl.h>
#include <windowsx.h>


enum class e_import_menu_id : std::uint16_t
{
	e_matching = s_import_view_menu_min,
};
enum class e_import_column
{
	e_pi,
	e_type,
	e_ordinal,
	e_hint,
	e_name
};
static constexpr wchar_t const* const s_import_headers[] =
{
	L"PI",
	L"type",
	L"ordinal",
	L"hint",
	L"name"
};
static constexpr wchar_t const s_import_menu_orig_str[] = L"&Highlight Matching Export Function\tCtrl+M";
static constexpr wchar_t const s_import_type_true[] = L"ordinal";
static constexpr wchar_t const s_import_type_false[] = L"name";
static constexpr wchar_t const s_import_ordinal_na[] = L"N/A";
static constexpr wchar_t const s_import_hint_na[] = L"N/A";
static constexpr wchar_t const s_import_name_na[] = L"N/A";
static constexpr wchar_t const s_import_name_processing[] = L"Processing...";
static constexpr wchar_t const s_import_name_undecorating[] = L"Undecorating...";


static int g_import_type_column_max_width = 0;


struct string_helper_imp
{
	string_handle m_string;
};

inline bool operator<(string_helper_imp const& a, string_helper_imp const& b) noexcept
{
	auto const& aa = a.m_string;
	auto const& bb = b.m_string;
	auto const& aaa = aa.m_string;
	auto const& bbb = bb.m_string;
	if(!aaa && !bbb)
	{
		return false;
	}
	else if(aaa == get_export_name_processing().m_string && bbb == get_export_name_processing().m_string)
	{
		return false;
	}
	else
	{
		assert(aaa);
		assert(bbb);
		return aa < bb;
	}
}


import_view::import_view(HWND const parent, main_window& mw) :
	m_hwnd(CreateWindowExW(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE, WC_LISTVIEWW, nullptr, WS_VISIBLE | WS_CHILD | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_OWNERDATA, 0, 0, 0, 0, parent, nullptr, get_instance(), nullptr)),
	m_main_window(mw),
	m_menu(create_menu()),
	m_sort(),
	m_string_converter()
{
	static constexpr unsigned const extended_lv_styles = LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP | LVS_EX_DOUBLEBUFFER;
	LRESULT const set_import = SendMessageW(m_hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, extended_lv_styles, extended_lv_styles);
	for(int i = 0; i != static_cast<int>(std::size(s_import_headers)); ++i)
	{
		LVCOLUMNW cl;
		cl.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		cl.fmt = LVCFMT_LEFT;
		cl.cx = 200;
		cl.pszText = const_cast<LPWSTR>(s_import_headers[i]);
		cl.cchTextMax = 0;
		cl.iSubItem = i;
		cl.iImage = 0;
		cl.iOrder = i;
		cl.cxMin = 0;
		cl.cxDefault = 0;
		cl.cxIdeal = 0;
		auto const inserted = SendMessageW(m_hwnd, LVM_INSERTCOLUMNW, i, reinterpret_cast<LPARAM>(&cl));
		assert(inserted != -1 && inserted == i);
	}
	HIMAGELIST const img_list = ImageList_LoadImageW(get_instance(), MAKEINTRESOURCEW(s_res_icons_import_export), 30, 0, CLR_DEFAULT, IMAGE_BITMAP, LR_DEFAULTCOLOR);
	assert(img_list);
	LRESULT const img_list_set = SendMessageW(m_hwnd, LVM_SETIMAGELIST, LVSIL_SMALL, reinterpret_cast<LPARAM>(img_list));
}

import_view::~import_view()
{
}

HWND import_view::get_hwnd() const
{
	return m_hwnd;
}

void import_view::on_notify(NMHDR& nmhdr)
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

void import_view::on_getdispinfow(NMHDR& nmhdr)
{
	NMLVDISPINFOW& nm = reinterpret_cast<NMLVDISPINFOW&>(nmhdr);
	HWND const tree = m_main_window.m_tree_view.get_hwnd();
	HTREEITEM const selected = reinterpret_cast<HTREEITEM>(SendMessageW(tree, TVM_GETNEXTITEM, TVGN_CARET, 0));
	if(!selected)
	{
		return;
	}
	HTREEITEM const parent = reinterpret_cast<HTREEITEM>(SendMessageW(tree, TVM_GETNEXTITEM, TVGN_PARENT, reinterpret_cast<LPARAM>(selected)));
	if(!parent)
	{
		return;
	}
	TVITEMEXW ti;
	ti.hItem = parent;
	ti.mask = TVIF_PARAM;
	LRESULT const got_parent = SendMessageW(tree, TVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&ti));
	assert(got_parent == TRUE);
	file_info const& parent_fi = *reinterpret_cast<file_info*>(ti.lParam);
	ti.hItem = selected;
	ti.mask = TVIF_PARAM;
	LRESULT const got_selected = SendMessageW(tree, TVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&ti));
	assert(got_selected == TRUE);
	file_info const& fi_tmp = *reinterpret_cast<file_info*>(ti.lParam);
	file_info const& fi = fi_tmp.m_orig_instance ? *fi_tmp.m_orig_instance : fi_tmp;
	std::uint16_t const dll_idx = static_cast<std::uint16_t>(&fi_tmp - parent_fi.m_sub_file_infos.data());
	int const row = nm.item.iItem;
	int const col = nm.item.iSubItem;
	std::uint16_t const imp_idx = static_cast<std::uint16_t>(row);
	e_import_column const ecol = static_cast<e_import_column>(col);
	if((nm.item.mask | LVIF_TEXT) != 0)
	{
		switch(ecol)
		{
			case e_import_column::e_pi:
			{
				nm.item.pszText = const_cast<wchar_t*>(L"");
			}
			break;
			case e_import_column::e_type:
			{
				nm.item.pszText = const_cast<wchar_t*>(on_get_col_type(parent_fi.m_import_table, dll_idx, imp_idx));
			}
			break;
			case e_import_column::e_ordinal:
			{
				nm.item.pszText = const_cast<wchar_t*>(on_get_col_ordinal(parent_fi.m_import_table, dll_idx, imp_idx, fi));
			}
			break;
			case e_import_column::e_hint:
			{
				nm.item.pszText = const_cast<wchar_t*>(on_get_col_hint(parent_fi.m_import_table, dll_idx, imp_idx, fi));
			}
			break;
			case e_import_column::e_name:
			{
				nm.item.pszText = const_cast<wchar_t*>(on_get_col_name(parent_fi.m_import_table, dll_idx, imp_idx, fi));
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
		std::uint16_t const& imp_idx_sorted = m_sort.empty() ? imp_idx : m_sort[imp_idx];
		std::uint8_t const icon_idx = pe_get_import_icon_id(parent_fi.m_import_table, dll_idx, imp_idx_sorted);
		nm.item.iImage = icon_idx;
	}
}

void import_view::on_columnclick(NMHDR& nmhdr)
{
	NMLISTVIEW const& nmlv = reinterpret_cast<NMLISTVIEW&>(nmhdr);
	int const new_sort = list_view_base::on_columnclick(&nmlv, static_cast<int>(std::size(s_import_headers)), m_main_window.m_settings.m_import_sort);
	assert(new_sort <= 0xFF);
	m_main_window.m_settings.m_import_sort = static_cast<std::uint8_t>(new_sort);
	sort_view();
	refresh_headers();
}

void import_view::on_context_menu(LPARAM const lparam)
{
	POINT cursor_screen;
	std::uint16_t ith_line;
	if(lparam == LPARAM{-1})
	{
		LRESULT const sel = SendMessageW(m_hwnd, LVM_GETNEXTITEM, WPARAM{0} - 1, LVNI_SELECTED);
		if(sel == -1)
		{
			return;
		}
		RECT rect;
		rect.top = static_cast<int>(e_import_column::e_type);
		rect.left = LVIR_BOUNDS;
		LRESULT const got_rect = SendMessageW(m_hwnd, LVM_GETSUBITEMRECT, sel, reinterpret_cast<LPARAM>(&rect));
		if(got_rect == 0)
		{
			return;
		}
		cursor_screen.x = rect.left + (rect.right - rect.left) / 2;
		cursor_screen.y = rect.top + (rect.bottom - rect.top) / 2;
		BOOL const converted = ClientToScreen(m_hwnd, &cursor_screen);
		assert(converted != 0);
		assert(static_cast<std::size_t>(sel) <= 0xFFFF);
		ith_line = static_cast<std::uint16_t>(sel);
	}
	else
	{
		cursor_screen.x = GET_X_LPARAM(lparam);
		cursor_screen.y = GET_Y_LPARAM(lparam);
		POINT cursor_client = cursor_screen;
		BOOL const converted = ScreenToClient(m_hwnd, &cursor_client);
		assert(converted != 0);
		LVHITTESTINFO hti;
		hti.pt = cursor_client;
		hti.flags = LVHT_ONITEM;
		LPARAM const hit_tested = SendMessageW(m_hwnd, LVM_HITTEST, 0, reinterpret_cast<LPARAM>(&hti));
		if(hit_tested == -1)
		{
			return;
		}
		assert(hit_tested == hti.iItem);
		assert(hti.iItem <= 0xFFFF);
		ith_line = static_cast<std::uint16_t>(hti.iItem);
	}
	std::uint16_t const ith_import = m_sort.empty() ? ith_line : m_sort[ith_line];
	HWND const tree = m_main_window.m_tree_view.get_hwnd();
	HTREEITEM const tree_selected = reinterpret_cast<HTREEITEM>(SendMessageW(tree, TVM_GETNEXTITEM, TVGN_CARET, 0));
	if(!tree_selected)
	{
		return;
	}
	HTREEITEM const tree_parent = reinterpret_cast<HTREEITEM>(SendMessageW(tree, TVM_GETNEXTITEM, TVGN_PARENT, reinterpret_cast<LPARAM>(tree_selected)));
	if(!tree_parent)
	{
		return;
	}
	TVITEMEXW ti;
	ti.hItem = tree_selected;
	ti.mask = TVIF_PARAM;
	LRESULT const got_item = SendMessageW(tree, TVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&ti));
	assert(got_item == TRUE);
	file_info const& fi = *reinterpret_cast<file_info*>(ti.lParam);
	TVITEMEXW ti_2;
	ti_2.hItem = tree_parent;
	ti_2.mask = TVIF_PARAM;
	LRESULT const got_item_2 = SendMessageW(tree, TVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&ti_2));
	assert(got_item_2 == TRUE);
	file_info const& parent_fi = *reinterpret_cast<file_info*>(ti_2.lParam);
	int const ith_dll = static_cast<int>(&fi - parent_fi.m_sub_file_infos.data());
	std::uint16_t const& matched_export = parent_fi.m_import_table.m_matched_exports[ith_dll][ith_import];
	bool const enable_goto_orig = matched_export != 0xFFFF;
	HMENU const menu = reinterpret_cast<HMENU>(m_menu.get());
	BOOL const enabled = EnableMenuItem(menu, static_cast<std::uint16_t>(e_import_menu_id::e_matching), MF_BYCOMMAND | (enable_goto_orig ? MF_ENABLED : MF_GRAYED));
	assert(enabled != -1 && (enabled == MF_ENABLED || enabled == MF_GRAYED));
	BOOL const tracked = TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON | TPM_NOANIMATION, cursor_screen.x, cursor_screen.y, 0, m_main_window.m_hwnd, nullptr);
	assert(tracked != 0);
}

void import_view::on_menu(std::uint16_t const menu_id)
{
	e_import_menu_id const e_menu = static_cast<e_import_menu_id>(menu_id);
	switch(e_menu)
	{
		case e_import_menu_id::e_matching:
		{
			on_menu_matching();
		}
		break;
		default:
		{
			assert(false);
		}
		break;
	}
}

void import_view::on_menu_matching()
{
	select_matching_instance();
}

void import_view::on_accel_matching()
{
	select_matching_instance();
}

void import_view::refresh()
{
	LRESULT const redr_off = SendMessageW(m_hwnd, WM_SETREDRAW, FALSE, 0);
	LRESULT const deleted = SendMessageW(m_hwnd, LVM_DELETEALLITEMS, 0, 0);
	assert(deleted == TRUE);
	auto const redraw_import = mk::make_scope_exit([&]()
	{
		LRESULT const redr_on = SendMessageW(m_hwnd, WM_SETREDRAW, TRUE, 0);
		repaint();
	});

	HWND const tree = m_main_window.m_tree_view.get_hwnd();
	HTREEITEM const selected = reinterpret_cast<HTREEITEM>(SendMessageW(tree, TVM_GETNEXTITEM, TVGN_CARET, 0));
	if(!selected)
	{
		return;
	}
	TVITEMEXW ti;
	ti.hItem = selected;
	ti.mask = TVIF_PARAM;
	LRESULT const got_1 = SendMessageW(tree, TVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&ti));
	assert(got_1 == TRUE);
	file_info const& fi_tmp = *reinterpret_cast<file_info*>(ti.lParam);
	HTREEITEM const parent = reinterpret_cast<HTREEITEM>(SendMessageW(tree, TVM_GETNEXTITEM, TVGN_PARENT, reinterpret_cast<LPARAM>(selected)));
	if(parent)
	{
		TVITEMEXW ti_2;
		ti_2.hItem = parent;
		ti_2.mask = TVIF_PARAM;
		LRESULT const got_2 = SendMessageW(tree, TVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&ti_2));
		assert(got_2 == TRUE);
		file_info const& parent_fi = *reinterpret_cast<file_info*>(ti_2.lParam);
		int const idx = static_cast<int>(&fi_tmp - parent_fi.m_sub_file_infos.data());

		LRESULT const set_size = SendMessageW(m_hwnd, LVM_SETITEMCOUNT, parent_fi.m_import_table.m_import_counts[idx], 0);
		assert(set_size != 0);
	}

	sort_view();

	int const import_type_column_max_width = get_type_column_max_width();
	int const ordinal_column_max_width = m_main_window.get_ordinal_column_max_width();

	LRESULT const auto_sized_pi = SendMessageW(m_hwnd, LVM_SETCOLUMNWIDTH, static_cast<int>(e_import_column::e_pi), LVSCW_AUTOSIZE);
	assert(auto_sized_pi == TRUE);
	LRESULT const type_sized = SendMessageW(m_hwnd, LVM_SETCOLUMNWIDTH, static_cast<int>(e_import_column::e_type), import_type_column_max_width);
	assert(type_sized == TRUE);
	LRESULT const ordinal_sized = SendMessageW(m_hwnd, LVM_SETCOLUMNWIDTH, static_cast<int>(e_import_column::e_ordinal), ordinal_column_max_width);
	assert(ordinal_sized == TRUE);
	LRESULT const hint_sized = SendMessageW(m_hwnd, LVM_SETCOLUMNWIDTH, static_cast<int>(e_import_column::e_hint), ordinal_column_max_width);
	assert(hint_sized == TRUE);
}

void import_view::repaint()
{
	BOOL const redrawn = RedrawWindow(m_hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_FRAME);
	assert(redrawn != 0);
}

void import_view::refresh_headers()
{
	list_view_base::refresh_headers(&m_hwnd, static_cast<int>(std::size(s_import_headers)), m_main_window.m_settings.m_import_sort);
}

void import_view::select_item(std::uint16_t const item_idx)
{
	list_view_base::select_item(&m_hwnd, &m_sort, item_idx);
	repaint();
}

void import_view::sort_view()
{
	std::uint8_t const cur_sort_raw = m_main_window.m_settings.m_import_sort;
	if(cur_sort_raw == 0xFF)
	{
		m_sort.clear();
	}
	else
	{
		bool const cur_sort_asc = (cur_sort_raw & (1u << 7u)) == 0u;
		std::uint8_t const cur_sort_col = cur_sort_raw &~ (1u << 7u);

		HWND const tree = m_main_window.m_tree_view.get_hwnd();
		HTREEITEM const selected = reinterpret_cast<HTREEITEM>(SendMessageW(tree, TVM_GETNEXTITEM, TVGN_CARET, 0));
		if(!selected)
		{
			return;
		}
		HTREEITEM const parent = reinterpret_cast<HTREEITEM>(SendMessageW(tree, TVM_GETNEXTITEM, TVGN_PARENT, reinterpret_cast<LPARAM>(selected)));
		if(!parent)
		{
			return;
		}
		TVITEMEXW ti;
		ti.hItem = parent;
		ti.mask = TVIF_PARAM;
		LRESULT const got_parent = SendMessageW(tree, TVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&ti));
		assert(got_parent == TRUE);
		file_info const& parent_fi = *reinterpret_cast<file_info*>(ti.lParam);
		ti.hItem = selected;
		ti.mask = TVIF_PARAM;
		LRESULT const got_selected = SendMessageW(tree, TVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&ti));
		assert(got_selected == TRUE);
		file_info const& fi_tmp = *reinterpret_cast<file_info*>(ti.lParam);
		file_info const& fi = fi_tmp.m_orig_instance ? *fi_tmp.m_orig_instance : fi_tmp;
		std::uint16_t const dll_idx = static_cast<std::uint16_t>(&fi_tmp - parent_fi.m_sub_file_infos.data());
		pe_import_table_info const& iti = parent_fi.m_import_table;
		pe_export_table_info const& eti = fi.m_export_table;

		std::uint16_t const n_items = iti.m_import_counts[dll_idx];
		if(static_cast<int>(m_sort.size()) != n_items * 2)
		{
			m_sort.resize(n_items * 2);
			std::iota(m_sort.begin(), m_sort.end(), std::uint16_t{0});
		}
		std::uint16_t* const sort = m_sort.data();
		assert(cur_sort_col >= 0 && cur_sort_col < std::size(s_import_headers));
		e_import_column const col = static_cast<e_import_column>(cur_sort_col);
		switch(col)
		{
			case e_import_column::e_pi:
			{
				auto const fn_compare_icon = [&](std::uint16_t const a, std::uint16_t const b, auto const& cmp) -> bool
				{
					std::uint8_t const icon_idx_a = pe_get_import_icon_id(iti, dll_idx, a);
					std::uint8_t const icon_idx_b = pe_get_import_icon_id(iti, dll_idx, b);
					return cmp(icon_idx_a, icon_idx_b);
				};
				if(cur_sort_asc)
				{
					std::stable_sort(sort, sort + n_items, [&](auto const& a, auto const& b){ return fn_compare_icon(a, b, std::less<>{}); });
				}
				else
				{
					std::stable_sort(sort, sort + n_items, [&](auto const& a, auto const& b){ return fn_compare_icon(a, b, std::greater<>{}); });
				}
			}
			break;
			case e_import_column::e_type:
			{
				auto const fn_compare_type = [&](std::uint16_t const a, std::uint16_t const b, auto const& cmp) -> bool
				{
					bool const is_ordinal_a = pe_get_import_is_ordinal(iti, dll_idx, a);
					bool const is_ordinal_b = pe_get_import_is_ordinal(iti, dll_idx, b);
					return cmp(is_ordinal_a, is_ordinal_b);
				};
				if(cur_sort_asc)
				{
					std::stable_sort(sort, sort + n_items, [&](auto const& a, auto const& b){ return fn_compare_type(a, b, std::less<>{}); });
				}
				else
				{
					std::stable_sort(sort, sort + n_items, [&](auto const& a, auto const& b){ return fn_compare_type(a, b, std::greater<>{}); });
				}
			}
			break;
			case e_import_column::e_ordinal:
			{
				auto const fn_compare_ordinal = [&](std::uint16_t const a, std::uint16_t const b, auto const& cmp) -> bool
				{
					auto const ret_a = pe_get_import_ordinal(iti, eti, dll_idx, a);
					auto const ret_b = pe_get_import_ordinal(iti, eti, dll_idx, b);
					return cmp(std::tie(ret_a.m_is_valid, ret_a.m_value), std::tie(ret_b.m_is_valid, ret_b.m_value));
				};
				if(cur_sort_asc)
				{
					std::stable_sort(sort, sort + n_items, [&](auto const& a, auto const& b){ return fn_compare_ordinal(a, b, std::less<>{}); });
				}
				else
				{
					std::stable_sort(sort, sort + n_items, [&](auto const& a, auto const& b){ return fn_compare_ordinal(a, b, std::greater<>{}); });
				}
			}
			break;
			case e_import_column::e_hint:
			{
				auto const fn_compare_hint = [&](std::uint16_t const a, std::uint16_t const b, auto const& cmp) -> bool
				{
					auto const ret_a = pe_get_import_hint(iti, eti, dll_idx, a);
					auto const ret_b = pe_get_import_hint(iti, eti, dll_idx, b);
					return cmp(std::tie(ret_a.m_is_valid, ret_a.m_value), std::tie(ret_b.m_is_valid, ret_b.m_value));
				};
				if(cur_sort_asc)
				{
					std::stable_sort(sort, sort + n_items, [&](auto const& a, auto const& b){ return fn_compare_hint(a, b, std::less<>{}); });
				}
				else
				{
					std::stable_sort(sort, sort + n_items, [&](auto const& a, auto const& b){ return fn_compare_hint(a, b, std::greater<>{}); });
				}
			}
			break;
			case e_import_column::e_name:
			{
				auto const fn_compare_name = [&](std::uint16_t const a, std::uint16_t const b, auto const& cmp) -> bool
				{
					string_handle const ret_a = pe_get_import_name(iti, eti, dll_idx, a);
					string_handle const ret_b = pe_get_import_name(iti, eti, dll_idx, b);
					int const proxy_a = !ret_a.m_string ? 2 : (ret_a.m_string == get_export_name_processing().m_string ? 1 : 0);
					int const proxy_b = !ret_b.m_string ? 2 : (ret_b.m_string == get_export_name_processing().m_string ? 1 : 0);
					string_helper_imp const sha{ret_a};
					string_helper_imp const shb{ret_b};
					return cmp(std::tie(proxy_a, sha), std::tie(proxy_b, shb));
				};
				if(cur_sort_asc)
				{
					std::stable_sort(sort, sort + n_items, [&](auto const& a, auto const& b){ return fn_compare_name(a, b, std::less<>{}); });
				}
				else
				{
					std::stable_sort(sort, sort + n_items, [&](auto const& a, auto const& b){ return fn_compare_name(a, b, std::greater<>{}); });
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
	repaint();
}

smart_menu import_view::create_menu()
{
	HMENU const menu = CreatePopupMenu();
	assert(menu);
	MENUITEMINFOW mi{};
	mi.cbSize = sizeof(mi);
	mi.fMask = MIIM_ID | MIIM_STRING | MIIM_FTYPE;
	mi.fType = MFT_STRING;
	mi.wID = static_cast<std::uint16_t>(e_import_menu_id::e_matching);
	mi.dwTypeData = const_cast<wchar_t*>(s_import_menu_orig_str);
	BOOL const inserted = InsertMenuItemW(menu, 0, TRUE, &mi);
	assert(inserted != 0);
	return smart_menu{menu};
}

wchar_t const* import_view::on_get_col_type(pe_import_table_info const& iti, std::uint16_t const dll_idx, std::uint16_t const imp_idx)
{
	std::uint16_t const& imp_idx_sorted = m_sort.empty() ? imp_idx : m_sort[imp_idx];
	bool const is_ordinal = pe_get_import_is_ordinal(iti, dll_idx, imp_idx_sorted);
	if(is_ordinal)
	{
		return s_import_type_true;
	}
	else
	{
		return s_import_type_false;
	}
}

wchar_t const* import_view::on_get_col_ordinal(pe_import_table_info const& iti, std::uint16_t const dll_idx, std::uint16_t const imp_idx, file_info const& fi)
{
	std::uint16_t const& imp_idx_sorted = m_sort.empty() ? imp_idx : m_sort[imp_idx];
	auto const oridnal_opt = pe_get_import_ordinal(iti, fi.m_export_table, dll_idx, imp_idx_sorted);
	if(oridnal_opt.m_is_valid)
	{
		return ordinal_to_string(oridnal_opt.m_value, m_string_converter);
	}
	else
	{
		return s_import_ordinal_na;
	}
}

wchar_t const* import_view::on_get_col_hint(pe_import_table_info const& iti, std::uint16_t const dll_idx, std::uint16_t const imp_idx, file_info const& fi)
{
	std::uint16_t const& imp_idx_sorted = m_sort.empty() ? imp_idx : m_sort[imp_idx];
	auto const hint_opt = pe_get_import_hint(iti, fi.m_export_table, dll_idx, imp_idx_sorted);
	if(hint_opt.m_is_valid)
	{
		return ordinal_to_string(hint_opt.m_value, m_string_converter);
	}
	else
	{
		return s_import_hint_na;
	}
}

wchar_t const* import_view::on_get_col_name(pe_import_table_info const& iti, std::uint16_t const dll_idx, std::uint16_t const imp_idx, file_info const& fi)
{
	std::uint16_t const& imp_idx_sorted = m_sort.empty() ? imp_idx : m_sort[imp_idx];
	bool const undecorate = m_main_window.m_settings.m_undecorate;
	if(undecorate)
	{
		string_handle const name = pe_get_import_name_undecorated(iti, fi.m_export_table, dll_idx, imp_idx_sorted);
		if(!name.m_string)
		{
			return s_import_name_na;
		}
		else if(name.m_string == get_export_name_processing().m_string)
		{
			return s_import_name_processing;
		}
		else if(name.m_string == get_name_undecorating().m_string)
		{
			return s_import_name_undecorating;
		}
		else
		{
			return m_string_converter.convert(name);
		}
	}
	else
	{
		string_handle const name = pe_get_import_name(iti, fi.m_export_table, dll_idx, imp_idx_sorted);
		if(!name.m_string)
		{
			return s_import_name_na;
		}
		else if(name.m_string == get_export_name_processing().m_string)
		{
			return s_import_name_processing;
		}
		else
		{
			return m_string_converter.convert(name);
		}
	}
}

void import_view::select_matching_instance()
{
	LRESULT const sel = SendMessageW(m_hwnd, LVM_GETNEXTITEM, WPARAM{0} - 1, LVNI_SELECTED);
	if(sel == -1)
	{
		return;
	}
	assert(static_cast<std::size_t>(sel) <= 0xFFFF);
	std::uint16_t const ith_line = static_cast<std::uint16_t>(sel);
	std::uint16_t const ith_import = m_sort.empty() ? ith_line : m_sort[ith_line];
	HWND const tree = m_main_window.m_tree_view.get_hwnd();
	HTREEITEM const tree_selected = reinterpret_cast<HTREEITEM>(SendMessageW(tree, TVM_GETNEXTITEM, TVGN_CARET, 0));
	if(!tree_selected)
	{
		return;
	}
	HTREEITEM const tree_parent = reinterpret_cast<HTREEITEM>(SendMessageW(tree, TVM_GETNEXTITEM, TVGN_PARENT, reinterpret_cast<LPARAM>(tree_selected)));
	if(!tree_parent)
	{
		return;
	}
	TVITEMEXW ti;
	ti.hItem = tree_selected;
	ti.mask = TVIF_PARAM;
	LRESULT const got_item = SendMessageW(tree, TVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&ti));
	assert(got_item == TRUE);
	file_info const& fi = *reinterpret_cast<file_info*>(ti.lParam);
	TVITEMEXW ti_2;
	ti_2.hItem = tree_parent;
	ti_2.mask = TVIF_PARAM;
	LRESULT const got_item_2 = SendMessageW(tree, TVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&ti_2));
	assert(got_item_2 == TRUE);
	file_info const& parent_fi = *reinterpret_cast<file_info*>(ti_2.lParam);
	std::uint16_t const ith_dll = static_cast<std::uint16_t>(&fi - parent_fi.m_sub_file_infos.data());
	std::uint16_t const& matched_exp = parent_fi.m_import_table.m_matched_exports[ith_dll][ith_import];
	if(matched_exp == 0xFFFF)
	{
		return;
	}
	m_main_window.m_export_view.select_item(matched_exp);
}

int import_view::get_type_column_max_width()
{
	if(g_import_type_column_max_width != 0)
	{
		return g_import_type_column_max_width;
	}

	HDC const dc = GetDC(m_hwnd);
	assert(dc != NULL);
	smart_dc const sdc(m_hwnd, dc);
	auto const orig_font = SelectObject(dc, reinterpret_cast<HFONT>(SendMessageW(m_hwnd, WM_GETFONT, 0, 0)));
	auto const fn_revert = mk::make_scope_exit([&](){ SelectObject(dc, orig_font); });

	int maximum = 0;
	SIZE size;

	BOOL const got1 = GetTextExtentPointW(dc, s_import_type_true, static_cast<int>(std::size(s_import_type_true)) - 1, &size);
	assert(got1 != 0);
	maximum = (std::max)(maximum, static_cast<int>(size.cx));

	BOOL const got2 = GetTextExtentPointW(dc, s_import_type_false, static_cast<int>(std::size(s_import_type_false)) - 1, &size);
	assert(got2 != 0);
	maximum = (std::max)(maximum, static_cast<int>(size.cx));

	static constexpr int const s_trailing_label_padding = 12;
	g_import_type_column_max_width = maximum + s_trailing_label_padding;
	return maximum;
}
