#include "modules_view.h"

#include "constants.h"
#include "file_info_getters.h"
#include "list_view_base.h"
#include "main.h"
#include "main_window.h"

#include "../nogui/cassert_my.h"
#include "../nogui/scope_exit.h"
#include "../nogui/utils.h"

#include <algorithm>
#include <iterator>
#include <numeric>

#include "../nogui/my_windows.h"

#include <commctrl.h>


enum class e_modules_menu_id : std::uint16_t
{
	e_matching = s_modules_view_menu_min,
	e_properties,
};
enum class e_modules_column
{
	e_name,
	e_path,
};
static constexpr wchar_t const* const s_modules_headers[] =
{
	L"name",
	L"path",
};
static constexpr wchar_t const s_modules_menu_str_matching[] = L"&Highlight Matching Module In Tree\tCtrl+M";
static constexpr wchar_t const s_modules_menu_str_properties[] = L"&Properties...\tAlt+Enter";


modules_view::modules_view(HWND const parent, main_window& mw) :
	m_hwnd(CreateWindowExW(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE, WC_LISTVIEWW, nullptr, WS_VISIBLE | WS_CHILD | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_OWNERDATA, 0, 0, 0, 0, parent, nullptr, get_instance(), nullptr)),
	m_main_window(mw),
	m_menu(create_menu()),
	m_string_converter(),
	m_sort_direction(0xFF),
	m_sort()
{
	assert(parent != nullptr);
	assert(m_hwnd != nullptr);
	static constexpr unsigned const extended_lv_styles = LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP | LVS_EX_DOUBLEBUFFER;
	LRESULT const set_ex_style = SendMessageW(m_hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, extended_lv_styles, extended_lv_styles);
	for(int i = 0; i != static_cast<int>(std::size(s_modules_headers)); ++i)
	{
		LVCOLUMNW cl;
		cl.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		cl.fmt = LVCFMT_LEFT;
		cl.cx = 200;
		cl.pszText = const_cast<LPWSTR>(s_modules_headers[i]);
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
}

modules_view::~modules_view()
{
}

HWND const& modules_view::get_hwnd() const
{
	return m_hwnd;
}

void modules_view::on_notify(NMHDR& nmhdr)
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
		case LVN_ITEMCHANGED:
		{
			on_itemchanged(nmhdr);
		}
		break;
	}
}

void modules_view::refresh()
{
	LRESULT const redr_off = SendMessageW(m_hwnd, WM_SETREDRAW, FALSE, 0);
	LRESULT const deleted = SendMessageW(m_hwnd, LVM_DELETEALLITEMS, 0, 0);
	assert(deleted == TRUE);
	auto const redr_on = mk::make_scope_exit([&]()
	{
		LRESULT const redr_on = SendMessageW(m_hwnd, WM_SETREDRAW, TRUE, 0);
		repaint();
	});
	sort_view();
	auto const& count = m_main_window.m_mo.m_modules_list.m_count;
	LRESULT const set_count = SendMessageW(m_hwnd, LVM_SETITEMCOUNT, count, 0);
	assert(set_count != 0);
}

void modules_view::repaint()
{
	BOOL const redrawn = RedrawWindow(m_hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_FRAME);
	assert(redrawn != 0);
}

file_info const* modules_view::get_selection()
{
	int const sel = list_view_base::get_selection(&m_hwnd);
	if(sel == -1)
	{
		return nullptr;
	}
	assert(sel >= 0 && sel <= 0xFFFF);
	std::uint16_t const line_idx = static_cast<std::uint16_t>(sel);
	std::uint16_t const item_idx = m_sort.empty() ? line_idx : m_sort[line_idx];
	assert(item_idx < m_main_window.m_mo.m_modules_list.m_count);
	file_info const* const fi = m_main_window.m_mo.m_modules_list.m_list[item_idx];
	assert(fi);
	assert(!fi->m_orig_instance);
	return fi;
}

void modules_view::select_item(file_info const* const& fi)
{
	assert(fi);
	auto const b = m_main_window.m_mo.m_modules_list.m_list;
	auto const e = b + m_main_window.m_mo.m_modules_list.m_count;
	auto const it = std::find(b, e, fi);
	assert(it != e);
	auto const idx_ = it - b;
	assert(idx_ >= 0 && idx_ <= 0xFFFF);
	std::uint16_t const idx = static_cast<std::uint16_t>(idx_);
	list_view_base::select_item(&m_hwnd, &m_sort, idx);
}

void modules_view::on_context_menu(LPARAM const lparam)
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
	assert(item_idx < m_main_window.m_mo.m_modules_list.m_count);
	file_info const* const fi = m_main_window.m_mo.m_modules_list.m_list[item_idx];
	assert(fi);
	assert(!fi->m_orig_instance);
	bool const enable_properties = !!fi->m_file_path;
	HMENU const menu = reinterpret_cast<HMENU>(m_menu.get());
	BOOL const enabled = EnableMenuItem(menu, static_cast<std::uint16_t>(e_modules_menu_id::e_properties), MF_BYCOMMAND | (enable_properties ? MF_ENABLED : MF_GRAYED));
	assert(enabled != -1 && (enabled == MF_ENABLED || enabled == MF_GRAYED));
	BOOL const tracked = TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON | TPM_NOANIMATION, screen_pos.x, screen_pos.y, 0, m_main_window.m_hwnd, nullptr);
	assert(tracked != 0);
}

void modules_view::on_menu(std::uint16_t const menu_id)
{
	e_modules_menu_id const e_menu = static_cast<e_modules_menu_id>(menu_id);
	switch(e_menu)
	{
		case e_modules_menu_id::e_matching:
		{
			on_menu_matching();
		}
		break;
		case e_modules_menu_id::e_properties:
		{
			on_menu_properties();
		}
		break;
		default:
		{
			assert(false);
		}
		break;
	}
}

void modules_view::on_accel_matching()
{
	matching();
}

void modules_view::on_accel_properties()
{
	properties();
}

smart_menu modules_view::create_menu()
{
	static constexpr std::uint16_t const menu_ids[] =
	{
		static_cast<std::uint16_t>(e_modules_menu_id::e_matching),
		static_cast<std::uint16_t>(e_modules_menu_id::e_properties),
	};
	static constexpr wchar_t const* const menu_strs[] =
	{
		s_modules_menu_str_matching,
		s_modules_menu_str_properties,
	};
	static_assert(std::size(menu_ids) == std::size(menu_strs), "");

	HMENU const tree_menu = CreatePopupMenu();
	assert(tree_menu);
	smart_menu sm{tree_menu};
	for(int i = 0; i != static_cast<int>(std::size(menu_ids)); ++i)
	{
		MENUITEMINFOW mi{};
		mi.cbSize = sizeof(mi);
		mi.fMask = MIIM_ID | MIIM_STRING | MIIM_FTYPE;
		mi.fType = MFT_STRING;
		mi.wID = menu_ids[i];
		mi.dwTypeData = const_cast<wchar_t*>(menu_strs[i]);
		BOOL const inserted = InsertMenuItemW(tree_menu, i, TRUE, &mi);
		assert(inserted != 0);
	}
	return sm;
}

void modules_view::on_getdispinfow(NMHDR& nmhdr)
{
	NMLVDISPINFOW& nm = reinterpret_cast<NMLVDISPINFOW&>(nmhdr);
	int const row = nm.item.iItem;
	int const col = nm.item.iSubItem;
	assert(row >= 0 && row <= 0xFFFF);
	assert(col >= 0 && col <= static_cast<int>(e_modules_column::e_path));
	auto const ecol = static_cast<e_modules_column>(col);
	if((nm.item.mask & LVIF_TEXT) != 0)
	{
		switch(ecol)
		{
			case e_modules_column::e_name:
			{
				wstring const str = on_get_col_name(static_cast<std::uint16_t>(row));
				assert(str);
				nm.item.pszText = const_cast<wchar_t*>(str.m_str);
			}
			break;
			case e_modules_column::e_path:
			{
				wstring const str = on_get_col_path(static_cast<std::uint16_t>(row));
				assert(str);
				nm.item.pszText = const_cast<wchar_t*>(str.m_str);
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

void modules_view::on_columnclick(NMHDR& nmhdr)
{
	NMLISTVIEW const& nmlv = reinterpret_cast<NMLISTVIEW&>(nmhdr);
	int const new_sort = list_view_base::on_columnclick(&nmlv, static_cast<int>(std::size(s_modules_headers)), m_sort_direction);
	assert(new_sort <= 0xFF);
	m_sort_direction = static_cast<std::uint8_t>(new_sort);
	refresh_headers();
	sort_view();
	repaint();
}

void modules_view::on_itemchanged([[maybe_unused]] NMHDR& nmhdr)
{
	m_main_window.on_modules_itemchanged();
}

wstring modules_view::on_get_col_name(std::uint16_t const& row)
{
	std::uint16_t const idx_unsorted = m_sort.empty() ? row : m_sort[row];
	wstring const ret = on_get_col_name_unsorted(idx_unsorted);
	return ret;
}

wstring modules_view::on_get_col_path(std::uint16_t const& row)
{
	std::uint16_t const idx_unsorted = m_sort.empty() ? row : m_sort[row];
	wstring const ret = on_get_col_path_unsorted(idx_unsorted);
	return ret;
}

wstring modules_view::on_get_col_name_unsorted(std::uint16_t const& row)
{
	assert(row < m_main_window.m_mo.m_modules_list.m_count);
	file_info const* const fi = m_main_window.m_mo.m_modules_list.m_list[row];
	assert(!fi->m_orig_instance);
	if(fi->m_file_path)
	{
		wchar_t const* const name = find_file_name(begin(fi->m_file_path), size(fi->m_file_path));
		assert(name != begin(fi->m_file_path));
		int const len = size(fi->m_file_path) - static_cast<int>(name - begin(fi->m_file_path));
		wstring const ret{name, len};
		return ret;
	}
	else
	{
		string_handle const& dll_name_a = get_dll_name_no_path(fi);
		wchar_t const* const dll_name_w = m_string_converter.convert(dll_name_a);
		assert(dll_name_w);
		wstring const ret{dll_name_w, size(dll_name_a)};
		return ret;
	}
}

wstring modules_view::on_get_col_path_unsorted(std::uint16_t const& row)
{
	assert(row < m_main_window.m_mo.m_modules_list.m_count);
	file_info const* const fi = m_main_window.m_mo.m_modules_list.m_list[row];
	assert(!fi->m_orig_instance);
	if(fi->m_file_path)
	{
		wstring const& ret = *fi->m_file_path.m_string;
		return ret;
	}
	else
	{
		wstring const ret{L"", 0};
		return ret;
	}
}

void modules_view::on_menu_matching()
{
	matching();
}

void modules_view::on_menu_properties()
{
	properties();
}

void modules_view::matching()
{
	int const sel = list_view_base::get_selection(&m_hwnd);
	if(sel == -1)
	{
		return;
	}
	assert(sel >= 0 && sel <= 0xFFFF);
	std::uint16_t const line_idx = static_cast<std::uint16_t>(sel);
	std::uint16_t const item_idx = m_sort.empty() ? line_idx : m_sort[line_idx];

	assert(item_idx < m_main_window.m_mo.m_modules_list.m_count);
	file_info const* const fi = m_main_window.m_mo.m_modules_list.m_list[item_idx];
	assert(fi);
	HTREEITEM const item = reinterpret_cast<HTREEITEM>(fi->m_tree_item);

	HWND const tree = m_main_window.m_tree_view.get_hwnd();
	LRESULT const visibled = SendMessageW(tree, TVM_ENSUREVISIBLE, 0, reinterpret_cast<LPARAM>(item));
	LRESULT const selected = SendMessageW(tree, TVM_SELECTITEM, TVGN_CARET, reinterpret_cast<LPARAM>(item));
	assert(selected != FALSE);
	SetFocus(tree);
}

void modules_view::properties()
{
	wstring_handle fp{};
	int const sel = list_view_base::get_selection(&m_hwnd);
	if(sel != -1)
	{
		assert(sel >= 0 && sel <= 0xFFFF);
		std::uint16_t const line_idx = static_cast<std::uint16_t>(sel);
		std::uint16_t const item_idx = m_sort.empty() ? line_idx : m_sort[line_idx];
		assert(item_idx < m_main_window.m_mo.m_modules_list.m_count);
		file_info const* const fi = m_main_window.m_mo.m_modules_list.m_list[item_idx];
		assert(fi);
		assert(!fi->m_orig_instance);
		fp = fi->m_file_path;
	}
	m_main_window.properties(fp);
}

void modules_view::refresh_headers()
{
	list_view_base::refresh_headers(&m_hwnd, static_cast<int>(std::size(s_modules_headers)), m_sort_direction);
}

void modules_view::sort_view()
{
	modules_list_t& modules_list = m_main_window.m_mo.m_modules_list;
	std::uint8_t const cur_sort_raw = m_sort_direction;
	if(cur_sort_raw == 0xFF)
	{
		m_sort.clear();
	}
	else
	{
		std::uint16_t const& n_items = modules_list.m_count;
		if(static_cast<int>(m_sort.size()) != n_items * 2)
		{
			m_sort.resize(n_items * 2);
			std::iota(m_sort.begin(), m_sort.begin() + n_items, std::uint16_t{0});
		}
		std::uint16_t* const sort = m_sort.data();
		bool const cur_sort_asc = (cur_sort_raw & (1u << 7u)) == 0u;
		std::uint8_t const cur_sort_col = cur_sort_raw &~ (1u << 7u);
		assert(cur_sort_col <= static_cast<std::uint8_t>(e_modules_column::e_path));
		e_modules_column const col = static_cast<e_modules_column>(cur_sort_col);
		switch(col)
		{
			case e_modules_column::e_name:
			{
				auto const fn_compare_name = [&](std::uint16_t const& a, std::uint16_t const& b) -> bool
				{
					wstring const val_a = on_get_col_name_unsorted(a);
					wstring const val_b = on_get_col_name_unsorted(b);
					auto const ret = wstring_case_insensitive_less{}(val_a, val_b);
					return ret;
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
			case e_modules_column::e_path:
			{
				auto const fn_compare_path = [&](std::uint16_t const& a, std::uint16_t const& b) -> bool
				{
					wstring const val_a = on_get_col_path_unsorted(a);
					wstring const val_b = on_get_col_path_unsorted(b);
					auto const ret = wstring_case_insensitive_less{}(val_a, val_b);
					return ret;
				};
				if(cur_sort_asc)
				{
					std::stable_sort(sort, sort + n_items, [&](auto const& a, auto const& b){ return fn_compare_path(a, b); });
				}
				else
				{
					std::stable_sort(sort, sort + n_items, [&](auto const& a, auto const& b){ return fn_compare_path(b, a); });
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
}
