#include "modules_window_impl.h"

#include "common_controls.h"
#include "file_info_getters.h"
#include "list_view_base.h"
#include "main.h"
#include "processor.h"

#include "../nogui/cassert_my.h"
#include "../nogui/scope_exit.h"

#include <algorithm>
#include <iterator>
#include <numeric>

#include "../nogui/windows_my.h"

#include <commctrl.h>


enum class e_modules_menu_id___2 : std::uint16_t
{
	e_matching,
};
static constexpr wchar_t const s_modules_menu_str_matching___2[] = L"&Highlight Matching Module In Tree\tCtrl+M";

enum class e_modules_column___2
{
	e_name,
	e_path,
};
static constexpr wchar_t const* const s_modules_headers___2[] =
{
	L"name",
	L"path",
};


ATOM modules_window_impl::g_class;


modules_window_impl::modules_window_impl(HWND const& self) :
	m_self(self),
	m_list_view(),
	m_modlist(),
	m_sort_col(0xFF),
	m_sort(),
	m_string_converter(),
	m_context_menu(),
	m_cmd_matching_fn(),
	m_cmd_matching_ctx()
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
	for(int i = 0; i != static_cast<int>(std::size(s_modules_headers___2)); ++i)
	{
		LVCOLUMNW cl;
		cl.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		cl.fmt = LVCFMT_LEFT;
		cl.cx = 200;
		cl.pszText = const_cast<LPWSTR>(s_modules_headers___2[i]);
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

	refresh();
}

modules_window_impl::~modules_window_impl()
{
}

void modules_window_impl::init()
{
	register_class();
}

void modules_window_impl::deinit()
{
	unregister_class();
}

wchar_t const* modules_window_impl::get_class_atom()
{
	assert(g_class != 0);
	return reinterpret_cast<wchar_t const*>(g_class);
}

void modules_window_impl::register_class()
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
	wc.lpszClassName = L"modules_window";
	wc.hIconSm = nullptr;

	assert(g_class == 0);
	g_class = RegisterClassExW(&wc);
	assert(g_class != 0);
}

void modules_window_impl::unregister_class()
{
	assert(g_class != 0);
	BOOL const unregistered = UnregisterClassW(reinterpret_cast<wchar_t const*>(g_class), get_instance());
	assert(unregistered != 0);
	g_class = 0;
}

LRESULT CALLBACK modules_window_impl::class_proc(HWND const hwnd, UINT const msg, WPARAM const wparam, LPARAM const lparam)
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
			modules_window_impl* const self = reinterpret_cast<modules_window_impl*>(self_ptr);
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

LRESULT modules_window_impl::on_wm_create(HWND const& hwnd, WPARAM const& wparam, LPARAM const& lparam)
{
	modules_window_impl* const self = new modules_window_impl{hwnd};
	assert((SetLastError(0), true));
	LONG_PTR const prev = SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
	assert(prev == 0 && GetLastError() == 0);

	LRESULT const ret = DefWindowProcW(hwnd, WM_CREATE, wparam, lparam);
	return ret;
}

LRESULT modules_window_impl::on_wm_destroy(HWND const& hwnd, WPARAM const& wparam, LPARAM const& lparam)
{
	LONG_PTR const prev = SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
	assert(prev != 0);
	modules_window_impl* const self = reinterpret_cast<modules_window_impl*>(prev);
	delete self;

	LRESULT const ret = DefWindowProcW(hwnd, WM_DESTROY, wparam, lparam);
	return ret;
}

LRESULT modules_window_impl::on_message(UINT const& msg, WPARAM const& wparam, LPARAM const& lparam)
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
		case static_cast<std::uint32_t>(modules_window::wm::wm_repaint):
		{
			LRESULT const ret = on_wm_repaint(wparam, lparam);
			return ret;
		}
		break;
		case static_cast<std::uint32_t>(modules_window::wm::wm_setmodlist):
		{
			LRESULT const ret = on_wm_setmodlist(wparam, lparam);
			return ret;
		}
		break;
		case static_cast<std::uint32_t>(modules_window::wm::wm_setcmdmatching):
		{
			LRESULT const ret = on_wm_setcmdmatching(wparam, lparam);
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

LRESULT modules_window_impl::on_wm_size(WPARAM const& wparam, LPARAM const& lparam)
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

LRESULT modules_window_impl::on_wm_notify(WPARAM const& wparam, LPARAM const& lparam)
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
			case LVN_COLUMNCLICK:
			{
				on_columnclick(nmhdr);
			}
			break;
		}
	}

	LRESULT const ret = DefWindowProcW(m_self, WM_NOTIFY, wparam, lparam);
	return ret;
}

LRESULT modules_window_impl::on_wm_repaint(WPARAM const& wparam, LPARAM const& lparam)
{
	repaint();

	UINT const msg = static_cast<std::uint32_t>(modules_window::wm::wm_repaint);
	LRESULT const ret = DefWindowProcW(m_self, msg, wparam, lparam);
	return ret;
}

LRESULT modules_window_impl::on_wm_setmodlist(WPARAM const& wparam, LPARAM const& lparam)
{
	modules_list_t const* const modlist = reinterpret_cast<modules_list_t const*>(lparam);
	m_modlist = modlist;
	refresh();

	UINT const msg = static_cast<std::uint32_t>(modules_window::wm::wm_setmodlist);
	LRESULT const ret = DefWindowProcW(m_self, msg, wparam, lparam);
	return ret;
}

LRESULT modules_window_impl::on_wm_setcmdmatching(WPARAM const& wparam, LPARAM const& lparam)
{
	static_assert(sizeof(wparam) == sizeof(modules_window::cmd_matching_fn_t), "");
	static_assert(sizeof(lparam) == sizeof(modules_window::cmd_matching_ctx_t), "");
	auto const cmd_matching_fn = reinterpret_cast<modules_window::cmd_matching_fn_t>(wparam);
	auto const cmd_matching_ctx = reinterpret_cast<modules_window::cmd_matching_ctx_t>(lparam);
	m_cmd_matching_fn = cmd_matching_fn;
	m_cmd_matching_ctx = cmd_matching_ctx;

	UINT const msg = static_cast<std::uint32_t>(modules_window::wm::wm_setcmdmatching);
	LRESULT const ret = DefWindowProcW(m_self, msg, wparam, lparam);
	return ret;
}

void modules_window_impl::on_getdispinfow(NMHDR& nmhdr)
{
	NMLVDISPINFOW& nm = reinterpret_cast<NMLVDISPINFOW&>(nmhdr);
	modules_list_t const* const modlist = m_modlist;
	if(!modlist)
	{
		return;
	}
	if((nm.item.mask & LVIF_TEXT) != 0)
	{
		int const row = nm.item.iItem;
		assert(row >= 0 && row <= 0xFFFF);
		std::uint16_t const idx = static_cast<std::uint16_t>(row);
		int const col_ = nm.item.iSubItem;
		assert(col_ >= static_cast<std::uint16_t>(e_modules_column___2::e_name));
		assert(col_ <= static_cast<std::uint16_t>(e_modules_column___2::e_path));
		e_modules_column___2 const col = static_cast<e_modules_column___2>(col_);
		switch(col)
		{
			case e_modules_column___2::e_name:
			{
				nm.item.pszText = const_cast<wchar_t*>(get_col_name(idx));
			}
			break;
			case e_modules_column___2::e_path:
			{
				nm.item.pszText = const_cast<wchar_t*>(get_col_path(idx));
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

void modules_window_impl::on_columnclick(NMHDR& nmhdr)
{
	NMLISTVIEW const& nmlv = reinterpret_cast<NMLISTVIEW&>(nmhdr);
	int const new_sort = list_view_base::on_columnclick(&nmlv, static_cast<int>(std::size(s_modules_headers___2)), m_sort_col);
	assert(new_sort >= 0 && new_sort <= 0xFF);
	m_sort_col = static_cast<std::uint8_t>(new_sort);
	sort_view();
	list_view_base::refresh_headers(&m_list_view, static_cast<int>(std::size(s_modules_headers___2)), m_sort_col);
	repaint();
}

wchar_t const* modules_window_impl::get_col_name(std::uint16_t const& idx)
{
	std::uint16_t const& idx_sorted = m_sort.empty() ? idx : m_sort[idx];
	assert(m_modlist);
	wstring const ret = get_modules_list_col_name(*m_modlist, idx_sorted, m_string_converter);
	return ret.m_str;
}

wchar_t const* modules_window_impl::get_col_path(std::uint16_t const& idx)
{
	std::uint16_t const& idx_sorted = m_sort.empty() ? idx : m_sort[idx];
	assert(m_modlist);
	wstring const ret = get_modules_list_col_path(*m_modlist, idx_sorted);
	return ret.m_str;
}

smart_menu modules_window_impl::create_context_menu()
{
	HMENU const menu = CreatePopupMenu();
	assert(menu);
	smart_menu menu_sp{menu};
	MENUITEMINFOW mi{};
	mi.cbSize = sizeof(mi);
	mi.fMask = MIIM_ID | MIIM_STRING | MIIM_FTYPE;
	mi.fType = MFT_STRING;
	mi.wID = static_cast<std::uint16_t>(e_modules_menu_id___2::e_matching);
	mi.dwTypeData = const_cast<wchar_t*>(s_modules_menu_str_matching___2);
	BOOL const inserted = InsertMenuItemW(menu, 0, TRUE, &mi);
	assert(inserted != 0);
	return menu_sp;
}

bool modules_window_impl::command_matching_available(std::uint16_t const& item_idx, file_info const** const out_fi)
{
	modules_list_t const* const modlist = m_modlist;
	if(!modlist)
	{
		return false;
	}
	assert(item_idx < modlist->m_count);
	file_info const* const fi = modlist->m_list[item_idx];
	assert(fi);
	assert(!fi->m_orig_instance);
	bool const available = true;
	if(available && out_fi)
	{
		*out_fi = fi;
	}
	return available;
}

void modules_window_impl::command_matching()
{
	int const sel = list_view_base::get_selection(&m_list_view);
	if(sel == -1)
	{
		return;
	}
	assert(sel >= 0 && sel <= 0xFFFF);
	std::uint16_t const line_idx = static_cast<std::uint16_t>(sel);
	std::uint16_t const item_idx = m_sort.empty() ? line_idx : m_sort[line_idx];

	file_info const* fi;
	bool const available = command_matching_available(item_idx, &fi);
	if(!available)
	{
		return;
	}

	if(!m_cmd_matching_fn)
	{
		return;
	}
	m_cmd_matching_fn(m_cmd_matching_ctx, fi);
}

void modules_window_impl::refresh()
{
	LRESULT const redr_off = SendMessageW(m_list_view, WM_SETREDRAW, FALSE, 0);
	assert(redr_off == 0);
	auto const fn_redraw = mk::make_scope_exit([&]()
	{
		sort_view();

		LRESULT const redr_on = SendMessageW(m_list_view, WM_SETREDRAW, TRUE, 0);
		assert(redr_on == 0);
		repaint();
	});

	LRESULT const deleted = SendMessageW(m_list_view, LVM_DELETEALLITEMS, 0, 0);
	assert(deleted == TRUE);
	modules_list_t const* const modlist = m_modlist;
	if(!modlist)
	{
		return;
	}
	std::uint16_t const& count = modlist->m_count;
	LRESULT const set_size = SendMessageW(m_list_view, LVM_SETITEMCOUNT, count, 0);
	assert(set_size != 0);
}

void modules_window_impl::repaint()
{
	BOOL const redrawn = RedrawWindow(m_list_view, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_FRAME);
	assert(redrawn != 0);
}

void modules_window_impl::sort_view()
{
	m_sort.clear();
	std::uint8_t const cur_sort_raw = m_sort_col;
	if(cur_sort_raw == 0xFF)
	{
		return;
	}
	bool const asc = (cur_sort_raw & (1u << 7u)) == 0u;
	std::uint8_t const col_ = cur_sort_raw &~ (1u << 7u);
	modules_list_t const* const modlist = m_modlist;
	if(!modlist)
	{
		return;
	}
	std::uint16_t const n_items = modlist->m_count;
	assert(n_items <= 0xFFFF / 2);
	m_sort.resize(n_items * 2);
	std::iota(m_sort.begin(), m_sort.begin() + n_items, std::uint16_t{0});
	std::uint16_t* const sort = m_sort.data();
	assert(col_ >= static_cast<std::uint16_t>(e_modules_column___2::e_name));
	assert(col_ <= static_cast<std::uint16_t>(e_modules_column___2::e_path));
	e_modules_column___2 const col = static_cast<e_modules_column___2>(col_);
	switch(col)
	{
		case e_modules_column___2::e_name:
		{
			auto const fn_compare_name = [&](std::uint16_t const a, std::uint16_t const b) -> bool
			{
				wstring const a2 = get_modules_list_col_name(*modlist, a, m_string_converter);
				wstring const b2 = get_modules_list_col_name(*modlist, b, m_string_converter);
				bool const ret = wstring_case_insensitive_less{}(a2, b2);
				return ret;
			};
			if(asc)
			{
				std::sort(sort, sort + n_items, [&](auto const& a, auto const& b){ return fn_compare_name(a, b); });
			}
			else
			{
				std::sort(sort, sort + n_items, [&](auto const& a, auto const& b){ return fn_compare_name(b, a); });
			}
		}
		break;
		case e_modules_column___2::e_path:
		{
			auto const fn_compare_path = [&](std::uint16_t const a, std::uint16_t const b) -> bool
			{
				wstring const a2 = get_modules_list_col_path(*modlist, a);
				wstring const b2 = get_modules_list_col_path(*modlist, b);
				bool const ret = wstring_case_insensitive_less{}(a2, b2);
				return ret;
			};
			if(asc)
			{
				std::sort(sort, sort + n_items, [&](auto const& a, auto const& b){ return fn_compare_path(a, b); });
			}
			else
			{
				std::sort(sort, sort + n_items, [&](auto const& a, auto const& b){ return fn_compare_path(b, a); });
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
