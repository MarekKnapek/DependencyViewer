#include "export_view.h"

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
#include "../nogui/pe_getters_export.h"
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


enum class e_export_menu_id : std::uint16_t
{
	e_matching = s_export_view_menu_min,
};
enum class e_export_column
{
	e_e,
	e_type,
	e_ordinal,
	e_hint,
	e_name,
	e_entry_point
};
static constexpr wchar_t const* const s_export_headers[] =
{
	L"E",
	L"type",
	L"ordinal",
	L"hint",
	L"name",
	L"entry point"
};
static constexpr wchar_t const s_export_menu_orig_str[] = L"&Highlight Matching Import Function\tCtrl+M";
static constexpr wchar_t const s_export_type_true[] = L"address";
static constexpr wchar_t const s_export_type_false[] = L"forwarder";
static constexpr wchar_t const s_export_hint_na[] = L"N/A";
static constexpr wchar_t const s_export_name_processing[] = L"Processing...";
static constexpr wchar_t const s_export_name_na[] = L"N/A";
static constexpr wchar_t const s_export_name_undecorating[] = L"Undecorating...";


static int g_export_type_column_max_width = 0;


struct string_helper_exp
{
	string_handle const& m_string;
};

inline bool operator<(string_helper_exp const& a, string_helper_exp const& b) noexcept
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

struct entry_point_helper
{
	bool const& m_is_rva;
	pe_rva_or_forwarder const& m_entry_point;
};

inline bool operator<(entry_point_helper const& a, entry_point_helper const& b)
{
	if(a.m_is_rva < b.m_is_rva)
	{
		return true;
	}
	else if(b.m_is_rva < a.m_is_rva)
	{
		return false;
	}
	else
	{
		assert(a.m_is_rva == b.m_is_rva);
		if(a.m_is_rva)
		{
			return a.m_entry_point.m_rva < b.m_entry_point.m_rva;
		}
		else
		{
			return a.m_entry_point.m_forwarder < b.m_entry_point.m_forwarder;
		}
	}
}


export_view::export_view(HWND const parent, main_window& mw) :
	m_hwnd(CreateWindowExW(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE, WC_LISTVIEWW, nullptr, WS_VISIBLE | WS_CHILD | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_OWNERDATA, 0, 0, 0, 0, parent, nullptr, get_instance(), nullptr)),
	m_main_window(mw),
	m_menu(create_menu()),
	m_sort(),
	m_string_converter()
{
	static constexpr unsigned const extended_lv_styles = LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP | LVS_EX_DOUBLEBUFFER;
	LRESULT const set_export = SendMessageW(m_hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, extended_lv_styles, extended_lv_styles);
	for(int i = 0; i != static_cast<int>(std::size(s_export_headers)); ++i)
	{
		LVCOLUMNW cl;
		cl.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		cl.fmt = LVCFMT_LEFT;
		cl.cx = 200;
		cl.pszText = const_cast<LPWSTR>(s_export_headers[i]);
		cl.cchTextMax = 0;
		cl.iSubItem = i;
		cl.iImage = 0;
		cl.iOrder = i;
		cl.cxMin = 0;
		cl.cxDefault = 0;
		cl.cxIdeal = 0;
		LRESULT const inserted = SendMessageW(m_hwnd, LVM_INSERTCOLUMNW, i, reinterpret_cast<LPARAM>(&cl));
		assert(inserted != -1 && inserted == i);
	}
	HIMAGELIST const img_list = common_controls::ImageList_LoadImageW(get_instance(), MAKEINTRESOURCEW(s_res_icons_import_export), 30, 0, CLR_DEFAULT, IMAGE_BITMAP, LR_DEFAULTCOLOR);
	assert(img_list);
	LRESULT const img_list_set = SendMessageW(m_hwnd, LVM_SETIMAGELIST, LVSIL_SMALL, reinterpret_cast<LPARAM>(img_list));
}

export_view::~export_view()
{
}

HWND export_view::get_hwnd() const
{
	return m_hwnd;
}

void export_view::on_notify(NMHDR& nmhdr)
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

void export_view::on_getdispinfow(NMHDR& nmhdr)
{
	NMLVDISPINFOW& nm = reinterpret_cast<NMLVDISPINFOW&>(nmhdr);
	file_info const* const tmp_fi = m_main_window.m_tree_view.get_selection();
	if(!tmp_fi)
	{
		return;
	}
	file_info const& fi = tmp_fi->m_orig_instance ? *tmp_fi->m_orig_instance : *tmp_fi;
	int const row = nm.item.iItem;
	int const col = nm.item.iSubItem;
	pe_export_table_info const& eti = fi.m_export_table;
	std::uint16_t const exp_idx = static_cast<std::uint16_t>(row);
	e_export_column const ecol = static_cast<e_export_column>(col);
	if((nm.item.mask & LVIF_TEXT) != 0)
	{
		switch(ecol)
		{
			case e_export_column::e_e:
			{
				nm.item.pszText = const_cast<wchar_t*>(L"");
			}
			break;
			case e_export_column::e_type:
			{
				nm.item.pszText = const_cast<wchar_t*>(on_get_col_type(eti, exp_idx));
			}
			break;
			case e_export_column::e_ordinal:
			{
				nm.item.pszText = const_cast<wchar_t*>(on_get_col_ordinal(eti, exp_idx));
			}
			break;
			case e_export_column::e_hint:
			{
				nm.item.pszText = const_cast<wchar_t*>(on_get_col_hint(eti, exp_idx));
			}
			break;
			case e_export_column::e_name:
			{
				nm.item.pszText = const_cast<wchar_t*>(on_get_col_name(eti, exp_idx));
			}
			break;
			case e_export_column::e_entry_point:
			{
				nm.item.pszText = const_cast<wchar_t*>(on_get_col_address(eti, exp_idx));
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
		std::uint16_t const& real_exp_idx = m_sort.empty() ? exp_idx : m_sort[exp_idx];
		std::uint8_t const img_idx = pe_get_export_icon_id(eti, tmp_fi->m_matched_imports, real_exp_idx);
		nm.item.iImage = img_idx;
	}
}

void export_view::on_columnclick(NMHDR& nmhdr)
{
	NMLISTVIEW const& nmlv = reinterpret_cast<NMLISTVIEW&>(nmhdr);
	int const new_sort = list_view_base::on_columnclick(&nmlv, static_cast<int>(std::size(s_export_headers)), m_main_window.m_settings.m_export_sort);
	assert(new_sort <= 0xFF);
	m_main_window.m_settings.m_export_sort = static_cast<std::uint8_t>(new_sort);
	sort_view();
	refresh_headers();
}

void export_view::on_context_menu(LPARAM const lparam)
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
	std::uint16_t const matched = (fi->m_matched_imports != nullptr) ? fi->m_matched_imports[item_idx] : static_cast<std::uint16_t>(0xFFFF);
	bool const enable_goto_orig = matched != 0xFFFF;
	HMENU const menu = reinterpret_cast<HMENU>(m_menu.get());
	BOOL const enabled = EnableMenuItem(menu, static_cast<std::uint16_t>(e_export_menu_id::e_matching), MF_BYCOMMAND | (enable_goto_orig ? MF_ENABLED : MF_GRAYED));
	assert(enabled != -1 && (enabled == MF_ENABLED || enabled == MF_GRAYED));
	BOOL const tracked = TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON | TPM_NOANIMATION, screen_pos.x, screen_pos.y, 0, m_main_window.m_hwnd, nullptr);
	assert(tracked != 0);
}

void export_view::on_menu(std::uint16_t const menu_id)
{
	e_export_menu_id const e_menu = static_cast<e_export_menu_id>(menu_id);
	switch(e_menu)
	{
		case e_export_menu_id::e_matching:
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

void export_view::on_menu_matching()
{
	select_matching_instance();
}

void export_view::on_accel_matching()
{
	select_matching_instance();
}

void export_view::refresh()
{
	LRESULT const redr_off = SendMessageW(m_hwnd, WM_SETREDRAW, FALSE, 0);
	LRESULT const deleted = SendMessageW(m_hwnd, LVM_DELETEALLITEMS, 0, 0);
	assert(deleted == TRUE);
	auto const redraw_export = mk::make_scope_exit([&]()
	{
		LRESULT const redr_on = SendMessageW(m_hwnd, WM_SETREDRAW, TRUE, 0);
		repaint();
	});

	file_info const* const tmp_fi = m_main_window.m_tree_view.get_selection();
	if(!tmp_fi)
	{
		return;
	}
	file_info const& fi = tmp_fi->m_orig_instance ? *tmp_fi->m_orig_instance : *tmp_fi;

	LRESULT const set_size = SendMessageW(m_hwnd, LVM_SETITEMCOUNT, fi.m_export_table.m_count, 0);
	assert(set_size != 0);

	sort_view();

	int const export_type_column_max_width = get_type_column_max_width();
	int const ordinal_column_max_width = m_main_window.get_ordinal_column_max_width();

	LRESULT const auto_sized_e = SendMessageW(m_hwnd, LVM_SETCOLUMNWIDTH, static_cast<int>(e_export_column::e_e), LVSCW_AUTOSIZE);
	assert(auto_sized_e == TRUE);
	LRESULT const type_sized = SendMessageW(m_hwnd, LVM_SETCOLUMNWIDTH, static_cast<int>(e_export_column::e_type), export_type_column_max_width);
	assert(type_sized == TRUE);
	LRESULT const ordinal_sized = SendMessageW(m_hwnd, LVM_SETCOLUMNWIDTH, static_cast<int>(e_export_column::e_ordinal), ordinal_column_max_width);
	assert(ordinal_sized == TRUE);
	LRESULT const hint_sized = SendMessageW(m_hwnd, LVM_SETCOLUMNWIDTH, static_cast<int>(e_export_column::e_hint), ordinal_column_max_width);
	assert(hint_sized == TRUE);
}

void export_view::repaint()
{
	BOOL const redrawn = RedrawWindow(m_hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_FRAME);
	assert(redrawn != 0);
}

void export_view::refresh_headers()
{
	list_view_base::refresh_headers(&m_hwnd, static_cast<int>(std::size(s_export_headers)), m_main_window.m_settings.m_export_sort);
}

void export_view::select_item(std::uint16_t const item_idx)
{
	list_view_base::select_item(&m_hwnd, &m_sort, item_idx);
	repaint();
}

void export_view::sort_view()
{
	std::uint8_t const cur_sort_raw = m_main_window.m_settings.m_export_sort;
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
		file_info const& fi = tmp_fi->m_orig_instance ? *tmp_fi->m_orig_instance : *tmp_fi;
		pe_export_table_info const& eti = fi.m_export_table;

		std::uint16_t const n_items = eti.m_count;
		if(static_cast<int>(m_sort.size()) != n_items * 2)
		{
			m_sort.resize(n_items * 2);
			std::iota(m_sort.begin(), m_sort.begin() + n_items, std::uint16_t{0});
		}
		std::uint16_t* const sort = m_sort.data();
		assert(cur_sort_col >= 0 && cur_sort_col < std::size(s_export_headers));
		e_export_column const col = static_cast<e_export_column>(cur_sort_col);
		switch(col)
		{
			case e_export_column::e_e:
			{
				auto const fn_compare_icon = [&](std::uint16_t const a, std::uint16_t const b) -> bool
				{
					std::uint8_t const icon_idx_a = pe_get_export_icon_id(eti, tmp_fi->m_matched_imports, a);
					std::uint8_t const icon_idx_b = pe_get_export_icon_id(eti, tmp_fi->m_matched_imports, b);
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
			case e_export_column::e_type:
			{
				auto const fn_compare_type = [&](std::uint16_t const a, std::uint16_t const b) -> bool
				{
					bool const is_rva_a = pe_get_export_type(eti, a);
					bool const is_rva_b = pe_get_export_type(eti, b);
					return is_rva_a < is_rva_b;
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
			case e_export_column::e_ordinal:
			{
				auto const fn_compare_ordinal = [&](std::uint16_t const a, std::uint16_t const b) -> bool
				{
					std::uint16_t const ordinal_a = pe_get_export_ordinal(eti, a);
					std::uint16_t const ordinal_b = pe_get_export_ordinal(eti, b);
					return ordinal_a < ordinal_b;
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
			case e_export_column::e_hint:
			{
				auto const fn_compare_hint = [&](std::uint16_t const a, std::uint16_t const b) -> bool
				{
					auto const ret_a = pe_get_export_hint(eti, a);
					auto const ret_b = pe_get_export_hint(eti, b);
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
			case e_export_column::e_name:
			{
				auto const fn_compare_name = [&](std::uint16_t const a, std::uint16_t const b) -> bool
				{
					string_handle const ret_a = pe_get_export_name(eti, a);
					string_handle const ret_b = pe_get_export_name(eti, b);
					int const proxy_a = !ret_a.m_string ? 2 : (ret_a.m_string == get_export_name_processing().m_string ? 1 : 0);
					int const proxy_b = !ret_b.m_string ? 2 : (ret_b.m_string == get_export_name_processing().m_string ? 1 : 0);
					string_helper_exp const sha{ret_a};
					string_helper_exp const shb{ret_b};
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
			case e_export_column::e_entry_point:
			{
				auto const fn_compare_entry_point = [&](std::uint16_t const a, std::uint16_t const b) -> bool
				{
					bool const is_rva_a = pe_get_export_type(eti, a);
					bool const is_rva_b = pe_get_export_type(eti, b);
					pe_rva_or_forwarder const entry_a = pe_get_export_entry_point(eti, a);
					pe_rva_or_forwarder const entry_b = pe_get_export_entry_point(eti, b);
					entry_point_helper const ha{is_rva_a, entry_a};
					entry_point_helper const hb{is_rva_b, entry_b};
					return std::tie(ha) < std::tie(hb);
				};
				if(cur_sort_asc)
				{
					std::stable_sort(sort, sort + n_items, [&](auto const& a, auto const& b){ return fn_compare_entry_point(a, b); });
				}
				else
				{
					std::stable_sort(sort, sort + n_items, [&](auto const& a, auto const& b){ return fn_compare_entry_point(b, a); });
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

smart_menu export_view::create_menu()
{
	HMENU const menu = CreatePopupMenu();
	assert(menu);
	MENUITEMINFOW mi{};
	mi.cbSize = sizeof(mi);
	mi.fMask = MIIM_ID | MIIM_STRING | MIIM_FTYPE;
	mi.fType = MFT_STRING;
	mi.wID = static_cast<std::uint16_t>(e_export_menu_id::e_matching);
	mi.dwTypeData = const_cast<wchar_t*>(s_export_menu_orig_str);
	BOOL const inserted = InsertMenuItemW(menu, 0, TRUE, &mi);
	assert(inserted != 0);
	return smart_menu{menu};
}

wchar_t const* export_view::on_get_col_type(pe_export_table_info const& eti, std::uint16_t const exp_idx)
{
	std::uint16_t const& exp_idx_sorted = m_sort.empty() ? exp_idx : m_sort[exp_idx];
	bool const is_rva = pe_get_export_type(eti, exp_idx_sorted);
	if(is_rva)
	{
		return s_export_type_true;
	}
	else
	{
		return s_export_type_false;
	}
}

wchar_t const* export_view::on_get_col_ordinal(pe_export_table_info const& eti, std::uint16_t const exp_idx)
{
	std::uint16_t const& exp_idx_sorted = m_sort.empty() ? exp_idx : m_sort[exp_idx];
	std::uint16_t const ordinal = pe_get_export_ordinal(eti, exp_idx_sorted);
	return ordinal_to_string(ordinal, m_string_converter);
}

wchar_t const* export_view::on_get_col_hint(pe_export_table_info const& eti, std::uint16_t const exp_idx)
{
	std::uint16_t const& exp_idx_sorted = m_sort.empty() ? exp_idx : m_sort[exp_idx];
	auto const hint_opt = pe_get_export_hint(eti, exp_idx_sorted);
	if(hint_opt.m_is_valid)
	{
		std::uint16_t const& hint = hint_opt.m_value;
		return ordinal_to_string(hint, m_string_converter);
	}
	else
	{
		return s_export_hint_na;
	}
}

wchar_t const* export_view::on_get_col_name(pe_export_table_info const& eti, std::uint16_t const exp_idx)
{
	std::uint16_t const& exp_idx_sorted = m_sort.empty() ? exp_idx : m_sort[exp_idx];
	bool const undecorate = m_main_window.m_settings.m_undecorate;
	if(undecorate)
	{
		string_handle const name = pe_get_export_name_undecorated(eti, exp_idx_sorted);
		if(!name.m_string)
		{
			return s_export_name_na;
		}
		else if(name.m_string == get_export_name_processing().m_string)
		{
			return s_export_name_processing;
		}
		else if(name.m_string == get_name_undecorating().m_string)
		{
			return s_export_name_undecorating;
		}
		else
		{
			return m_string_converter.convert(name);
		}
	}
	else
	{
		string_handle const name = pe_get_export_name(eti, exp_idx_sorted);
		if(!name.m_string)
		{
			return s_export_name_na;
		}
		else if(name.m_string == get_export_name_processing().m_string)
		{
			return s_export_name_processing;
		}
		else
		{
			return m_string_converter.convert(name);
		}
	}
}

wchar_t const* export_view::on_get_col_address(pe_export_table_info const& eti, std::uint16_t const exp_idx)
{
	std::uint16_t const& exp_idx_sorted = m_sort.empty() ? exp_idx : m_sort[exp_idx];
	bool const is_rva = pe_get_export_type(eti, exp_idx_sorted);
	pe_rva_or_forwarder const entry_point = pe_get_export_entry_point(eti, exp_idx_sorted);
	if(is_rva)
	{
		return rva_to_string(entry_point.m_rva, m_string_converter);
	}
	else
	{
		return m_string_converter.convert(entry_point.m_forwarder);
	}
}

void export_view::select_matching_instance()
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
	std::uint16_t const matched_imp = (fi->m_matched_imports != nullptr) ? fi->m_matched_imports[item_idx] : static_cast<std::uint16_t>(0xFFFF);
	if(matched_imp == 0xFFFF)
	{
		return;
	}
	m_main_window.m_import_view.select_item(matched_imp);
}

int export_view::get_type_column_max_width()
{
	if(g_export_type_column_max_width != 0)
	{
		return g_export_type_column_max_width;
	}

	HDC const dc = GetDC(m_hwnd);
	assert(dc != NULL);
	smart_dc const sdc(m_hwnd, dc);
	auto const orig_font = SelectObject(dc, reinterpret_cast<HFONT>(SendMessageW(m_hwnd, WM_GETFONT, 0, 0)));
	auto const fn_revert = mk::make_scope_exit([&](){ SelectObject(dc, orig_font); });

	int maximum = 0;
	SIZE size;

	BOOL const got1 = GetTextExtentPointW(dc, s_export_type_true, static_cast<int>(std::size(s_export_type_true)) - 1, &size);
	assert(got1 != 0);
	maximum = (std::max)(maximum, static_cast<int>(size.cx));

	BOOL const got2 = GetTextExtentPointW(dc, s_export_type_false, static_cast<int>(std::size(s_export_type_false)) - 1, &size);
	assert(got2 != 0);
	maximum = (std::max)(maximum, static_cast<int>(size.cx));

	static constexpr int const s_trailing_label_padding = 12;
	g_export_type_column_max_width = maximum + s_trailing_label_padding;
	return g_export_type_column_max_width;
}
