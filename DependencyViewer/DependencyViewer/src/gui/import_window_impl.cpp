#include "import_window_impl.h"

#include "common_controls.h"
#include "list_view_base.h"
#include "main.h"
#include "processor.h"

#include "../nogui/cassert_my.h"
#include "../nogui/int_to_string.h"
#include "../nogui/pe_getters_import.h"
#include "../nogui/scope_exit.h"

#include "../res/resources.h"

#include <cstdint>
#include <iterator>

#include "../nogui/windows_my.h"

#include <commctrl.h>


enum class e_import_column___2 : std::uint16_t
{
	e_pi,
	e_type,
	e_ordinal,
	e_hint,
	e_name,
};
static constexpr wchar_t const* const s_import_headers___2[] =
{
	L"PI",
	L"type",
	L"ordinal",
	L"hint",
	L"name",
};
static constexpr wchar_t const s_import_type_true___2[] = L"ordinal";
static constexpr wchar_t const s_import_type_false___2[] = L"name";
static constexpr wchar_t const s_import_ordinal_na___2[] = L"N/A";
static constexpr wchar_t const s_import_hint_na___2[] = L"N/A";
static constexpr wchar_t const s_import_name_na___2[] = L"N/A";
static constexpr wchar_t const s_import_name_processing___2[] = L"Processing...";
static constexpr wchar_t const s_import_name_undecorating___2[] = L"Undecorating...";


ATOM import_window_impl::g_class;
int import_window_impl::g_column_type_max_width;


import_window_impl::import_window_impl(HWND const& self) :
	m_self(self),
	m_list_view(),
	m_fi(),
	m_undecorate(),
	m_string_converter()
{
	assert(self != nullptr);

	DWORD const ex_style = WS_EX_CLIENTEDGE;
	wchar_t const* const class_name = WC_LISTVIEWW;
	wchar_t const* const window_name = nullptr;
	DWORD const style = (WS_VISIBLE | WS_CHILD | WS_TABSTOP) | (LVS_REPORT | LVS_SHOWSELALWAYS | LVS_OWNERDATA);
	int const x_pos = 0;
	int const y_pos = 0;
	int const width = 0;
	int const height = 0;
	HWND const parent = m_self;
	HMENU const menu = nullptr;
	HINSTANCE const instance = get_instance();
	LPVOID const param = nullptr;
	m_list_view = CreateWindowExW(ex_style, class_name, window_name, style, x_pos, y_pos, width, height, parent, menu, instance, param);
	assert(m_list_view != nullptr);
	unsigned const extended_lv_styles = LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP | LVS_EX_DOUBLEBUFFER;
	LRESULT const prev_style = SendMessageW(m_list_view, LVM_SETEXTENDEDLISTVIEWSTYLE, extended_lv_styles, extended_lv_styles);
	for(int i = 0; i != static_cast<int>(std::size(s_import_headers___2)); ++i)
	{
		LVCOLUMNW cl;
		cl.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		cl.fmt = LVCFMT_LEFT;
		cl.cx = 200;
		cl.pszText = const_cast<LPWSTR>(s_import_headers___2[i]);
		cl.cchTextMax = 0;
		cl.iSubItem = i;
		cl.iImage = 0;
		cl.iOrder = i;
		cl.cxMin = 0;
		cl.cxDefault = 0;
		cl.cxIdeal = 0;
		auto const inserted = SendMessageW(m_list_view, LVM_INSERTCOLUMNW, i, reinterpret_cast<LPARAM>(&cl));
		assert(inserted != -1);
		assert(inserted == i);
	}
	HIMAGELIST const img_list = common_controls::ImageList_LoadImageW(get_instance(), MAKEINTRESOURCEW(s_res_icons_import_export), 30, 0, CLR_DEFAULT, IMAGE_BITMAP, LR_DEFAULTCOLOR);
	assert(img_list);
	LRESULT const prev_imgs = SendMessageW(m_list_view, LVM_SETIMAGELIST, LVSIL_SMALL, reinterpret_cast<LPARAM>(img_list));

	refresh();
}

import_window_impl::~import_window_impl()
{
}

void import_window_impl::init()
{
	register_class();
}

void import_window_impl::deinit()
{
	unregister_class();
}

wchar_t const* import_window_impl::get_class_atom()
{
	assert(g_class != 0);
	return reinterpret_cast<wchar_t const*>(g_class);
}

void import_window_impl::register_class()
{
	WNDCLASSEXW wc{};
	wc.cbSize = sizeof(wc);
	wc.style = 0;
	wc.lpfnWndProc = &class_proc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = get_instance();
	wc.hIcon = nullptr;
	wc.hCursor = LoadCursorW(nullptr, reinterpret_cast<wchar_t const*>(IDC_ARROW));
	wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = L"import_window";
	wc.hIconSm = nullptr;

	assert(g_class == 0);
	g_class = RegisterClassExW(&wc);
	assert(g_class != 0);
}

void import_window_impl::unregister_class()
{
	assert(g_class != 0);
	BOOL const unregistered = UnregisterClassW(reinterpret_cast<wchar_t const*>(g_class), get_instance());
	assert(unregistered != 0);
	g_class = 0;
}

LRESULT CALLBACK import_window_impl::class_proc(HWND const hwnd, UINT const msg, WPARAM const wparam, LPARAM const lparam)
{
	if(msg == WM_CREATE)
	{
		LRESULT const ret = on_wm_create(hwnd, wparam, lparam);
		return ret;
	}
	else if(msg == WM_DESTROY)
	{
		LRESULT const ret = on_wm_destroy(hwnd, wparam, lparam);
		return ret;
	}
	else
	{
		LONG_PTR const self_ptr = GetWindowLongPtrW(hwnd, GWLP_USERDATA);
		if(self_ptr != 0)
		{
			import_window_impl* const self = reinterpret_cast<import_window_impl*>(self_ptr);
			LRESULT const ret = self->on_message(msg, wparam, lparam);
			return ret;
		}
		else
		{
			LRESULT const ret = DefWindowProcW(hwnd, msg, wparam, lparam);
			return ret;
		}
	}
}

LRESULT import_window_impl::on_wm_create(HWND const& hwnd, WPARAM const& wparam, LPARAM const& lparam)
{
	import_window_impl* const self = new import_window_impl{hwnd};
	assert((SetLastError(0), true));
	LONG_PTR const prev = SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
	assert(prev == 0 && GetLastError() == 0);

	LRESULT const ret = DefWindowProcW(hwnd, WM_CREATE, wparam, lparam);
	return ret;
}

LRESULT import_window_impl::on_wm_destroy(HWND const& hwnd, WPARAM const& wparam, LPARAM const& lparam)
{
	LONG_PTR const prev = SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
	assert(prev != 0);
	import_window_impl* const self = reinterpret_cast<import_window_impl*>(prev);
	delete self;

	LRESULT const ret = DefWindowProcW(hwnd, WM_DESTROY, wparam, lparam);
	return ret;
}

LRESULT import_window_impl::on_message(UINT const& msg, WPARAM const& wparam, LPARAM const& lparam)
{
	switch(msg)
	{
		case WM_SIZE:
		{
			LRESULT const ret = on_wm_size(wparam, lparam);
			return ret;
		}
		break;
		case WM_NOTIFY:
		{
			LRESULT const ret = on_wm_notify(wparam, lparam);
			return ret;
		}
		break;
		case static_cast<std::uint32_t>(import_window::wm::wm_setfi):
		{
			LRESULT const ret = on_wm_setfi(wparam, lparam);
			return ret;
		}
		break;
		case static_cast<std::uint32_t>(import_window::wm::wm_setundecorate):
		{
			LRESULT const ret = on_wm_setundecorate(wparam, lparam);
			return ret;
		}
		break;
		default:
		{
			LRESULT const ret = DefWindowProcW(m_self, msg, wparam, lparam);
			return ret;
		}
		break;
	}
}

LRESULT import_window_impl::on_wm_size(WPARAM const& wparam, LPARAM const& lparam)
{
	RECT rect;
	BOOL const got = GetClientRect(m_self, &rect);
	assert(got != 0);
	assert(rect.left == 0 && rect.top == 0);
	BOOL const moved = MoveWindow(m_list_view, 0, 0, rect.right, rect.bottom, TRUE);
	assert(moved != 0);

	LRESULT const ret = DefWindowProcW(m_self, WM_SIZE, wparam, lparam);
	return ret;
}

LRESULT import_window_impl::on_wm_notify(WPARAM const& wparam, LPARAM const& lparam)
{
	NMHDR& nmhdr = *reinterpret_cast<NMHDR*>(lparam);
	if(nmhdr.hwndFrom == m_list_view)
	{
		switch(nmhdr.code)
		{
			case LVN_GETDISPINFOW:
			{
				on_getdispinfow(nmhdr);
			}
			break;
		}
	}

	LRESULT const ret = DefWindowProcW(m_self, WM_NOTIFY, wparam, lparam);
	return ret;
}

LRESULT import_window_impl::on_wm_setfi(WPARAM const& wparam, LPARAM const& lparam)
{
	file_info const* const fi = reinterpret_cast<file_info const*>(lparam);
	m_fi = fi;
	refresh();

	UINT const msg = static_cast<std::uint32_t>(import_window::wm::wm_setfi);
	LRESULT const ret = DefWindowProcW(m_self, msg, wparam, lparam);
	return ret;
}

LRESULT import_window_impl::on_wm_setundecorate(WPARAM const& wparam, LPARAM const& lparam)
{
	assert(wparam == 0 || wparam == 1);
	bool const undecorate = wparam != 0;
	m_undecorate = undecorate;
	repaint();

	UINT const msg = static_cast<std::uint32_t>(import_window::wm::wm_setundecorate);
	LRESULT const ret = DefWindowProcW(m_self, msg, wparam, lparam);
	return ret;
}

void import_window_impl::on_getdispinfow(NMHDR& nmhdr)
{
	NMLVDISPINFOW& nm = reinterpret_cast<NMLVDISPINFOW&>(nmhdr);
	file_info const* const tmp_fi = m_fi;
	if(!tmp_fi)
	{
		return;
	}
	file_info const* const parent_fi = tmp_fi->m_parent;
	if(!parent_fi)
	{
		return;
	}
	file_info const* const fi = tmp_fi->m_orig_instance ? tmp_fi->m_orig_instance : tmp_fi;
	auto const dll_idx_ = tmp_fi - parent_fi->m_fis;
	assert(dll_idx_ >= 0 && dll_idx_ <= 0xFFFF);
	std::uint16_t const dll_idx = static_cast<std::uint16_t>(dll_idx_);
	pe_import_table_info const& iti = parent_fi->m_import_table;
	pe_export_table_info const& eti = fi->m_export_table;
	int const row = nm.item.iItem;
	assert(row >= 0 && row <= 0xFFFF);
	std::uint16_t const imp_idx = static_cast<std::uint16_t>(row);
	if((nm.item.mask & LVIF_TEXT) != 0)
	{
		int const col_ = nm.item.iSubItem;
		assert(col_ >= static_cast<std::uint16_t>(e_import_column___2::e_pi));
		assert(col_ <= static_cast<std::uint16_t>(e_import_column___2::e_name));
		e_import_column___2 const col = static_cast<e_import_column___2>(col_);
		switch(col)
		{
			case e_import_column___2::e_pi:
			{
				nm.item.pszText = const_cast<wchar_t*>(L"");
			}
			break;
			case e_import_column___2::e_type:
			{
				nm.item.pszText = const_cast<wchar_t*>(get_col_type(iti, dll_idx, imp_idx));
			}
			break;
			case e_import_column___2::e_ordinal:
			{
				nm.item.pszText = const_cast<wchar_t*>(get_col_ordinal(iti, dll_idx, imp_idx));
			}
			break;
			case e_import_column___2::e_hint:
			{
				nm.item.pszText = const_cast<wchar_t*>(get_col_hint(iti, dll_idx, imp_idx));
			}
			break;
			case e_import_column___2::e_name:
			{
				nm.item.pszText = const_cast<wchar_t*>(get_col_name(iti, dll_idx, imp_idx, eti));
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
		nm.item.iImage = get_col_icon(iti, dll_idx, imp_idx);
	}
}

wchar_t const* import_window_impl::get_col_type(pe_import_table_info const& iti, std::uint16_t const dll_idx, std::uint16_t const imp_idx)
{
	bool const is_ordinal = pe_get_import_is_ordinal(iti, dll_idx, imp_idx);
	if(is_ordinal)
	{
		return s_import_type_true___2;
	}
	else
	{
		return s_import_type_false___2;
	}
}

wchar_t const* import_window_impl::get_col_ordinal(pe_import_table_info const& iti, std::uint16_t const dll_idx, std::uint16_t const imp_idx)
{
	auto const oridnal_opt = pe_get_import_ordinal(iti, dll_idx, imp_idx);
	if(oridnal_opt.m_is_valid)
	{
		wchar_t const* const ret = ordinal_to_string(oridnal_opt.m_value, m_string_converter);
		return ret;
	}
	else
	{
		return s_import_ordinal_na___2;
	}
}

wchar_t const* import_window_impl::get_col_hint(pe_import_table_info const& iti, std::uint16_t const dll_idx, std::uint16_t const imp_idx)
{
	auto const hint_opt = pe_get_import_hint(iti, dll_idx, imp_idx);
	if(hint_opt.m_is_valid)
	{
		wchar_t const* const ret = ordinal_to_string(hint_opt.m_value, m_string_converter);
		return ret;
	}
	else
	{
		return s_import_hint_na___2;
	}
}

wchar_t const* import_window_impl::get_col_name(pe_import_table_info const& iti, std::uint16_t const dll_idx, std::uint16_t const imp_idx, pe_export_table_info const& eti)
{
	if(m_undecorate)
	{
		string_handle const name = pe_get_import_name_undecorated(iti, eti, dll_idx, imp_idx);
		if(!name.m_string)
		{
			return s_import_name_na___2;
		}
		else if(name.m_string == get_export_name_processing().m_string)
		{
			return s_import_name_processing___2;
		}
		else if(name.m_string == get_name_undecorating().m_string)
		{
			return s_import_name_undecorating___2;
		}
		else
		{
			wchar_t const* const ret = m_string_converter.convert(name);
			return ret;
		}
	}
	else
	{
		string_handle const name = pe_get_import_name(iti, eti, dll_idx, imp_idx);
		if(!name.m_string)
		{
			return s_import_name_na___2;
		}
		else if(name.m_string == get_export_name_processing().m_string)
		{
			return s_import_name_processing___2;
		}
		else
		{
			wchar_t const* const ret = m_string_converter.convert(name);
			return ret;
		}
	}
}

std::uint8_t import_window_impl::get_col_icon(pe_import_table_info const& iti, std::uint16_t const dll_idx, std::uint16_t const imp_idx)
{
	std::uint8_t const icon_idx = pe_get_import_icon_id(iti, dll_idx, imp_idx);
	return icon_idx;
}

void import_window_impl::refresh()
{
	LRESULT const redr_off = SendMessageW(m_list_view, WM_SETREDRAW, FALSE, 0);
	assert(redr_off == 0);
	auto const fn_redraw = mk::make_scope_exit([&]()
	{
		LRESULT const auto_sized_pi = SendMessageW(m_list_view, LVM_SETCOLUMNWIDTH, static_cast<int>(e_import_column___2::e_pi), LVSCW_AUTOSIZE);
		assert(auto_sized_pi == TRUE);
		LRESULT const type_sized = SendMessageW(m_list_view, LVM_SETCOLUMNWIDTH, static_cast<int>(e_import_column___2::e_type), get_column_type_max_width());
		assert(type_sized == TRUE);
		LRESULT const ordinal_sized = SendMessageW(m_list_view, LVM_SETCOLUMNWIDTH, static_cast<int>(e_import_column___2::e_ordinal), list_view_base::get_column_uint16_max_width(&m_list_view));
		assert(ordinal_sized == TRUE);
		LRESULT const hint_sized = SendMessageW(m_list_view, LVM_SETCOLUMNWIDTH, static_cast<int>(e_import_column___2::e_hint), list_view_base::get_column_uint16_max_width(&m_list_view));
		assert(hint_sized == TRUE);

		LRESULT const redr_on = SendMessageW(m_list_view, WM_SETREDRAW, TRUE, 0);
		assert(redr_on == 0);
		repaint();
	});

	LRESULT const deleted = SendMessageW(m_list_view, LVM_DELETEALLITEMS, 0, 0);
	assert(deleted == TRUE);
	file_info const* const tmp_fi = m_fi;
	if(!tmp_fi)
	{
		return;
	}
	file_info const* const parent_fi = tmp_fi->m_parent;
	if(parent_fi)
	{
		auto const dll_idx_ = tmp_fi - parent_fi->m_fis;
		assert(dll_idx_ >= 0 && dll_idx_ <= 0xFFFF);
		std::uint16_t const dll_idx = static_cast<std::uint16_t>(dll_idx_);
		std::uint16_t const& count = parent_fi->m_import_table.m_import_counts[dll_idx];
		LRESULT const set_size = SendMessageW(m_list_view, LVM_SETITEMCOUNT, count, 0);
		assert(set_size != 0);
	}
}

void import_window_impl::repaint()
{
	BOOL const redrawn = RedrawWindow(m_list_view, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_FRAME);
	assert(redrawn != 0);
}

int import_window_impl::get_column_type_max_width()
{
	if(g_column_type_max_width != 0)
	{
		return g_column_type_max_width;
	}
	static constexpr std::pair<wchar_t const* const, int const> const s_strings[] =
	{
		{s_import_type_true___2, static_cast<int>(std::size(s_import_type_true___2)) - 1},
		{s_import_type_false___2, static_cast<int>(std::size(s_import_type_false___2)) - 1},
	};
	g_column_type_max_width = list_view_base::get_column_max_width(&m_list_view, &s_strings, static_cast<int>(std::size(s_strings)));
	return g_column_type_max_width;
}
