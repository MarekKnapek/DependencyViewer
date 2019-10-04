#include "export_view.h"

#include "main.h"
#include "main_window.h"
#include "smart_dc.h"

#include "../nogui/pe.h"
#include "../nogui/int_to_string.h"
#include "../nogui/scope_exit.h"

#include <algorithm>
#include <iterator>
#include <cassert>

#include <commctrl.h>


enum class e_export_column
{
	e_type,
	e_ordinal,
	e_hint,
	e_name,
	e_entry_point
};
static constexpr wchar_t const* const s_export_headers[] =
{
	L"type",
	L"ordinal",
	L"hint",
	L"name",
	L"entry point"
};
static constexpr wchar_t const s_export_type_true[] = L"address";
static constexpr wchar_t const s_export_type_false[] = L"forwarder";
static constexpr wchar_t const s_export_hint_na[] = L"N/A";
static constexpr wchar_t const s_export_name_processing[] = L"Processing...";
static constexpr wchar_t const s_export_name_na[] = L"N/A";


static int g_export_type_column_max_width = 0;


export_view::export_view(HWND const parent, main_window& mw) :
	m_hwnd(CreateWindowExW(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE, WC_LISTVIEWW, nullptr, WS_VISIBLE | WS_CHILD | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_OWNERDATA, 0, 0, 0, 0, parent, nullptr, get_instance(), nullptr)),
	m_main_window(mw),
	m_tmp_strings(),
	m_tmp_string_idx()
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
	if((nm.item.mask | LVIF_TEXT) != 0)
	{
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
		pe_export_address_entry const& export_entry = fi.m_export_table.m_export_address_table[row];
		e_export_column const ecol = static_cast<e_export_column>(col);
		switch(ecol)
		{
			case e_export_column::e_type:
			{
				nm.item.pszText = const_cast<wchar_t*>(on_get_col_type(export_entry));
			}
			break;
			case e_export_column::e_ordinal:
			{
				nm.item.pszText = const_cast<wchar_t*>(on_get_col_ordinal(export_entry));
			}
			break;
			case e_export_column::e_hint:
			{
				nm.item.pszText = const_cast<wchar_t*>(on_get_col_hint(export_entry));
			}
			break;
			case e_export_column::e_name:
			{
				nm.item.pszText = const_cast<wchar_t*>(on_get_col_name(export_entry));
			}
			break;
			case e_export_column::e_entry_point:
			{
				nm.item.pszText = const_cast<wchar_t*>(on_get_col_address(export_entry));
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

	LRESULT const set_size = SendMessageW(m_hwnd, LVM_SETITEMCOUNT, fi.m_export_table.m_export_address_table.size(), 0);
	assert(set_size != 0);

	int const ordinal_column_max_width = m_main_window.get_ordinal_column_max_width();
	int const export_type_column_max_width = get_type_column_max_width();

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
	HWND const prev_focus = SetFocus(m_hwnd);
	assert(prev_focus != nullptr);
	(void)prev_focus;
	repaint();
}

wchar_t const* export_view::on_get_col_type(pe_export_address_entry const& export_entry)
{
	if(export_entry.m_is_rva)
	{
		return s_export_type_true;
	}
	else
	{
		return s_export_type_false;
	}
}

wchar_t const* export_view::on_get_col_ordinal(pe_export_address_entry const& export_entry)
{
	std::wstring& tmpstr = m_tmp_strings[m_tmp_string_idx++ % m_tmp_strings.size()];
	ordinal_to_string(export_entry.m_ordinal, tmpstr);
	return tmpstr.c_str();
}

wchar_t const* export_view::on_get_col_hint(pe_export_address_entry const& export_entry)
{
	if(!export_entry.m_name)
	{
		return s_export_hint_na;
	}
	else
	{
		std::wstring& tmpstr = m_tmp_strings[m_tmp_string_idx++ % m_tmp_strings.size()];
		ordinal_to_string(export_entry.m_hint, tmpstr);
		return tmpstr.c_str();
	}
}

wchar_t const* export_view::on_get_col_name(pe_export_address_entry const& export_entry)
{
	if(export_entry.m_name)
	{
		std::wstring& tmpstr = m_tmp_strings[m_tmp_string_idx++ % m_tmp_strings.size()];
		tmpstr.resize(export_entry.m_name->m_len);
		std::transform(cbegin(export_entry.m_name), cend(export_entry.m_name), begin(tmpstr), [](char const& e) -> wchar_t { return static_cast<wchar_t>(e); });
		return tmpstr.c_str();
	}
	else
	{
		if(export_entry.m_is_rva)
		{
			if(export_entry.m_debug_name)
			{
				return export_entry.m_debug_name->m_str;
			}
			else
			{
				return s_export_name_processing;
			}
		}
		else
		{
			return s_export_name_na;
		}
	}
}

wchar_t const* export_view::on_get_col_address(pe_export_address_entry const& export_entry)
{
	if(export_entry.m_is_rva)
	{
		std::wstring& tmpstr = m_tmp_strings[m_tmp_string_idx++ % m_tmp_strings.size()];
		rva_to_string(export_entry.rva_or_forwarder.m_rva, tmpstr);
		return tmpstr.c_str();
	}
	else
	{
		std::wstring& tmpstr = m_tmp_strings[m_tmp_string_idx++ % m_tmp_strings.size()];
		tmpstr.resize(export_entry.rva_or_forwarder.m_forwarder->m_len);
		std::transform(cbegin(export_entry.rva_or_forwarder.m_forwarder), cend(export_entry.rva_or_forwarder.m_forwarder), begin(tmpstr), [](char const& e) -> wchar_t { return static_cast<wchar_t>(e); });
		return tmpstr.c_str();
	}
}

int export_view::get_type_column_max_width()
{
	if(g_export_type_column_max_width != 0)
	{
		return g_export_type_column_max_width;
	}

	HDC const dc = GetDC(m_hwnd);
	assert(dc != NULL);
	smart_dc sdc(m_hwnd, dc);

	int maximum = 0;
	SIZE size;

	BOOL const got1 = GetTextExtentPointW(dc, s_export_type_true, static_cast<int>(std::size(s_export_type_true)) - 1, &size);
	assert(got1 != 0);
	maximum = (std::max)(maximum, static_cast<int>(size.cx));

	BOOL const got2 = GetTextExtentPointW(dc, s_export_type_false, static_cast<int>(std::size(s_export_type_false)) - 1, &size);
	assert(got2 != 0);
	maximum = (std::max)(maximum, static_cast<int>(size.cx));

	g_export_type_column_max_width = maximum;
	return g_export_type_column_max_width;
}
