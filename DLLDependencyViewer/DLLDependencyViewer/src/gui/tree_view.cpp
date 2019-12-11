#include "tree_view.h"

#include "constants.h"
#include "main.h"
#include "main_window.h"
#include "processor.h"

#include "../nogui/utils.h"

#include "../res/resources.h"

#include <cassert>
#include <cstdint>

#include <commctrl.h>
#include <windowsx.h>
#include <shellapi.h>


enum class e_tree_menu_id : std::uint16_t
{
	e_orig = s_tree_view_menu_min,
	e_expand,
	e_collapse,
	e_properties
};
static constexpr wchar_t const s_tree_menu_str_orig[] = L"Highlight &Original Instance\tCtrl+K";
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
	HIMAGELIST const tree_img_list = ImageList_LoadImageW(get_instance(), MAKEINTRESOURCEW(s_res_icons_tree), 26, 0, CLR_DEFAULT, IMAGE_BITMAP, LR_DEFAULTCOLOR);
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
	file_info_2 const& tmp_fi = *reinterpret_cast<file_info_2*>(di.item.lParam);
	file_info_2 const& fi = tmp_fi.m_orig_instance ? *tmp_fi.m_orig_instance : tmp_fi;
	file_info_2 const* parent_fi = nullptr;
	HTREEITEM const parent_item = reinterpret_cast<HTREEITEM>(SendMessageW(m_hwnd, TVM_GETNEXTITEM, TVGN_PARENT, reinterpret_cast<LPARAM>(di.item.hItem)));
	if(parent_item)
	{
		TVITEMEXW ti;
		ti.hItem = parent_item;
		ti.mask = TVIF_PARAM;
		LRESULT const got = SendMessageW(m_hwnd, TVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&ti));
		assert(got == TRUE);
		parent_fi = reinterpret_cast<file_info_2*>(ti.lParam);
	}
	if((di.item.mask & TVIF_TEXT) != 0)
	{
		bool const full_paths = m_main_window.m_settings.m_full_paths;
		if(full_paths && fi.m_file_path != get_not_found_string())
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
		bool delay;
		if(parent_fi)
		{
			auto const idx_ = &tmp_fi - parent_fi->m_fis;
			assert(idx_ >= 0 && idx_ <= 0xFFFF);
			std::uint16_t const idx = static_cast<std::uint16_t>(idx_);
			delay = idx >= parent_fi->m_import_table.m_non_delay_dll_count;
		}
		else
		{
			delay = false;
		}
		bool const is_32_bit = fi.m_is_32_bit;
		bool const is_duplicate = tmp_fi.m_orig_instance != nullptr;
		bool const is_missing = fi.m_file_path == get_not_found_string();
		bool const is_delay = delay;
		if(is_missing)
		{
			if(is_delay)
			{
				di.item.iImage = s_res_icon_missing_delay;
			}
			else
			{
				di.item.iImage = s_res_icon_missing;
			}
		}
		else
		{
			if(is_duplicate)
			{
				if(is_32_bit)
				{
					if(is_delay)
					{
						di.item.iImage = s_res_icon_duplicate_delay;
					}
					else
					{
						di.item.iImage = s_res_icon_duplicate;
					}
				}
				else
				{
					if(is_delay)
					{
						di.item.iImage = s_res_icon_duplicate_delay_64;
					}
					else
					{
						di.item.iImage = s_res_icon_duplicate_64;
					}
				}
			}
			else
			{
				if(is_32_bit)
				{
					if(is_delay)
					{
						di.item.iImage = s_res_icon_normal_delay;
					}
					else
					{
						di.item.iImage = s_res_icon_normal;
					}
				}
				else
				{
					if(is_delay)
					{
						di.item.iImage = s_res_icon_normal_delay_64;
					}
					else
					{
						di.item.iImage = s_res_icon_normal_64;
					}
				}
			}
		}
		di.item.iSelectedImage = di.item.iImage;
	}
}

void tree_view::on_selchangedw([[maybe_unused]] NMHDR& nmhdr)
{
	m_main_window.on_tree_selchangedw();
}

void tree_view::on_context_menu(LPARAM const lparam)
{
	POINT cursor_screen;
	HTREEITEM item;
	if(lparam == LPARAM{-1})
	{
		HTREEITEM const selected = reinterpret_cast<HTREEITEM>(SendMessageW(m_hwnd, TVM_GETNEXTITEM, TVGN_CARET, 0));
		if(!selected)
		{
			return;
		}
		RECT rect;
		*reinterpret_cast<HTREEITEM*>(&rect) = selected;
		LRESULT const got_rect = SendMessageW(m_hwnd, TVM_GETITEMRECT, TRUE, reinterpret_cast<LPARAM>(&rect));
		if(got_rect == FALSE)
		{
			return;
		}
		cursor_screen.x = rect.left + (rect.right - rect.left) / 2;
		cursor_screen.y = rect.top + (rect.bottom - rect.top) / 2;
		BOOL const converted = ClientToScreen(m_hwnd, &cursor_screen);
		assert(converted != 0);
		item = selected;
	}
	else
	{
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
			item = hti.hItem;
		}
		else
		{
			item = nullptr;
		}
	}
	HMENU const menu = reinterpret_cast<HMENU>(m_menu.get());
	bool enable_goto_orig;
	bool enable_properties;
	if(item)
	{
		TVITEMEXW ti;
		ti.hItem = item;
		ti.mask = TVIF_PARAM;
		LRESULT const got_item = SendMessageW(m_hwnd, TVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&ti));
		assert(got_item == TRUE);
		file_info_2 const& tmp_fi = *reinterpret_cast<file_info_2*>(ti.lParam);
		file_info_2 const& fi = tmp_fi.m_orig_instance ? *tmp_fi.m_orig_instance : tmp_fi;
		enable_goto_orig = tmp_fi.m_orig_instance != nullptr;
		enable_properties = !!fi.m_file_path && fi.m_file_path != get_not_found_string();
	}
	else
	{
		enable_goto_orig = false;
		enable_properties = false;
	}
	{
		BOOL const enabled_orig = EnableMenuItem(menu, static_cast<std::uint16_t>(e_tree_menu_id::e_orig), MF_BYCOMMAND | (enable_goto_orig ? MF_ENABLED : MF_GRAYED));
		assert(enabled_orig != -1 && (enabled_orig == MF_ENABLED || enabled_orig == MF_GRAYED));
	}
	{
		BOOL const enabled_properties = EnableMenuItem(menu, static_cast<std::uint16_t>(e_tree_menu_id::e_properties), MF_BYCOMMAND | (enable_properties ? MF_ENABLED : MF_GRAYED));
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
		case e_tree_menu_id::e_orig:
		{
			on_menu_orig();
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

void tree_view::on_menu_orig()
{
	select_original_instance();
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

void tree_view::on_accel_orig()
{
	select_original_instance();
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

void tree_view::on_toolbar_properties()
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

	file_info_2& fi = m_main_window.m_mo.m_fi;
	std::uint16_t const n = fi.m_import_table.m_dll_count;
	assert(n >= 1);
	for(std::uint16_t i = 0; i != n; ++i)
	{
		file_info_2& sub_fi = fi.m_fis[i];
		refresh_view_recursive(sub_fi, TVI_ROOT);
	}

	for(std::uint16_t i = 0; i != n; ++i)
	{
		file_info_2& sub_fi = fi.m_fis[i];
		LRESULT const expanded = SendMessageW(m_hwnd, TVM_EXPAND, TVE_EXPAND, reinterpret_cast<LPARAM>(sub_fi.m_tree_item));
	}
	HTREEITEM const first = static_cast<HTREEITEM>(fi.m_fis[0].m_tree_item);
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

smart_menu tree_view::create_menu()
{
	static constexpr std::uint16_t const menu_ids[] =
	{
		static_cast<std::uint16_t>(e_tree_menu_id::e_orig),
		static_cast<std::uint16_t>(e_tree_menu_id::e_expand),
		static_cast<std::uint16_t>(e_tree_menu_id::e_collapse),
		static_cast<std::uint16_t>(e_tree_menu_id::e_properties)
	};
	static constexpr wchar_t const* const menu_strs[] =
	{
		s_tree_menu_str_orig,
		s_tree_menu_str_expand,
		s_tree_menu_str_collapse,
		s_tree_menu_str_properties
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

void tree_view::refresh_view_recursive(file_info_2& fi, void* const parent_ti)
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
	fi.m_tree_item = ti;
	m_main_window.request_symbols_from_addresses(fi);
	m_main_window.request_symbol_undecoration(fi);
	std::uint16_t const n = fi.m_import_table.m_dll_count;
	for(std::uint16_t i = 0; i != n; ++i)
	{
		file_info_2& sub_fi = fi.m_fis[i];
		refresh_view_recursive(sub_fi, ti);
	}
}

void tree_view::select_original_instance()
{
	HTREEITEM const selected = reinterpret_cast<HTREEITEM>(SendMessageW(m_hwnd, TVM_GETNEXTITEM, TVGN_CARET, 0));
	if(!selected)
	{
		return;
	}
	TVITEMW ti;
	ti.hItem = selected;
	ti.mask = TVIF_PARAM;
	LRESULT const got_item = SendMessageW(m_hwnd, TVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&ti));
	assert(got_item == TRUE);
	file_info_2 const& fi = *reinterpret_cast<file_info_2*>(ti.lParam);
	if(!fi.m_orig_instance)
	{
		return;
	}
	LRESULT const orig_selected = SendMessageW(m_hwnd, TVM_SELECTITEM, TVGN_CARET, reinterpret_cast<LPARAM>(fi.m_orig_instance->m_tree_item));
	assert(orig_selected == TRUE);
}

void tree_view::expand()
{
	static constexpr auto const expand_all = [](HWND const hwnd, HTREEITEM const root_first) -> void
	{
		static constexpr auto const recursion = [](auto const& self, HWND const hwnd, HTREEITEM const item) -> void
		{
			HTREEITEM const child_first = reinterpret_cast<HTREEITEM>(SendMessageW(hwnd, TVM_GETNEXTITEM, TVGN_CHILD, reinterpret_cast<LPARAM>(item)));
			if(child_first)
			{
				LRESULT const expanded = SendMessageW(hwnd, TVM_EXPAND, TVE_EXPAND, reinterpret_cast<LPARAM>(item));
				assert(expanded != 0);
				self(self, hwnd, child_first);
			}
			HTREEITEM child_prev = child_first;
			for(;;)
			{
				HTREEITEM const child_next = reinterpret_cast<HTREEITEM>(SendMessageW(hwnd, TVM_GETNEXTITEM, TVGN_NEXT, reinterpret_cast<LPARAM>(child_prev)));
				if(child_next)
				{
					child_prev = child_next;
					self(self, hwnd, child_next);
				}
				else
				{
					break;
				}
			}
		};
		recursion(recursion, hwnd, root_first);
		HTREEITEM root_prev = root_first;
		for(;;)
		{
			HTREEITEM const root_next = reinterpret_cast<HTREEITEM>(SendMessageW(hwnd, TVM_GETNEXTITEM, TVGN_NEXT, reinterpret_cast<LPARAM>(root_prev)));
			if(root_next)
			{
				root_prev = root_next;
				recursion(recursion, hwnd, root_next);
			}
			else
			{
				break;
			}
		}
	};
	HTREEITEM const root_first = reinterpret_cast<HTREEITEM>(SendMessageW(m_hwnd, TVM_GETNEXTITEM, TVGN_ROOT, LPARAM{0}));
	if(!root_first)
	{
		return;
	}
	HTREEITEM const selection = reinterpret_cast<HTREEITEM>(SendMessageW(m_hwnd, TVM_GETNEXTITEM, TVGN_CARET, LPARAM{0}));
	LRESULT const redr_off = SendMessageW(m_hwnd, WM_SETREDRAW, FALSE, 0);
	expand_all(m_hwnd, root_first);
	LRESULT const redr_on = SendMessageW(m_hwnd, WM_SETREDRAW, TRUE, 0);
	if(selection)
	{
		LRESULT const visibled = SendMessageW(m_hwnd, TVM_ENSUREVISIBLE, 0, reinterpret_cast<LPARAM>(selection));
	}
	repaint();
}

void tree_view::collapse()
{
	static constexpr auto const collapse_all = [](HWND const hwnd, HTREEITEM const root_first) -> void
	{
		static constexpr auto const recursion = [](auto const& self, HWND const hwnd, HTREEITEM const item) -> void
		{
			HTREEITEM const child_first = reinterpret_cast<HTREEITEM>(SendMessageW(hwnd, TVM_GETNEXTITEM, TVGN_CHILD, reinterpret_cast<LPARAM>(item)));
			if(child_first)
			{
				self(self, hwnd, child_first);
				LRESULT const collapsed = SendMessageW(hwnd, TVM_EXPAND, TVE_COLLAPSE, reinterpret_cast<LPARAM>(item));
			}
			HTREEITEM child_prev = child_first;
			for(;;)
			{
				HTREEITEM const child_next = reinterpret_cast<HTREEITEM>(SendMessageW(hwnd, TVM_GETNEXTITEM, TVGN_NEXT, reinterpret_cast<LPARAM>(child_prev)));
				if(child_next)
				{
					child_prev = child_next;
					self(self, hwnd, child_next);
				}
				else
				{
					break;
				}
			}
		};
		recursion(recursion, hwnd, root_first);
		HTREEITEM root_prev = root_first;
		for(;;)
		{
			HTREEITEM const root_next = reinterpret_cast<HTREEITEM>(SendMessageW(hwnd, TVM_GETNEXTITEM, TVGN_NEXT, reinterpret_cast<LPARAM>(root_prev)));
			if(root_next)
			{
				root_prev = root_next;
				recursion(recursion, hwnd, root_next);
			}
			else
			{
				break;
			}
		}
	};
	HTREEITEM const root_first = reinterpret_cast<HTREEITEM>(SendMessageW(m_hwnd, TVM_GETNEXTITEM, TVGN_ROOT, LPARAM{0}));
	if(!root_first)
	{
		return;
	}
	LRESULT const selected = SendMessageW(m_hwnd, TVM_SELECTITEM, TVGN_CARET, reinterpret_cast<LPARAM>(root_first));
	assert(selected == TRUE);
	LRESULT const redr_off = SendMessageW(m_hwnd, WM_SETREDRAW, FALSE, 0);
	collapse_all(m_hwnd, root_first);
	LRESULT const redr_on = SendMessageW(m_hwnd, WM_SETREDRAW, TRUE, 0);
	LRESULT const visibled = SendMessageW(m_hwnd, TVM_ENSUREVISIBLE, 0, reinterpret_cast<LPARAM>(root_first));
	repaint();
}

void tree_view::properties()
{
	HTREEITEM const selection = reinterpret_cast<HTREEITEM>(SendMessageW(m_hwnd, TVM_GETNEXTITEM, TVGN_CARET, LPARAM{0}));
	if(!selection)
	{
		return;
	}
	TVITEMEXW ti;
	ti.hItem = selection;
	ti.mask = TVIF_PARAM;
	LRESULT const got = SendMessageW(m_hwnd, TVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&ti));
	assert(got == TRUE);
	assert(ti.lParam);
	file_info_2 const& tmp_fi = *reinterpret_cast<file_info_2*>(ti.lParam);
	file_info_2 const& fi = tmp_fi.m_orig_instance ? *tmp_fi.m_orig_instance : tmp_fi;
	if(!fi.m_file_path || fi.m_file_path == get_not_found_string())
	{
		return;
	}
	SHELLEXECUTEINFOW info{};
	info.cbSize = sizeof(info);
	info.fMask = SEE_MASK_INVOKEIDLIST;
	info.lpVerb = L"properties";
	info.lpFile = fi.m_file_path.m_string->m_str;
	info.nShow = SW_SHOWNORMAL;
	BOOL const executed = ShellExecuteExW(&info);
	assert(executed != FALSE);
	assert(static_cast<int>(reinterpret_cast<std::uintptr_t>(info.hInstApp)) > 32);
}
