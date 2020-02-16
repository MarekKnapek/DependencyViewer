#include "list_view_base.h"

#include "../nogui/cassert_my.h"

#include <cstdint>
#include <vector>

#include "../nogui/my_windows.h"

#include <commctrl.h>
#include <windowsx.h>


int list_view_base::on_columnclick(void const* const param, [[maybe_unused]] int const n_headers, int const curr_sort)
{
	assert(n_headers <= 127);
	assert(curr_sort <= 0xFF);
	NMLISTVIEW const& nmlv = *static_cast<NMLISTVIEW const*>(param);
	assert(nmlv.iItem == -1);
	assert(nmlv.iSubItem <= 127);
	assert(nmlv.iSubItem >= 0 && nmlv.iSubItem < n_headers);
	std::uint8_t const col_u8 = static_cast<std::uint8_t>(nmlv.iSubItem);
	std::uint8_t const cur_sort_raw = static_cast<std::uint8_t>(curr_sort);
	std::uint8_t new_sort;
	if(cur_sort_raw == 0xFF)
	{
		new_sort = col_u8;
	}
	else
	{
		bool const cur_sort_asc = (cur_sort_raw & (1u << 7u)) == 0u;
		std::uint8_t const cur_sort_col = cur_sort_raw &~ (1u << 7u);
		if(cur_sort_col != col_u8)
		{
			new_sort = col_u8;
		}
		else
		{
			if(cur_sort_asc)
			{
				new_sort = cur_sort_col | (1u << 7u);
			}
			else
			{
				new_sort = 0xFF;
			}
		}
	}
	return new_sort;
}

void list_view_base::refresh_headers(void const* const hwnd_ptr, int const n_headers, int const curr_sort)
{
	assert(hwnd_ptr);
	HWND const& hwnd = *static_cast<HWND const*>(hwnd_ptr);
	assert(IsWindow(hwnd) != 0);
	HWND const hdr = reinterpret_cast<HWND>(SendMessageW(hwnd, LVM_GETHEADER, 0, 0));
	assert(hdr != nullptr);
	for(int i = 0; i != n_headers; ++i)
	{
		HDITEMW hd_item;
		hd_item.mask = HDI_FORMAT;
		LRESULT const got_item = SendMessageW(hdr, HDM_GETITEMW, i, reinterpret_cast<LPARAM>(&hd_item));
		assert(got_item != FALSE);
		if((hd_item.fmt & HDF_SORTDOWN) != 0 || (hd_item.fmt & HDF_SORTUP) != 0)
		{
			hd_item.fmt &=~ HDF_SORTDOWN;
			hd_item.fmt &=~ HDF_SORTUP;
			LRESULT const set_item = SendMessageW(hdr, HDM_SETITEMW, i, reinterpret_cast<LPARAM>(&hd_item));
			assert(set_item != 0);
		}
	}
	assert(curr_sort <= 0xFF);
	std::uint8_t const cur_sort_raw = static_cast<std::uint8_t>(curr_sort);
	if(cur_sort_raw != 0xFF)
	{
		bool const cur_sort_asc = (cur_sort_raw & (1u << 7u)) == 0u;
		std::uint8_t const cur_sort_col = cur_sort_raw &~ (1u << 7u);
		HDITEMW hd_item;
		hd_item.mask = HDI_FORMAT;
		LRESULT const got_item = SendMessageW(hdr, HDM_GETITEMW, cur_sort_col, reinterpret_cast<LPARAM>(&hd_item));
		assert(got_item != FALSE);
		if(cur_sort_asc)
		{
			hd_item.fmt |= HDF_SORTUP;
		}
		else
		{
			hd_item.fmt |= HDF_SORTDOWN;
		}
		LRESULT const set_item = SendMessageW(hdr, HDM_SETITEMW, cur_sort_col, reinterpret_cast<LPARAM>(&hd_item));
		assert(set_item != 0);
	}
}

int list_view_base::get_selection(void const* const hwnd_ptr)
{
	assert(hwnd_ptr);
	HWND const& hwnd = *static_cast<HWND const*>(hwnd_ptr);
	assert(IsWindow(hwnd) != 0);
	LRESULT const sel = SendMessageW(hwnd, LVM_GETNEXTITEM, WPARAM{0} - 1, LVNI_SELECTED);
	if(sel == -1)
	{
		return -1;
	}
	assert(sel >= 0 && sel <= 0xFFFF);
	std::uint16_t const line_idx = static_cast<std::uint16_t>(sel);
	int const ret = line_idx;
	return ret;
}

void list_view_base::select_item(void const* const hwnd_ptr, void const* const sort_ptr, int const item_idx)
{
	assert(hwnd_ptr);
	HWND const& hwnd = *static_cast<HWND const*>(hwnd_ptr);
	assert(IsWindow(hwnd) != 0);
	assert(sort_ptr);
	std::vector<std::uint16_t> const& sort = *static_cast<std::vector<std::uint16_t> const*>(sort_ptr);
	assert(item_idx <= 0xFFFF);
	std::uint16_t const ith_line = sort.empty() ? static_cast<std::uint16_t>(item_idx) : sort[sort.size() / 2 + item_idx];
	LRESULT const visibility_ensured = SendMessageW(hwnd, LVM_ENSUREVISIBLE, ith_line, FALSE);
	assert(visibility_ensured == TRUE);
	LVITEMW lvi;
	lvi.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
	lvi.state = 0;
	LRESULT const selection_cleared = SendMessageW(hwnd, LVM_SETITEMSTATE, WPARAM{0} - 1, reinterpret_cast<LPARAM>(&lvi));
	assert(selection_cleared == TRUE);
	lvi.state = LVIS_FOCUSED | LVIS_SELECTED;
	LRESULT const selection_set = SendMessageW(hwnd, LVM_SETITEMSTATE, static_cast<WPARAM>(ith_line), reinterpret_cast<LPARAM>(&lvi));
	assert(selection_set == TRUE);
	[[maybe_unused]] HWND const prev_focus = SetFocus(hwnd);
	assert(prev_focus != nullptr);
}

bool list_view_base::get_context_menu(void const* const hwnd_ptr, void const* const lparam_ptr, void const* const sort_ptr, int* const out_item_idx, void* const out_screen_pos_ptr)
{
	assert(hwnd_ptr);
	HWND const& hwnd = *static_cast<HWND const*>(hwnd_ptr);
	assert(IsWindow(hwnd) != 0);
	assert(lparam_ptr);
	LPARAM const& lparam = *static_cast<LPARAM const*>(lparam_ptr);
	assert(sort_ptr);
	std::vector<std::uint16_t> const& sort = *static_cast<std::vector<std::uint16_t> const*>(sort_ptr);
	assert(out_item_idx);
	assert(out_screen_pos_ptr);
	POINT& out_screen_pos = *static_cast<POINT*>(out_screen_pos_ptr);
	if(lparam == LPARAM{-1})
	{
		int const sel = list_view_base::get_selection(&hwnd);
		if(sel == -1)
		{
			return false;
		}
		assert(sel >= 0 && sel <= 0xFFFF);
		std::uint16_t const line_idx = static_cast<std::uint16_t>(sel);
		std::uint16_t const item_idx = sort.empty() ? line_idx : sort[line_idx];
		RECT rect;
		rect.top = 1;
		rect.left = LVIR_BOUNDS;
		LRESULT const got_rect = SendMessageW(hwnd, LVM_GETSUBITEMRECT, line_idx, reinterpret_cast<LPARAM>(&rect));
		if(got_rect == 0)
		{
			return false;
		}
		POINT client_pos;
		client_pos.x = rect.left + (rect.right - rect.left) / 2;
		client_pos.y = rect.top + (rect.bottom - rect.top) / 2;
		POINT screen_pos = client_pos;
		BOOL const converted = ClientToScreen(hwnd, &screen_pos);
		assert(converted != 0);
		*out_item_idx = item_idx;
		out_screen_pos = screen_pos;
		return true;
	}
	else
	{
		POINT screen_pos;
		screen_pos.x = GET_X_LPARAM(lparam);
		screen_pos.y = GET_Y_LPARAM(lparam);
		POINT client_pos = screen_pos;
		BOOL const converted = ScreenToClient(hwnd, &client_pos);
		assert(converted != 0);
		LVHITTESTINFO hti;
		hti.pt = client_pos;
		hti.flags = LVHT_ONITEM;
		LPARAM const hit_tested = SendMessageW(hwnd, LVM_HITTEST, 0, reinterpret_cast<LPARAM>(&hti));
		if(hit_tested == -1)
		{
			return false;
		}
		assert(hit_tested == hti.iItem);
		assert(hti.iItem >= 0 && hti.iItem <= 0xFFFF);
		std::uint16_t const line_idx = static_cast<std::uint16_t>(hti.iItem);
		std::uint16_t const item_idx = sort.empty() ? line_idx : sort[line_idx];
		*out_item_idx = item_idx;
		out_screen_pos = screen_pos;
		return true;
	}
}
