#include "import_view.h"

#include "common_controls.h"
#include "constants.h"
#include "list_view_base.h"
#include "main.h"
#include "main_window.h"
#include "smart_dc.h"

#include "../nogui/array_bool.h"
#include "../nogui/cassert_my.h"
#include "../nogui/int_to_string.h"
#include "../nogui/pe.h"
#include "../nogui/pe_getters_import.h"
#include "../nogui/scope_exit.h"

#include "../res/resources.h"

#include <algorithm>
#include <functional>
#include <iterator>
#include <numeric>
#include <tuple>

#include "../nogui/my_windows.h"

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
	HIMAGELIST const img_list = common_controls::ImageList_LoadImageW(get_instance(), MAKEINTRESOURCEW(s_res_icons_import_export), 30, 0, CLR_DEFAULT, IMAGE_BITMAP, LR_DEFAULTCOLOR);
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
	file_info const* const tmp_fi = m_main_window.m_tree_view.get_selection();
	if(!tmp_fi)
	{
		return;
	}
	file_info const* const parent_fi = tmp_fi->m_parent;
	if(!parent_fi)
	{
		return;
	}
	file_info const& fi = tmp_fi->m_orig_instance ? *tmp_fi->m_orig_instance : *tmp_fi;
	auto const dll_idx_ = tmp_fi - parent_fi->m_fis;
	assert(dll_idx_ >= 0 && dll_idx_ <= 0xFFFF);
	std::uint16_t const dll_idx = static_cast<std::uint16_t>(dll_idx_);
	int const row = nm.item.iItem;
	int const col = nm.item.iSubItem;
	std::uint16_t const imp_idx = static_cast<std::uint16_t>(row);
	e_import_column const ecol = static_cast<e_import_column>(col);
	if((nm.item.mask & LVIF_TEXT) != 0)
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
				nm.item.pszText = const_cast<wchar_t*>(on_get_col_type(parent_fi->m_import_table, dll_idx, imp_idx));
			}
			break;
			case e_import_column::e_ordinal:
			{
				nm.item.pszText = const_cast<wchar_t*>(on_get_col_ordinal(parent_fi->m_import_table, dll_idx, imp_idx));
			}
			break;
			case e_import_column::e_hint:
			{
				nm.item.pszText = const_cast<wchar_t*>(on_get_col_hint(parent_fi->m_import_table, dll_idx, imp_idx));
			}
			break;
			case e_import_column::e_name:
			{
				nm.item.pszText = const_cast<wchar_t*>(on_get_col_name(parent_fi->m_import_table, dll_idx, imp_idx, fi));
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
		std::uint8_t const icon_idx = pe_get_import_icon_id(parent_fi->m_import_table, dll_idx, imp_idx_sorted);
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
	int item_idx_;
	POINT screen_pos;
	bool const context_found = list_view_base::get_context_menu(&m_hwnd, &lparam, &m_sort, &item_idx_, &screen_pos);
	if(!context_found)
	{
		return;
	}
	assert(item_idx_ >= 0 && item_idx_ <= 0xFFFF);
	std::uint16_t const item_idx = static_cast<std::uint16_t>(item_idx_);
	file_info const* const fi = m_main_window.m_tree_view.get_selection();
	if(!fi)
	{
		return;
	}
	file_info const* const parent_fi = fi->m_parent;
	if(!parent_fi)
	{
		return;
	}
	auto const dll_idx_ = fi - parent_fi->m_fis;
	assert(dll_idx_ >= 0 && dll_idx_ <= 0xFFFF);
	std::uint16_t const dll_idx = static_cast<std::uint16_t>(dll_idx_);
	std::uint16_t const& matched_export = parent_fi->m_import_table.m_matched_exports[dll_idx][item_idx];
	bool const enable_goto_orig = matched_export != 0xFFFF;
	HMENU const menu = reinterpret_cast<HMENU>(m_menu.get());
	BOOL const enabled = EnableMenuItem(menu, static_cast<std::uint16_t>(e_import_menu_id::e_matching), MF_BYCOMMAND | (enable_goto_orig ? MF_ENABLED : MF_GRAYED));
	assert(enabled != -1 && (enabled == MF_ENABLED || enabled == MF_GRAYED));
	BOOL const tracked = TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON | TPM_NOANIMATION, screen_pos.x, screen_pos.y, 0, m_main_window.m_hwnd, nullptr);
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

	file_info const* const tmp_fi = m_main_window.m_tree_view.get_selection();
	if(!tmp_fi)
	{
		return;
	}
	file_info const* const parent_fi = tmp_fi->m_parent;
	if(parent_fi)
	{
		auto const idx_ = tmp_fi - parent_fi->m_fis;
		assert(idx_ >= 0 && idx_ <= 0xFFFF);
		std::uint16_t const idx = static_cast<std::uint16_t>(idx_);

		LRESULT const set_size = SendMessageW(m_hwnd, LVM_SETITEMCOUNT, parent_fi->m_import_table.m_import_counts[idx], 0);
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

		file_info const* const tmp_fi = m_main_window.m_tree_view.get_selection();
		if(!tmp_fi)
		{
			return;
		}
		file_info const* const parent_fi = tmp_fi->m_parent;
		if(!parent_fi)
		{
			return;
		}
		file_info const& fi = tmp_fi->m_orig_instance ? *tmp_fi->m_orig_instance : *tmp_fi;
		auto const dll_idx_ = tmp_fi - parent_fi->m_fis;
		assert(dll_idx_ >= 0 && dll_idx_ <= 0xFFFF);
		std::uint16_t const dll_idx = static_cast<std::uint16_t>(dll_idx_);
		pe_import_table_info const& iti = parent_fi->m_import_table;
		pe_export_table_info const& eti = fi.m_export_table;

		std::uint16_t const n_items = iti.m_import_counts[dll_idx];
		if(static_cast<int>(m_sort.size()) != n_items * 2)
		{
			m_sort.resize(n_items * 2);
			std::iota(m_sort.begin(), m_sort.begin() + n_items, std::uint16_t{0});
		}
		std::uint16_t* const sort = m_sort.data();
		assert(cur_sort_col >= 0 && cur_sort_col < std::size(s_import_headers));
		e_import_column const col = static_cast<e_import_column>(cur_sort_col);
		switch(col)
		{
			case e_import_column::e_pi:
			{
				auto const fn_compare_icon = [&](std::uint16_t const a, std::uint16_t const b) -> bool
				{
					std::uint8_t const icon_idx_a = pe_get_import_icon_id(iti, dll_idx, a);
					std::uint8_t const icon_idx_b = pe_get_import_icon_id(iti, dll_idx, b);
					return icon_idx_a < icon_idx_b;
				};
				if(cur_sort_asc)
				{
					std::stable_sort(sort, sort + n_items, [&](auto const& a, auto const& b){ return fn_compare_icon(a, b); });
				}
				else
				{
					std::stable_sort(sort, sort + n_items, [&](auto const& a, auto const& b){ return fn_compare_icon(b, a); });
				}
			}
			break;
			case e_import_column::e_type:
			{
				auto const fn_compare_type = [&](std::uint16_t const a, std::uint16_t const b) -> bool
				{
					bool const is_ordinal_a = pe_get_import_is_ordinal(iti, dll_idx, a);
					bool const is_ordinal_b = pe_get_import_is_ordinal(iti, dll_idx, b);
					return is_ordinal_a < is_ordinal_b;
				};
				if(cur_sort_asc)
				{
					std::stable_sort(sort, sort + n_items, [&](auto const& a, auto const& b){ return fn_compare_type(a, b); });
				}
				else
				{
					std::stable_sort(sort, sort + n_items, [&](auto const& a, auto const& b){ return fn_compare_type(b, a); });
				}
			}
			break;
			case e_import_column::e_ordinal:
			{
				auto const fn_compare_ordinal = [&](std::uint16_t const a, std::uint16_t const b) -> bool
				{
					auto const ret_a = pe_get_import_ordinal(iti, dll_idx, a);
					auto const ret_b = pe_get_import_ordinal(iti, dll_idx, b);
					return std::tie(ret_a.m_is_valid, ret_a.m_value) < std::tie(ret_b.m_is_valid, ret_b.m_value);
				};
				if(cur_sort_asc)
				{
					std::stable_sort(sort, sort + n_items, [&](auto const& a, auto const& b){ return fn_compare_ordinal(a, b); });
				}
				else
				{
					std::stable_sort(sort, sort + n_items, [&](auto const& a, auto const& b){ return fn_compare_ordinal(b, a); });
				}
			}
			break;
			case e_import_column::e_hint:
			{
				auto const fn_compare_hint = [&](std::uint16_t const a, std::uint16_t const b) -> bool
				{
					auto const ret_a = pe_get_import_hint(iti, dll_idx, a);
					auto const ret_b = pe_get_import_hint(iti, dll_idx, b);
					return std::tie(ret_a.m_is_valid, ret_a.m_value) < std::tie(ret_b.m_is_valid, ret_b.m_value);
				};
				if(cur_sort_asc)
				{
					std::stable_sort(sort, sort + n_items, [&](auto const& a, auto const& b){ return fn_compare_hint(a, b); });
				}
				else
				{
					std::stable_sort(sort, sort + n_items, [&](auto const& a, auto const& b){ return fn_compare_hint(b, a); });
				}
			}
			break;
			case e_import_column::e_name:
			{
				auto const fn_compare_name = [&](std::uint16_t const a, std::uint16_t const b) -> bool
				{
					string_handle const ret_a = pe_get_import_name(iti, eti, dll_idx, a);
					string_handle const ret_b = pe_get_import_name(iti, eti, dll_idx, b);
					int const proxy_a = !ret_a.m_string ? 2 : (ret_a.m_string == get_export_name_processing().m_string ? 1 : 0);
					int const proxy_b = !ret_b.m_string ? 2 : (ret_b.m_string == get_export_name_processing().m_string ? 1 : 0);
					string_helper_imp const sha{ret_a};
					string_helper_imp const shb{ret_b};
					return std::tie(proxy_a, sha) < std::tie(proxy_b, shb);
				};
				if(cur_sort_asc)
				{
					std::stable_sort(sort, sort + n_items, [&](auto const& a, auto const& b){ return fn_compare_name(a, b); });
				}
				else
				{
					std::stable_sort(sort, sort + n_items, [&](auto const& a, auto const& b){ return fn_compare_name(b, a); });
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

wchar_t const* import_view::on_get_col_ordinal(pe_import_table_info const& iti, std::uint16_t const dll_idx, std::uint16_t const imp_idx)
{
	std::uint16_t const& imp_idx_sorted = m_sort.empty() ? imp_idx : m_sort[imp_idx];
	auto const oridnal_opt = pe_get_import_ordinal(iti, dll_idx, imp_idx_sorted);
	if(oridnal_opt.m_is_valid)
	{
		return ordinal_to_string(oridnal_opt.m_value, m_string_converter);
	}
	else
	{
		return s_import_ordinal_na;
	}
}

wchar_t const* import_view::on_get_col_hint(pe_import_table_info const& iti, std::uint16_t const dll_idx, std::uint16_t const imp_idx)
{
	std::uint16_t const& imp_idx_sorted = m_sort.empty() ? imp_idx : m_sort[imp_idx];
	auto const hint_opt = pe_get_import_hint(iti, dll_idx, imp_idx_sorted);
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
	int const sel = list_view_base::get_selection(&m_hwnd);
	if(sel == -1)
	{
		return;
	}
	assert(sel >= 0 && sel <= 0xFFFF);
	std::uint16_t const line_idx = static_cast<std::uint16_t>(sel);
	std::uint16_t const item_idx = m_sort.empty() ? line_idx : m_sort[line_idx];
	file_info const* const fi = m_main_window.m_tree_view.get_selection();
	if(!fi)
	{
		return;
	}
	file_info const* const parent_fi = fi->m_parent;
	if(!parent_fi)
	{
		return;
	}
	auto const dll_idx_ = fi - parent_fi->m_fis;
	assert(dll_idx_ >= 0 && dll_idx_ <= 0xFFFF);
	std::uint16_t const dll_idx = static_cast<std::uint16_t>(dll_idx_);
	std::uint16_t const& matched_exp = parent_fi->m_import_table.m_matched_exports[dll_idx][item_idx];
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
