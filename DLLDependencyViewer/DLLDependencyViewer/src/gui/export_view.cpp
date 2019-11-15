#include "export_view.h"

#include "constants.h"
#include "main.h"
#include "main_window.h"
#include "smart_dc.h"

#include "../nogui/array_bool.h"
#include "../nogui/int_to_string.h"
#include "../nogui/pe.h"
#include "../nogui/scope_exit.h"

#include "../res/resources.h"

#include <algorithm>
#include <cassert>
#include <iterator>

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


export_view::export_view(HWND const parent, main_window& mw) :
	m_hwnd(CreateWindowExW(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE, WC_LISTVIEWW, nullptr, WS_VISIBLE | WS_CHILD | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_OWNERDATA, 0, 0, 0, 0, parent, nullptr, get_instance(), nullptr)),
	m_main_window(mw),
	m_menu(create_menu()),
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
	HIMAGELIST const img_list = ImageList_LoadImageW(get_instance(), MAKEINTRESOURCEW(s_res_icons_import_export), 30, 0, CLR_DEFAULT, IMAGE_BITMAP, LR_DEFAULTCOLOR);
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
	if(nmhdr.code == LVN_GETDISPINFOW)
	{
		on_getdispinfow(nmhdr);
	}
}

void export_view::on_getdispinfow(NMHDR& nmhdr)
{
	NMLVDISPINFOW& nm = reinterpret_cast<NMLVDISPINFOW&>(nmhdr);
	HWND const tree = m_main_window.m_tree_view.get_hwnd();
	HTREEITEM const selected = reinterpret_cast<HTREEITEM>(SendMessageW(tree, TVM_GETNEXTITEM, TVGN_CARET, 0));
	if(!selected)
	{
		return;
	}
	TVITEMEXW ti;
	ti.hItem = selected;
	ti.mask = TVIF_PARAM;
	LRESULT const got_selected = SendMessageW(tree, TVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&ti));
	assert(got_selected == TRUE);
	file_info const& fi_tmp = *reinterpret_cast<file_info*>(ti.lParam);
	file_info const& fi = fi_tmp.m_orig_instance ? *fi_tmp.m_orig_instance : fi_tmp;
	int const row = nm.item.iItem;
	int const col = nm.item.iSubItem;
	pe_export_table_info const& eti = fi.m_export_table;
	std::uint16_t const exp_idx = static_cast<std::uint16_t>(row);
	e_export_column const ecol = static_cast<e_export_column>(col);
	if((nm.item.mask | LVIF_TEXT) != 0)
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
		bool const is_used = array_bool_tst(eti.m_are_used, exp_idx);
		bool const is_matched = fi_tmp.m_matched_imports[exp_idx] != 0xffff;
		bool const is_rva = array_bool_tst(eti.m_are_rvas, exp_idx);
		bool const has_name = eti.m_hints[exp_idx] != 0xFFFF;
		static constexpr std::int8_t const s_export_images[2][2][2][2] =
		{
			{
				{
					{
						s_res_icon_export0000,
						s_res_icon_export0001,
					},
					{
						s_res_icon_export0010,
						s_res_icon_export0011,
					},
				},
				{
					{
						-1,
						-1,
					},
					{
						-1,
						-1,
					},
				},
			},
			{
				{
					{
						s_res_icon_export1000,
						s_res_icon_export1001,
					},
					{
						s_res_icon_export1010,
						s_res_icon_export1011,
					},
				},
				{
					{
						s_res_icon_export1100,
						s_res_icon_export1101,
					},
					{
						s_res_icon_export1110,
						s_res_icon_export1111,
					},
				},
			},
		};
		std::int8_t const img = s_export_images[is_used ? 1 : 0][is_matched ? 1 : 0][is_rva ? 1 : 0][has_name ? 1 : 0];
		assert(img != -1);
		nm.item.iImage = img;
	}
}

void export_view::on_context_menu(LPARAM const lparam)
{
	POINT cursor_screen;
	int ith_export;
	if(lparam == 0xffffffff)
	{
		LRESULT const sel = SendMessageW(m_hwnd, LVM_GETNEXTITEM, WPARAM{0} - 1, LVNI_SELECTED);
		if(sel == -1)
		{
			return;
		}
		RECT rect;
		rect.top = static_cast<int>(e_export_column::e_type);
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
		ith_export = static_cast<int>(sel);
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
		ith_export = hti.iItem;
	}
	HWND const tree = m_main_window.m_tree_view.get_hwnd();
	HTREEITEM const tree_selected = reinterpret_cast<HTREEITEM>(SendMessageW(tree, TVM_GETNEXTITEM, TVGN_CARET, 0));
	if(!tree_selected)
	{
		return;
	}
	TVITEMEXW ti;
	ti.hItem = tree_selected;
	ti.mask = TVIF_PARAM;
	LRESULT const got_item = SendMessageW(tree, TVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&ti));
	assert(got_item == TRUE);
	file_info const& fi = *reinterpret_cast<file_info*>(ti.lParam);
	std::uint16_t const& matched = fi.m_matched_imports[ith_export];
	bool const enable_goto_orig = matched != 0xffff;
	HMENU const menu = reinterpret_cast<HMENU>(m_menu.get());
	BOOL const enabled = EnableMenuItem(menu, static_cast<std::uint16_t>(e_export_menu_id::e_matching), MF_BYCOMMAND | (enable_goto_orig ? MF_ENABLED : MF_GRAYED));
	assert(enabled != -1 && (enabled == MF_ENABLED || enabled == MF_GRAYED));
	BOOL const tracked = TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON | TPM_NOANIMATION, cursor_screen.x, cursor_screen.y, 0, m_main_window.m_hwnd, nullptr);
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
	file_info const& fi = fi_tmp.m_orig_instance ? *fi_tmp.m_orig_instance : fi_tmp;

	LRESULT const set_size = SendMessageW(m_hwnd, LVM_SETITEMCOUNT, fi.m_export_table.m_count, 0);
	assert(set_size != 0);

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

void export_view::select_item(std::uint16_t const item_idx)
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
	[[maybe_unused]] HWND const prev_focus = SetFocus(m_hwnd);
	assert(prev_focus != nullptr);
	repaint();
}

void export_view::sort_view()
{
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
	bool const is_rva = array_bool_tst(eti.m_are_rvas, exp_idx);
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
	std::uint16_t const& ordinal = eti.m_ordinals[exp_idx];
	return ordinal_to_string(ordinal, m_string_converter);
}

wchar_t const* export_view::on_get_col_hint(pe_export_table_info const& eti, std::uint16_t const exp_idx)
{
	bool const has_name = eti.m_hints[exp_idx] != 0xFFFF;
	if(has_name)
	{
		std::uint16_t const& hint = eti.m_hints[exp_idx];
		return ordinal_to_string(hint, m_string_converter);
	}
	else
	{
		return s_export_hint_na;
	}
}

wchar_t const* export_view::on_get_col_name(pe_export_table_info const& eti, std::uint16_t const exp_idx)
{
	bool const has_name = eti.m_hints[exp_idx] != 0xFFFF;
	if(has_name)
	{
		bool const undecorate = m_main_window.m_settings.m_undecorate;
		if(!undecorate)
		{
			string_handle const& name = eti.m_names[exp_idx];
			return m_string_converter.convert(name);
		}
		else
		{
			string_handle const& name = eti.m_names[exp_idx];
			if(cbegin(name)[0] != '?')
			{
				return m_string_converter.convert(name);
			}
			else
			{
				string_handle const& undecorated_name = eti.m_undecorated_names[exp_idx];
				if(!undecorated_name.m_string)
				{
					return s_export_name_undecorating;
				}
				else if(undecorated_name.m_string == static_cast<string const*>(nullptr) + 1)
				{
					return m_string_converter.convert(name);
				}
				else
				{
					return m_string_converter.convert(undecorated_name);
				}
			}
		}
	}
	else
	{
		bool const is_rva = array_bool_tst(eti.m_are_rvas, exp_idx);
		if(is_rva)
		{
			string_handle const& debug_name = eti.m_names[exp_idx];
			if(!debug_name.m_string)
			{
				return s_export_name_processing;
			}
			else if(debug_name.m_string == static_cast<string const*>(nullptr) + 1)
			{
				return s_export_name_na;
			}
			else
			{
				bool const undecorate = m_main_window.m_settings.m_undecorate;
				if(!undecorate)
				{
					return m_string_converter.convert(debug_name);
				}
				else
				{
					if(cbegin(debug_name)[0] != '?')
					{
						return m_string_converter.convert(debug_name);
					}
					else
					{
						string_handle const& undecorated_name = eti.m_undecorated_names[exp_idx];
						if(!undecorated_name.m_string)
						{
							return s_export_name_undecorating;
						}
						else if(undecorated_name.m_string == static_cast<string const*>(nullptr) + 1)
						{
							return m_string_converter.convert(debug_name);
						}
						else
						{
							return m_string_converter.convert(undecorated_name);
						}
					}
				}
			}
		}
		else
		{
			return s_export_name_na;
		}
	}
}

wchar_t const* export_view::on_get_col_address(pe_export_table_info const& eti, std::uint16_t const exp_idx)
{
	bool const is_rva = array_bool_tst(eti.m_are_rvas, exp_idx);
	if(is_rva)
	{
		std::uint32_t const& rva = eti.m_rvas_or_forwarders[exp_idx].m_rva;
		return rva_to_string(rva, m_string_converter);
	}
	else
	{
		string_handle const& forwarder = eti.m_rvas_or_forwarders[exp_idx].m_forwarder;
		return m_string_converter.convert(forwarder);
	}
}

void export_view::select_matching_instance()
{
	LRESULT const sel = SendMessageW(m_hwnd, LVM_GETNEXTITEM, WPARAM{0} - 1, LVNI_SELECTED);
	if(sel == -1)
	{
		return;
	}
	int const ith_export = static_cast<int>(sel);
	HWND const tree = m_main_window.m_tree_view.get_hwnd();
	HTREEITEM const tree_selected = reinterpret_cast<HTREEITEM>(SendMessageW(tree, TVM_GETNEXTITEM, TVGN_CARET, 0));
	if(!tree_selected)
	{
		return;
	}
	TVITEMEXW ti;
	ti.hItem = tree_selected;
	ti.mask = TVIF_PARAM;
	LRESULT const got_item = SendMessageW(tree, TVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&ti));
	assert(got_item == TRUE);
	file_info const& fi = *reinterpret_cast<file_info*>(ti.lParam);
	std::uint16_t const& matched = fi.m_matched_imports[ith_export];
	if(matched == 0xffff)
	{
		return;
	}
	m_main_window.m_import_view.select_item(matched);
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
