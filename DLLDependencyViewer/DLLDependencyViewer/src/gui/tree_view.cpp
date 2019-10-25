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


enum class e_tree_menu_id : std::uint16_t
{
	e_orig = s_tree_view_menu_min,
};
static constexpr wchar_t const s_tree_menu_orig_str[] = L"Highlight &Original Instance\tCtrl+K";


tree_view::tree_view(HWND const parent, main_window& mw) :
	m_hwnd(CreateWindowExW(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE, WC_TREEVIEWW, nullptr, WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, parent, nullptr, get_instance(), nullptr)),
	m_main_window(mw),
	m_menu(create_menu()),
	m_tmp_strings(),
	m_tmp_string_idx()
{
	LRESULT const set_dbl_bfr = SendMessageW(m_hwnd, TVM_SETEXTENDEDSTYLE, TVS_EX_DOUBLEBUFFER, TVS_EX_DOUBLEBUFFER);
	assert(set_dbl_bfr == S_OK);
	LONG_PTR const prev = SetWindowLongPtrW(m_hwnd, GWL_STYLE, GetWindowLongPtrW(m_hwnd, GWL_STYLE) | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_SHOWSELALWAYS);
	(void)prev;
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
	file_info const& tmp_fi = *reinterpret_cast<file_info*>(di.item.lParam);
	file_info const& fi = tmp_fi.m_orig_instance ? *tmp_fi.m_orig_instance : tmp_fi;
	file_info const* parent_fi = nullptr;
	HTREEITEM const parent_item = reinterpret_cast<HTREEITEM>(SendMessageW(m_hwnd, TVM_GETNEXTITEM, TVGN_PARENT, reinterpret_cast<LPARAM>(di.item.hItem)));
	if(parent_item)
	{
		TVITEMEXW ti;
		ti.hItem = parent_item;
		ti.mask = TVIF_PARAM;
		LRESULT const got = SendMessageW(m_hwnd, TVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&ti));
		assert(got == TRUE);
		parent_fi = reinterpret_cast<file_info*>(ti.lParam);
	}
	if((di.item.mask & TVIF_TEXT) != 0)
	{
		bool const full_paths = m_main_window.m_settings.m_full_paths;
		if(full_paths && fi.m_file_path != get_not_found_string())
		{
			di.item.pszText = const_cast<wchar_t*>(fi.m_file_path->m_str);
		}
		else
		{
			if(parent_fi)
			{
				int const idx = static_cast<int>(&tmp_fi - parent_fi->m_sub_file_infos.data());
				string const* const my_name = parent_fi->m_import_table.m_dll_names[idx];
				std::wstring& tmp = m_tmp_strings[m_tmp_string_idx++ % m_tmp_strings.size()];
				tmp.resize(my_name->m_len);
				std::transform(my_name->m_str, my_name->m_str + my_name->m_len, tmp.begin(), [](char const& e) -> wchar_t { return static_cast<wchar_t>(e); });
				di.item.pszText = const_cast<wchar_t*>(tmp.c_str());
			}
			else
			{
				wchar_t const* const file_name = find_file_name(fi.m_file_path->m_str, fi.m_file_path->m_len);
				di.item.pszText = const_cast<wchar_t*>(file_name);
			}
		}
	}
	if((di.item.mask & (TVIF_IMAGE | TVIF_SELECTEDIMAGE)) != 0)
	{
		bool delay;
		if(parent_fi)
		{
			std::uint16_t const idx = static_cast<std::uint16_t>(&tmp_fi - parent_fi->m_sub_file_infos.data());
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

void tree_view::on_selchangedw(NMHDR& nmhdr)
{
	(void)nmhdr;
	m_main_window.on_tree_selchangedw();
}

void tree_view::on_context_menu(LPARAM const lparam)
{
	POINT cursor_screen;
	HTREEITEM item;
	if(lparam == 0xffffffff)
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
		HTREEITEM const hit_tested = reinterpret_cast<HTREEITEM>(SendMessageW(m_hwnd, TVM_HITTEST, 0, reinterpret_cast<LPARAM>(&hti)));
		(void)hit_tested;
		assert(hit_tested == hti.hItem);
		if(!(hti.hItem && (hti.flags & (TVHT_ONITEM | TVHT_ONITEMBUTTON | TVHT_ONITEMICON | TVHT_ONITEMLABEL | TVHT_ONITEMSTATEICON)) != 0))
		{
			return;
		}
		LRESULT const selected = SendMessageW(m_hwnd, TVM_SELECTITEM, TVGN_CARET, reinterpret_cast<LPARAM>(hti.hItem));
		assert(selected == TRUE);
		item = hti.hItem;
	}
	TVITEMEXW ti;
	ti.hItem = item;
	ti.mask = TVIF_PARAM;
	LRESULT const got_item = SendMessageW(m_hwnd, TVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&ti));
	assert(got_item == TRUE);
	file_info const& fi = *reinterpret_cast<file_info*>(ti.lParam);
	bool const enable_goto_orig = fi.m_orig_instance != nullptr;
	HMENU const menu = reinterpret_cast<HMENU>(m_menu.get());
	BOOL const enabled = EnableMenuItem(menu, static_cast<std::uint16_t>(e_tree_menu_id::e_orig), MF_BYCOMMAND | (enable_goto_orig ? MF_ENABLED : MF_GRAYED));
	assert(enabled != -1 && (enabled == MF_ENABLED || enabled == MF_GRAYED));
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

void tree_view::on_accel_orig()
{
	select_original_instance();
}

void tree_view::refresh()
{
	LRESULT const redr_off = SendMessageW(m_hwnd, WM_SETREDRAW, FALSE, 0);
	LRESULT const deselected = SendMessageW(m_hwnd, TVM_SELECTITEM, TVGN_CARET, reinterpret_cast<LPARAM>(nullptr));
	assert(deselected == TRUE);
	LRESULT const deleted = SendMessageW(m_hwnd, TVM_DELETEITEM, 0, reinterpret_cast<LPARAM>(TVI_ROOT));
	assert(deleted == TRUE);

	assert(m_main_window.m_mo.m_fi.m_sub_file_infos.size() >= 1);
	for(auto& sub_fi : m_main_window.m_mo.m_fi.m_sub_file_infos)
	{
		refresh_view_recursive(sub_fi, TVI_ROOT);
	}

	for(auto& sub_fi : m_main_window.m_mo.m_fi.m_sub_file_infos)
	{
		assert(!m_main_window.m_mo.m_fi.m_sub_file_infos[0].m_sub_file_infos.empty());
		LRESULT const expanded = SendMessageW(m_hwnd, TVM_EXPAND, TVE_EXPAND, reinterpret_cast<LPARAM>(sub_fi.m_sub_file_infos[0].m_tree_item));
	}
	HTREEITEM const first = static_cast<HTREEITEM>(m_main_window.m_mo.m_fi.m_sub_file_infos[0].m_sub_file_infos[0].m_tree_item);
	LRESULT const visibled = SendMessageW(m_hwnd, TVM_ENSUREVISIBLE, 0, reinterpret_cast<LPARAM>(first));
	LRESULT const selected = SendMessageW(m_hwnd, TVM_SELECTITEM, TVGN_CARET, reinterpret_cast<LPARAM>(first));
	assert(selected == TRUE);

	LRESULT const redr_on_1 = SendMessageW(m_hwnd, WM_SETREDRAW, TRUE, 0);
	repaint();
}

void tree_view::repaint()
{
	BOOL const redrawn = RedrawWindow(m_hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_FRAME);
	assert(redrawn != 0);
}

smart_menu tree_view::create_menu()
{
	HMENU const tree_menu = CreatePopupMenu();
	assert(tree_menu);
	MENUITEMINFOW mi{};
	mi.cbSize = sizeof(mi);
	mi.fMask = MIIM_ID | MIIM_STRING | MIIM_FTYPE;
	mi.fType = MFT_STRING;
	mi.wID = static_cast<std::uint16_t>(e_tree_menu_id::e_orig);
	mi.dwTypeData = const_cast<wchar_t*>(s_tree_menu_orig_str);
	BOOL const inserted = InsertMenuItemW(tree_menu, 0, TRUE, &mi);
	assert(inserted != 0);
	return smart_menu{tree_menu};
}

void tree_view::refresh_view_recursive(file_info& parent_fi, void* const parent_ti)
{
	for(auto& fi : parent_fi.m_sub_file_infos)
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
		m_main_window.request_symbol_traslation(fi);
		refresh_view_recursive(fi, ti);
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
	file_info const& fi = *reinterpret_cast<file_info*>(ti.lParam);
	if(!fi.m_orig_instance)
	{
		return;
	}
	LRESULT const orig_selected = SendMessageW(m_hwnd, TVM_SELECTITEM, TVGN_CARET, reinterpret_cast<LPARAM>(fi.m_orig_instance->m_tree_item));
	assert(orig_selected == TRUE);
}
