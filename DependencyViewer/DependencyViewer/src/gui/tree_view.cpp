#include "tree_view.h"

#include "common_controls.h"
#include "constants.h"
#include "main.h"
#include "main_window.h"
#include "tree_algos.h"

#include "../nogui/cassert_my.h"
#include "../nogui/utils.h"

#include "../res/resources.h"

#include <algorithm>
#include <cstdint>

#include "../nogui/my_windows.h"

#include <commctrl.h>
#include <shellapi.h>
#include <windowsx.h>


enum class e_tree_menu_id : std::uint16_t
{
	e_match = s_tree_view_menu_min,
	e_orig,
	e_prev,
	e_next,
	e_expand,
	e_collapse,
	e_properties,
};
static constexpr wchar_t const s_tree_menu_str_match[] = L"&Highlight Matching Module In List\tCtrl+M";
static constexpr wchar_t const s_tree_menu_str_orig[] = L"Highlight &Original Instance\tCtrl+K";
static constexpr wchar_t const s_tree_menu_str_prev[] = L"Highlight Previous &Instance\tCtrl+B";
static constexpr wchar_t const s_tree_menu_str_next[] = L"Highlight &Next Instance\tCtrl+N";
static constexpr wchar_t const s_tree_menu_str_expand[] = L"&Expand All\tCtrl+E";
static constexpr wchar_t const s_tree_menu_str_collapse[] = L"Co&llapse All\tCtrl+W";
static constexpr wchar_t const s_tree_menu_str_properties[] = L"&Properties...\tAlt+Enter";


tree_view::tree_view(HWND const parent, main_window& mw) :
	m_hwnd(CreateWindowExW(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE, WC_TREEVIEWW, nullptr, WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, parent, nullptr, get_instance(), nullptr)),
	m_main_window(mw),
	m_menu(create_menu()),
	m_string_converter()
{
	LRESULT const set_dbl_bfr = SendMessageW(m_hwnd, TVM_SETEXTENDEDSTYLE, TVS_EX_DOUBLEBUFFER, TVS_EX_DOUBLEBUFFER);
	assert(set_dbl_bfr == S_OK);
	[[maybe_unused]] LONG_PTR const prev = SetWindowLongPtrW(m_hwnd, GWL_STYLE, GetWindowLongPtrW(m_hwnd, GWL_STYLE) | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_SHOWSELALWAYS);
	HIMAGELIST const tree_img_list = common_controls::ImageList_LoadImageW(get_instance(), MAKEINTRESOURCEW(s_res_icons_tree), 26, 0, CLR_DEFAULT, IMAGE_BITMAP, LR_DEFAULTCOLOR);
	assert(tree_img_list);
	LRESULT const prev_img_list = SendMessageW(m_hwnd, TVM_SETIMAGELIST, TVSIL_NORMAL, reinterpret_cast<LPARAM>(tree_img_list));
	assert(!prev_img_list);
}

tree_view::~tree_view()
{
}

HWND tree_view::get_hwnd() const
{
	return m_hwnd;
}

void tree_view::on_notify(NMHDR& nmhdr)
{
	if(nmhdr.code == TVN_GETDISPINFOW)
	{
		on_getdispinfow(nmhdr);
	}
	else if(nmhdr.code == TVN_SELCHANGEDW)
	{
		on_selchangedw(nmhdr);
	}
}

void tree_view::on_getdispinfow(NMHDR& nmhdr)
{
	NMTVDISPINFOW& di = reinterpret_cast<NMTVDISPINFOW&>(nmhdr);
	file_info& tmp_fi = *reinterpret_cast<file_info*>(di.item.lParam);
	file_info const& fi = tmp_fi.m_orig_instance ? *tmp_fi.m_orig_instance : tmp_fi;
	file_info const* const parent_fi = tmp_fi.m_parent;
	if((di.item.mask & TVIF_TEXT) != 0)
	{
		bool const full_paths = m_main_window.m_settings.m_full_paths;
		if(full_paths && fi.m_file_path.m_string != nullptr)
		{
			di.item.pszText = const_cast<wchar_t*>(cbegin(fi.m_file_path));
		}
		else
		{
			if(parent_fi)
			{
				auto const idx_ = &tmp_fi - parent_fi->m_fis;
				assert(idx_ >= 0 && idx_ <= 0xFFFF);
				std::uint16_t const idx = static_cast<std::uint16_t>(idx_);
				string_handle const& my_name = parent_fi->m_import_table.m_dll_names[idx];
				di.item.pszText = const_cast<wchar_t*>(m_string_converter.convert(my_name));
			}
			else
			{
				wchar_t const* const file_name = find_file_name(cbegin(fi.m_file_path), size(fi.m_file_path));
				di.item.pszText = const_cast<wchar_t*>(file_name);
			}
		}
	}
	if((di.item.mask & (TVIF_IMAGE | TVIF_SELECTEDIMAGE)) != 0)
	{
		if(tmp_fi.m_icon == 0)
		{
			tmp_fi.m_icon = get_tree_item_icon(tmp_fi, parent_fi);
		}
		assert(tmp_fi.m_icon != 0);
		di.item.iImage = tmp_fi.m_icon - 1;
		di.item.iSelectedImage = di.item.iImage;
	}
}

void tree_view::on_selchangedw([[maybe_unused]] NMHDR& nmhdr)
{
	m_main_window.on_tree_selchangedw();
}

void tree_view::on_context_menu(LPARAM const lparam)
{
	file_info const* fi;
	POINT cursor_screen;
	bool const got_data = get_fi_and_point_for_context_menu(lparam, fi, cursor_screen);
	if(!got_data)
	{
		return;
	}
	HMENU const menu = reinterpret_cast<HMENU>(m_menu.get());
	{
		BOOL const enabled_match = EnableMenuItem(menu, static_cast<std::uint16_t>(e_tree_menu_id::e_match), MF_BYCOMMAND | ((fi && get_match_data(fi)) ? MF_ENABLED : MF_GRAYED));
		assert(enabled_match != -1 && (enabled_match == MF_ENABLED || enabled_match == MF_GRAYED));
	}
	{
		BOOL const enabled_orig = EnableMenuItem(menu, static_cast<std::uint16_t>(e_tree_menu_id::e_orig), MF_BYCOMMAND | ((fi && get_orig_data(fi)) ? MF_ENABLED : MF_GRAYED));
		assert(enabled_orig != -1 && (enabled_orig == MF_ENABLED || enabled_orig == MF_GRAYED));
	}
	{
		BOOL const enabled_orig = EnableMenuItem(menu, static_cast<std::uint16_t>(e_tree_menu_id::e_prev), MF_BYCOMMAND | ((fi && get_prev_data(fi)) ? MF_ENABLED : MF_GRAYED));
		assert(enabled_orig != -1 && (enabled_orig == MF_ENABLED || enabled_orig == MF_GRAYED));
	}
	{
		BOOL const enabled_orig = EnableMenuItem(menu, static_cast<std::uint16_t>(e_tree_menu_id::e_next), MF_BYCOMMAND | ((fi && get_next_data(fi)) ? MF_ENABLED : MF_GRAYED));
		assert(enabled_orig != -1 && (enabled_orig == MF_ENABLED || enabled_orig == MF_GRAYED));
	}
	{
		BOOL const enabled_properties = EnableMenuItem(menu, static_cast<std::uint16_t>(e_tree_menu_id::e_properties), MF_BYCOMMAND | ((fi && m_main_window.get_properties_data(fi)) ? MF_ENABLED : MF_GRAYED));
		assert(enabled_properties != -1 && (enabled_properties == MF_ENABLED || enabled_properties == MF_GRAYED));
	}
	BOOL const tracked = TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON | TPM_NOANIMATION, cursor_screen.x, cursor_screen.y, 0, m_main_window.m_hwnd, nullptr);
	assert(tracked != 0);
}

void tree_view::on_menu(std::uint16_t const menu_id)
{
	e_tree_menu_id const e_menu = static_cast<e_tree_menu_id>(menu_id);
	switch(e_menu)
	{
		case e_tree_menu_id::e_match:
		{
			on_menu_match();
		}
		break;
		case e_tree_menu_id::e_orig:
		{
			on_menu_orig();
		}
		break;
		case e_tree_menu_id::e_prev:
		{
			on_menu_prev();
		}
		break;
		case e_tree_menu_id::e_next:
		{
			on_menu_next();
		}
		break;
		case e_tree_menu_id::e_expand:
		{
			on_menu_expand();
		}
		break;
		case e_tree_menu_id::e_collapse:
		{
			on_menu_collapse();
		}
		break;
		case e_tree_menu_id::e_properties:
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

void tree_view::on_menu_match()
{
	select_match();
}

void tree_view::on_menu_orig()
{
	select_orig_instance();
}

void tree_view::on_menu_prev()
{
	select_prev_instance();
}

void tree_view::on_menu_next()
{
	select_next_instance();
}

void tree_view::on_menu_expand()
{
	expand();
}

void tree_view::on_menu_collapse()
{
	collapse();
}

void tree_view::on_menu_properties()
{
	properties();
}

void tree_view::on_accel_match()
{
	select_match();
}

void tree_view::on_accel_orig()
{
	select_orig_instance();
}

void tree_view::on_accel_prev()
{
	select_prev_instance();
}

void tree_view::on_accel_next()
{
	select_next_instance();
}

void tree_view::on_accel_expand()
{
	expand();
}

void tree_view::on_accel_collapse()
{
	collapse();
}

void tree_view::on_accel_properties()
{
	properties();
}

void tree_view::refresh()
{
	LRESULT const redr_off = SendMessageW(m_hwnd, WM_SETREDRAW, FALSE, 0);
	LRESULT const deselected = SendMessageW(m_hwnd, TVM_SELECTITEM, TVGN_CARET, reinterpret_cast<LPARAM>(nullptr));
	assert(deselected == TRUE);
	LRESULT const deleted = SendMessageW(m_hwnd, TVM_DELETEITEM, 0, reinterpret_cast<LPARAM>(TVI_ROOT));
	assert(deleted == TRUE);

	file_info& fi = *m_main_window.m_mo.m_fi;
	std::uint16_t const n = fi.m_import_table.m_normal_dll_count + fi.m_import_table.m_delay_dll_count;
	assert(n >= 1);
	for(std::uint16_t i = 0; i != n; ++i)
	{
		file_info& sub_fi = fi.m_fis[i];
		refresh_view_recursive(sub_fi, TVI_ROOT);
	}

	for(std::uint16_t i = 0; i != n; ++i)
	{
		file_info& sub_fi = fi.m_fis[i];
		LRESULT const expanded = SendMessageW(m_hwnd, TVM_EXPAND, TVE_EXPAND, reinterpret_cast<LPARAM>(sub_fi.m_tree_item));
	}
	HTREEITEM const first = reinterpret_cast<HTREEITEM>(fi.m_fis[0].m_tree_item);
	LRESULT const visibled = SendMessageW(m_hwnd, TVM_ENSUREVISIBLE, 0, reinterpret_cast<LPARAM>(first));
	LRESULT const selected = SendMessageW(m_hwnd, TVM_SELECTITEM, TVGN_CARET, reinterpret_cast<LPARAM>(first));
	assert(selected == TRUE);

	LRESULT const redr_on = SendMessageW(m_hwnd, WM_SETREDRAW, TRUE, 0);
	repaint();
}

void tree_view::repaint()
{
	BOOL const redrawn = RedrawWindow(m_hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_FRAME);
	assert(redrawn != 0);
}

file_info const* tree_view::get_selection()
{
	HTREEITEM const selected = reinterpret_cast<HTREEITEM>(SendMessageW(m_hwnd, TVM_GETNEXTITEM, TVGN_CARET, LPARAM{0}));
	if(!selected)
	{
		return nullptr;
	}
	file_info& ret = htreeitem_2_file_info(reinterpret_cast<htreeitem>(selected));
	return &ret;
}

smart_menu tree_view::create_menu()
{
	static constexpr std::uint16_t const menu_ids[] =
	{
		static_cast<std::uint16_t>(e_tree_menu_id::e_match),
		static_cast<std::uint16_t>(e_tree_menu_id::e_orig),
		static_cast<std::uint16_t>(e_tree_menu_id::e_prev),
		static_cast<std::uint16_t>(e_tree_menu_id::e_next),
		static_cast<std::uint16_t>(e_tree_menu_id::e_expand),
		static_cast<std::uint16_t>(e_tree_menu_id::e_collapse),
		static_cast<std::uint16_t>(e_tree_menu_id::e_properties),
	};
	static constexpr wchar_t const* const menu_strs[] =
	{
		s_tree_menu_str_match,
		s_tree_menu_str_orig,
		s_tree_menu_str_prev,
		s_tree_menu_str_next,
		s_tree_menu_str_expand,
		s_tree_menu_str_collapse,
		s_tree_menu_str_properties,
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

file_info& tree_view::htreeitem_2_file_info(htreeitem const& hti)
{
	TVITEMW ti;
	ti.hItem = reinterpret_cast<HTREEITEM>(hti);
	ti.mask = TVIF_PARAM;
	LRESULT const got_item = SendMessageW(m_hwnd, TVM_GETITEMW, WPARAM{0}, reinterpret_cast<LPARAM>(&ti));
	assert(got_item == TRUE);
	assert(ti.lParam);
	file_info& ret = *reinterpret_cast<file_info*>(ti.lParam);
	return ret;
}

bool tree_view::get_fi_and_point_for_context_menu(LPARAM const lparam, file_info const*& out_fi, POINT& out_point)
{
	if(lparam == LPARAM{-1})
	{
		file_info const* const selection = get_selection();
		if(!selection)
		{
			return false;
		}
		RECT rect;
		*reinterpret_cast<HTREEITEM*>(&rect) = reinterpret_cast<HTREEITEM>(selection->m_tree_item);
		LRESULT const got_rect = SendMessageW(m_hwnd, TVM_GETITEMRECT, TRUE, reinterpret_cast<LPARAM>(&rect));
		if(got_rect == FALSE)
		{
			return false;
		}
		POINT cursor_screen;
		cursor_screen.x = rect.left + (rect.right - rect.left) / 2;
		cursor_screen.y = rect.top + (rect.bottom - rect.top) / 2;
		BOOL const converted = ClientToScreen(m_hwnd, &cursor_screen);
		assert(converted != 0);
		out_point = cursor_screen;
		out_fi = selection;
		return true;
	}
	else
	{
		POINT cursor_screen;
		cursor_screen.x = GET_X_LPARAM(lparam);
		cursor_screen.y = GET_Y_LPARAM(lparam);
		POINT cursor_client = cursor_screen;
		BOOL const converted = ScreenToClient(m_hwnd, &cursor_client);
		assert(converted != 0);
		TVHITTESTINFO hti;
		hti.pt = cursor_client;
		[[maybe_unused]] HTREEITEM const hit_tested = reinterpret_cast<HTREEITEM>(SendMessageW(m_hwnd, TVM_HITTEST, 0, reinterpret_cast<LPARAM>(&hti)));
		assert(hit_tested == hti.hItem);
		if(hti.hItem && (hti.flags & (TVHT_ONITEM | TVHT_ONITEMBUTTON | TVHT_ONITEMICON | TVHT_ONITEMLABEL | TVHT_ONITEMSTATEICON)) != 0)
		{
			LRESULT const selected = SendMessageW(m_hwnd, TVM_SELECTITEM, TVGN_CARET, reinterpret_cast<LPARAM>(hti.hItem));
			assert(selected == TRUE);
			file_info& fi = htreeitem_2_file_info(reinterpret_cast<htreeitem>(hti.hItem));
			out_fi = &fi;
			out_point = cursor_screen;
			return true;
		}
		else
		{
			out_fi = nullptr;
			out_point = cursor_screen;
			return true;
		}
	}
}

std::uint8_t tree_view::get_tree_item_icon(file_info const& tmp_fi, file_info const* const parent_fi)
{
	assert(tmp_fi.m_icon == 0);
	static constexpr auto const fn_get_icon = [](file_info const& tmp_fi, file_info const* const parent_fi) -> std::uint8_t
	{
		std::uint8_t ret = 0;
		file_info const& fi = tmp_fi.m_orig_instance ? *tmp_fi.m_orig_instance : tmp_fi;
		bool is_delay;
		if(parent_fi)
		{
			auto const dll_idx_ = &tmp_fi - parent_fi->m_fis;
			assert(dll_idx_ >= 0 && dll_idx_ <= 0xFFFF);
			std::uint16_t const dll_idx = static_cast<std::uint16_t>(dll_idx_);
			is_delay = dll_idx >= parent_fi->m_import_table.m_normal_dll_count;
		}
		else
		{
			is_delay = false;
		}
		if(is_delay)
		{
			ret += 20;
		}
		else
		{
			ret += 0;
		}
		bool const is_missing = fi.m_file_path.m_string == nullptr;
		if(is_missing)
		{
			return ret + 0;
		}
		bool const is_32_bit = fi.m_is_32_bit;
		if(is_32_bit)
		{
			ret += 2;
		}
		else
		{
			ret += 6;
		}
		bool const is_duplicate = tmp_fi.m_orig_instance != nullptr;
		if(is_duplicate)
		{
			ret += 1;
		}
		else
		{
			ret += 0;
		}
		bool is_warning;
		if(parent_fi)
		{
			auto const dll_idx_ = &tmp_fi - parent_fi->m_fis;
			assert(dll_idx_ >= 0 && dll_idx_ <= 0xFFFF);
			std::uint16_t const dll_idx = static_cast<std::uint16_t>(dll_idx_);
			std::uint16_t const* const matched_exports = parent_fi->m_import_table.m_matched_exports[dll_idx];
			std::uint16_t const n = parent_fi->m_import_table.m_import_counts[dll_idx];
			auto const it = std::find(matched_exports, matched_exports + n, static_cast<std::uint16_t>(0xFFFF));
			is_warning = it != matched_exports + n;
		}
		else
		{
			is_warning = false;
		}
		if(is_warning)
		{
			ret += 2;
		}
		else
		{
			ret += 0;
		}
		return ret;
	};
	std::uint8_t const icon = fn_get_icon(tmp_fi, parent_fi);
	return icon + 1;
}

void tree_view::refresh_view_recursive(file_info& fi, void* const parent_ti)
{
	TVINSERTSTRUCTW tvi;
	tvi.hParent = reinterpret_cast<HTREEITEM>(parent_ti);
	tvi.hInsertAfter = TVI_LAST;
	tvi.itemex.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_PARAM | TVIF_SELECTEDIMAGE;
	tvi.itemex.hItem = nullptr;
	tvi.itemex.state = 0;
	tvi.itemex.stateMask = 0;
	tvi.itemex.pszText = LPSTR_TEXTCALLBACKW;
	tvi.itemex.cchTextMax = 0;
	tvi.itemex.iImage = I_IMAGECALLBACK;
	tvi.itemex.iSelectedImage = I_IMAGECALLBACK;
	tvi.itemex.cChildren = 0;
	tvi.itemex.lParam = reinterpret_cast<LPARAM>(&fi);
	tvi.itemex.iIntegral = 0;
	tvi.itemex.uStateEx = 0;
	tvi.itemex.hwnd = nullptr;
	tvi.itemex.iExpandedImage = 0;
	tvi.itemex.iReserved = 0;
	HTREEITEM const ti = reinterpret_cast<HTREEITEM>(SendMessageW(m_hwnd, TVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&tvi)));
	assert(ti != nullptr);
	fi.m_tree_item = reinterpret_cast<htreeitem>(ti);
	m_main_window.request_symbols_from_addresses(fi);
	m_main_window.request_symbol_undecoration(fi);
	std::uint16_t const n = fi.m_import_table.m_normal_dll_count + fi.m_import_table.m_delay_dll_count;
	for(std::uint16_t i = 0; i != n; ++i)
	{
		file_info& sub_fi = fi.m_fis[i];
		refresh_view_recursive(sub_fi, ti);
	}
}

void tree_view::select_match(htreeitem const data /* = nullptr */)
{
	htreeitem const dta = data ? data : get_match_data();
	if(!dta)
	{
		return;
	}
	file_info const& tmp_fi_ = htreeitem_2_file_info(dta);
	file_info const* const tmp_fi = &tmp_fi_;
	file_info const* const fi = tmp_fi->m_orig_instance ? tmp_fi->m_orig_instance : tmp_fi;
	m_main_window.m_modules_view.select_item(fi);
}

void tree_view::select_orig_instance(htreeitem const data /* = nullptr */)
{
	htreeitem const dta = data ? data : get_orig_data();
	if(!dta)
	{
		return;
	}
	LRESULT const orig_selected = SendMessageW(m_hwnd, TVM_SELECTITEM, TVGN_CARET, reinterpret_cast<LPARAM>(dta));
	assert(orig_selected == TRUE);
}

void tree_view::select_prev_instance(htreeitem const data /* = nullptr */)
{
	htreeitem const dta = data ? data : get_prev_data();
	if(!dta)
	{
		return;
	}
	LRESULT const orig_selected = SendMessageW(m_hwnd, TVM_SELECTITEM, TVGN_CARET, reinterpret_cast<LPARAM>(dta));
	assert(orig_selected == TRUE);
}

void tree_view::select_next_instance(htreeitem const data /* = nullptr */)
{
	htreeitem const dta = data ? data : get_next_data();
	if(!dta)
	{
		return;
	}
	LRESULT const orig_selected = SendMessageW(m_hwnd, TVM_SELECTITEM, TVGN_CARET, reinterpret_cast<LPARAM>(dta));
	assert(orig_selected == TRUE);
}

htreeitem tree_view::get_match_data(file_info const* const curr_fi /* = nullptr */)
{
	file_info const* fi;
	if(curr_fi)
	{
		fi = curr_fi;
	}
	else
	{
		file_info const* const f = get_selection();
		if(!f)
		{
			return nullptr;
		}
		fi = f;
	}
	assert(fi);
	return fi->m_tree_item;
}

htreeitem tree_view::get_orig_data(file_info const* const curr_fi /* = nullptr */)
{
	file_info const* fi;
	if(curr_fi)
	{
		fi = curr_fi;
	}
	else
	{
		file_info const* const f = get_selection();
		if(!f)
		{
			return nullptr;
		}
		fi = f;
	}
	assert(fi);
	if(!fi->m_orig_instance)
	{
		return nullptr;
	}
	return fi->m_orig_instance->m_tree_item;
}

htreeitem tree_view::get_prev_data(file_info const* const curr_fi /* = nullptr */)
{
	file_info const* fi;
	if(curr_fi)
	{
		fi = curr_fi;
	}
	else
	{
		file_info const* const f = get_selection();
		if(!f)
		{
			return nullptr;
		}
		fi = f;
	}
	assert(fi);
	if(!fi->m_orig_instance)
	{
		return nullptr;
	}
	return fi->m_prev_instance->m_tree_item;
}

htreeitem tree_view::get_next_data(file_info const* const curr_fi /* = nullptr */)
{
	file_info const* fi;
	if(curr_fi)
	{
		fi = curr_fi;
	}
	else
	{
		file_info const* const f = get_selection();
		if(!f)
		{
			return nullptr;
		}
		fi = f;
	}
	assert(fi);
	if(fi->m_next_instance == fi->m_orig_instance)
	{
		return nullptr;
	}
	return fi->m_next_instance->m_tree_item;
}

void tree_view::expand()
{
	static constexpr auto const expand_fn = [](file_info& fi, void* const data)
	{
		HWND const hwnd = static_cast<HWND>(data);
		HTREEITEM const& item = reinterpret_cast<HTREEITEM>(fi.m_tree_item);
		[[maybe_unused]] LRESULT collapsed = SendMessageW(hwnd, TVM_EXPAND, TVE_EXPAND, reinterpret_cast<LPARAM>(item));
	};
	HTREEITEM const root_first = reinterpret_cast<HTREEITEM>(SendMessageW(m_hwnd, TVM_GETNEXTITEM, TVGN_ROOT, LPARAM{0}));
	if(!root_first)
	{
		return;
	}
	HTREEITEM const selection = reinterpret_cast<HTREEITEM>(SendMessageW(m_hwnd, TVM_GETNEXTITEM, TVGN_CARET, LPARAM{0}));
	LRESULT const redr_off = SendMessageW(m_hwnd, WM_SETREDRAW, FALSE, 0);
	depth_first_visit(*m_main_window.m_mo.m_fi, expand_fn, static_cast<void*>(m_hwnd));
	LRESULT const redr_on = SendMessageW(m_hwnd, WM_SETREDRAW, TRUE, 0);
	if(selection)
	{
		LRESULT const visibled = SendMessageW(m_hwnd, TVM_ENSUREVISIBLE, 0, reinterpret_cast<LPARAM>(selection));
	}
	repaint();
}

void tree_view::collapse()
{
	static constexpr auto const collapse_fn = [](file_info& fi, void* const data)
	{
		HWND const hwnd = static_cast<HWND>(data);
		HTREEITEM const& item = reinterpret_cast<HTREEITEM>(fi.m_tree_item);
		[[maybe_unused]] LRESULT collapsed = SendMessageW(hwnd, TVM_EXPAND, TVE_COLLAPSE, reinterpret_cast<LPARAM>(item));
	};
	HTREEITEM const root_first = reinterpret_cast<HTREEITEM>(SendMessageW(m_hwnd, TVM_GETNEXTITEM, TVGN_ROOT, LPARAM{0}));
	if(!root_first)
	{
		return;
	}
	LRESULT const selected = SendMessageW(m_hwnd, TVM_SELECTITEM, TVGN_CARET, reinterpret_cast<LPARAM>(root_first));
	assert(selected == TRUE);
	LRESULT const redr_off = SendMessageW(m_hwnd, WM_SETREDRAW, FALSE, 0);
	children_first_visit(*m_main_window.m_mo.m_fi, collapse_fn, static_cast<void*>(m_hwnd));
	LRESULT const redr_on = SendMessageW(m_hwnd, WM_SETREDRAW, TRUE, 0);
	LRESULT const visibled = SendMessageW(m_hwnd, TVM_ENSUREVISIBLE, 0, reinterpret_cast<LPARAM>(root_first));
	repaint();
}

void tree_view::properties()
{
	wstring_handle fp{};
	file_info const* const fi = get_selection();
	if(fi)
	{
		file_info const* const real_fi = fi->m_orig_instance ? fi->m_orig_instance : fi;
		fp = real_fi->m_file_path;
	}
	m_main_window.properties(fp);
}
