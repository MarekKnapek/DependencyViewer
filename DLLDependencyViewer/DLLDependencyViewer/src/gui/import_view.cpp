#include "import_view.h"

#include "constants.h"
#include "main.h"
#include "main_window.h"
#include "smart_dc.h"

#include "../nogui/pe.h"
#include "../nogui/int_to_string.h"
#include "../nogui/scope_exit.h"

#include "../res/resources.h"

#include <algorithm>
#include <iterator>
#include <cassert>

#include <windowsx.h>
#include <commctrl.h>


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


static int g_import_type_column_max_width = 0;


import_view::import_view(HWND const parent, main_window& mw) :
	m_hwnd(CreateWindowExW(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE, WC_LISTVIEWW, nullptr, WS_VISIBLE | WS_CHILD | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_OWNERDATA, 0, 0, 0, 0, parent, nullptr, get_instance(), nullptr)),
	m_main_window(mw),
	m_menu(create_menu()),
	m_tmp_strings(),
	m_tmp_string_idx()
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
	if(nmhdr.code == LVN_GETDISPINFOW)
	{
		on_getdispinfow(nmhdr);
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
	int const dll_idx = static_cast<int>(&fi_tmp - parent_fi.m_sub_file_infos.data());
	int const row = nm.item.iItem;
	int const col = nm.item.iSubItem;
	pe_import_entry const& import_entry = parent_fi.m_import_table.m_dlls[dll_idx].m_entries[row];
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
				nm.item.pszText = const_cast<wchar_t*>(on_get_col_type(import_entry));
			}
			break;
			case e_import_column::e_ordinal:
			{
				nm.item.pszText = const_cast<wchar_t*>(on_get_col_ordinal(import_entry, fi));
			}
			break;
			case e_import_column::e_hint:
			{
				nm.item.pszText = const_cast<wchar_t*>(on_get_col_hint(import_entry, fi));
			}
			break;
			case e_import_column::e_name:
			{
				nm.item.pszText = const_cast<wchar_t*>(on_get_col_name(import_entry, fi));
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
		bool const matched = import_entry.m_matched_export != 0xffff;
		bool const ordinal = import_entry.m_is_ordinal;
		if(matched && ordinal)
		{
			nm.item.iImage = s_res_icon_import_found_o;
		}
		else if(matched && !ordinal)
		{
			nm.item.iImage = s_res_icon_import_found_c;
		}
		else if(!matched && ordinal)
		{
			nm.item.iImage = s_res_icon_import_not_found_o;
		}
		else if(!matched && !ordinal)
		{
			nm.item.iImage = s_res_icon_import_not_found_c;
		}
		else
		{
			__assume(false);
		}
	}
}

void import_view::on_context_menu(LPARAM const lparam)
{
	POINT cursor_screen;
	int ith_import;
	if(lparam == 0xffffffff)
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
		ith_import = static_cast<int>(sel);
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
		ith_import = hti.iItem;
	}
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
	std::uint16_t const& matched = parent_fi.m_import_table.m_dlls[ith_dll].m_entries[ith_import].m_matched_export;
	bool const enable_goto_orig = matched != 0xffff;
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

		LRESULT const set_size = SendMessageW(m_hwnd, LVM_SETITEMCOUNT, parent_fi.m_import_table.m_dlls[idx].m_entries.size(), 0);
		assert(set_size != 0);
	}

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

void import_view::select_item(std::uint16_t const item_idx)
{
	LRESULT const visibility_ensured = SendMessageW(m_hwnd, LVM_ENSUREVISIBLE, item_idx, FALSE);
	assert(visibility_ensured == TRUE);
	LVITEM lvi;
	lvi.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
	lvi.state = 0;
	LRESULT const selection_cleared = SendMessageW(m_hwnd, LVM_SETITEMSTATE, WPARAM{0} - 1, reinterpret_cast<LPARAM>(&lvi));
	assert(selection_cleared == TRUE);
	lvi.state = LVIS_FOCUSED | LVIS_SELECTED;
	LRESULT const selection_set = SendMessageW(m_hwnd, LVM_SETITEMSTATE, static_cast<WPARAM>(item_idx), reinterpret_cast<LPARAM>(&lvi));
	assert(selection_set == TRUE);
	HWND const prev_focus = SetFocus(m_hwnd);
	assert(prev_focus != nullptr);
	(void)prev_focus;
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

wchar_t const* import_view::on_get_col_type(pe_import_entry const& import_entry)
{
	if(import_entry.m_is_ordinal)
	{
		return s_import_type_true;
	}
	else
	{
		return s_import_type_false;
	}
}

wchar_t const* import_view::on_get_col_ordinal(pe_import_entry const& import_entry, file_info const& fi)
{
	if(import_entry.m_is_ordinal)
	{
		std::wstring& tmpstr = m_tmp_strings[m_tmp_string_idx++ % m_tmp_strings.size()];
		ordinal_to_string(import_entry.m_ordinal_or_hint, tmpstr);
		return tmpstr.c_str();
	}
	else
	{
		if(import_entry.m_matched_export != 0xffff)
		{
			return m_main_window.m_export_view.on_get_col_ordinal(fi.m_export_table.m_export_address_table[import_entry.m_matched_export]);
		}
		else
		{
			return s_import_ordinal_na;
		}
	}
}

wchar_t const* import_view::on_get_col_hint(pe_import_entry const& import_entry, file_info const& fi)
{
	if(import_entry.m_is_ordinal)
	{
		if(import_entry.m_matched_export != 0xffff)
		{
			return m_main_window.m_export_view.on_get_col_hint(fi.m_export_table.m_export_address_table[import_entry.m_matched_export]);
		}
		else
		{
			return s_import_hint_na;
		}
	}
	else
	{
		std::wstring& tmpstr = m_tmp_strings[m_tmp_string_idx++ % m_tmp_strings.size()];
		ordinal_to_string(import_entry.m_ordinal_or_hint, tmpstr);
		return tmpstr.c_str();
	}
}

wchar_t const* import_view::on_get_col_name(pe_import_entry const& import_entry, file_info const& fi)
{
	if(import_entry.m_is_ordinal)
	{
		if(import_entry.m_matched_export != 0xffff)
		{
			return m_main_window.m_export_view.on_get_col_name(fi.m_export_table.m_export_address_table[import_entry.m_matched_export]);
		}
		else
		{
			return s_import_name_na;
		}
	}
	else
	{
		std::wstring& tmpstr = m_tmp_strings[m_tmp_string_idx++ % m_tmp_strings.size()];
		tmpstr.resize(import_entry.m_name->m_len);
		std::transform(cbegin(import_entry.m_name), cend(import_entry.m_name), begin(tmpstr), [](char const& e) -> wchar_t { return static_cast<wchar_t>(e); });
		return tmpstr.c_str();
	}
}

void import_view::select_matching_instance()
{
	LRESULT const sel = SendMessageW(m_hwnd, LVM_GETNEXTITEM, WPARAM{0} - 1, LVNI_SELECTED);
	if(sel == -1)
	{
		return;
	}
	int const ith_import = static_cast<int>(sel);
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
	std::uint16_t const& matched = parent_fi.m_import_table.m_dlls[ith_dll].m_entries[ith_import].m_matched_export;
	if(matched == 0xffff)
	{
		return;
	}
	m_main_window.m_export_view.select_item(matched);
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
