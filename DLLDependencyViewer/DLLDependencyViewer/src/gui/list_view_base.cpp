#include "list_view_base.h"

#include <cassert>
#include <cstdint>

#include "../nogui/my_windows.h"
#include <commctrl.h>


list_view_base::list_view_base()
{
}

list_view_base::~list_view_base()
{
}

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

void list_view_base::refresh_headers(void const* const handle, int const n_headers, int const curr_sort)
{
	HWND const& hwnd = *static_cast<HWND const*>(handle);
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
