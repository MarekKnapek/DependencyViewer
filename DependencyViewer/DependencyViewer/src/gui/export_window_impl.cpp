#include "export_window_impl.h"

#include "common_controls.h"
#include "list_view_base.h"
#include "main.h"
#include "processor.h"

#include "../nogui/cassert_my.h"
#include "../nogui/scope_exit.h"

#include "../res/resources.h"

#include <cstdint>
#include <iterator>

#include "../nogui/windows_my.h"

#include <commctrl.h>


enum class e_export_column___2 : std::uint16_t
{
	e_e,
	e_type,
	e_ordinal,
	e_hint,
	e_name,
	e_entry_point,
};
static constexpr wchar_t const* const s_export_headers___2[] =
{
	L"E",
	L"type",
	L"ordinal",
	L"hint",
	L"name",
	L"entry point",
};
static constexpr wchar_t const s_export_type_true___2[] = L"address";
static constexpr wchar_t const s_export_type_false___2[] = L"forwarder";


ATOM export_window_impl::g_class;
int export_window_impl::g_column_type_max_width;


export_window_impl::export_window_impl(HWND const& self) :
	m_self(self),
	m_list_view(),
	m_fi()
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
	for(int i = 0; i != static_cast<int>(std::size(s_export_headers___2)); ++i)
	{
		LVCOLUMNW cl;
		cl.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		cl.fmt = LVCFMT_LEFT;
		cl.cx = 200;
		cl.pszText = const_cast<LPWSTR>(s_export_headers___2[i]);
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

export_window_impl::~export_window_impl()
{
}

void export_window_impl::init()
{
	register_class();
}

void export_window_impl::deinit()
{
	unregister_class();
}

wchar_t const* export_window_impl::get_class_atom()
{
	assert(g_class != 0);
	return reinterpret_cast<wchar_t const*>(g_class);
}

void export_window_impl::register_class()
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
	wc.lpszClassName = L"export_window";
	wc.hIconSm = nullptr;

	assert(g_class == 0);
	g_class = RegisterClassExW(&wc);
	assert(g_class != 0);
}

void export_window_impl::unregister_class()
{
	assert(g_class != 0);
	BOOL const unregistered = UnregisterClassW(reinterpret_cast<wchar_t const*>(g_class), get_instance());
	assert(unregistered != 0);
	g_class = 0;
}

LRESULT CALLBACK export_window_impl::class_proc(HWND const hwnd, UINT const msg, WPARAM const wparam, LPARAM const lparam)
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
			export_window_impl* const self = reinterpret_cast<export_window_impl*>(self_ptr);
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

LRESULT export_window_impl::on_wm_create(HWND const& hwnd, WPARAM const& wparam, LPARAM const& lparam)
{
	export_window_impl* const self = new export_window_impl{hwnd};
	assert((SetLastError(0), true));
	LONG_PTR const prev = SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
	assert(prev == 0 && GetLastError() == 0);

	LRESULT const ret = DefWindowProcW(hwnd, WM_CREATE, wparam, lparam);
	return ret;
}

LRESULT export_window_impl::on_wm_destroy(HWND const& hwnd, WPARAM const& wparam, LPARAM const& lparam)
{
	LONG_PTR const prev = SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
	assert(prev != 0);
	export_window_impl* const self = reinterpret_cast<export_window_impl*>(prev);
	delete self;

	LRESULT const ret = DefWindowProcW(hwnd, WM_DESTROY, wparam, lparam);
	return ret;
}

LRESULT export_window_impl::on_message(UINT const& msg, WPARAM const& wparam, LPARAM const& lparam)
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
		case static_cast<std::uint32_t>(export_window::wm::wm_repaint):
		{
			LRESULT const ret = on_wm_repaint(wparam, lparam);
			return ret;
		}
		break;
		case static_cast<std::uint32_t>(export_window::wm::wm_setfi):
		{
			LRESULT const ret = on_wm_setfi(wparam, lparam);
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

LRESULT export_window_impl::on_wm_size(WPARAM const& wparam, LPARAM const& lparam)
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

LRESULT export_window_impl::on_wm_notify(WPARAM const& wparam, LPARAM const& lparam)
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

LRESULT export_window_impl::on_wm_repaint(WPARAM const& wparam, LPARAM const& lparam)
{
	repaint();

	UINT const msg = static_cast<std::uint32_t>(export_window::wm::wm_repaint);
	LRESULT const ret = DefWindowProcW(m_self, msg, wparam, lparam);
	return ret;
}

LRESULT export_window_impl::on_wm_setfi(WPARAM const& wparam, LPARAM const& lparam)
{
	file_info const* const fi = reinterpret_cast<file_info const*>(lparam);
	m_fi = fi;
	refresh();

	UINT const msg = static_cast<std::uint32_t>(export_window::wm::wm_setfi);
	LRESULT const ret = DefWindowProcW(m_self, msg, wparam, lparam);
	return ret;
}

void export_window_impl::on_getdispinfow(NMHDR& nmhdr)
{
	NMLVDISPINFOW& nm = reinterpret_cast<NMLVDISPINFOW&>(nmhdr);
	if((nm.item.mask & LVIF_TEXT) != 0)
	{
		int const col_ = nm.item.iSubItem;
		assert(col_ >= static_cast<std::uint16_t>(e_export_column___2::e_e));
		assert(col_ <= static_cast<std::uint16_t>(e_export_column___2::e_entry_point));
		e_export_column___2 const col = static_cast<e_export_column___2>(col_);
		switch(col)
		{
			case e_export_column___2::e_e:
			{
				nm.item.pszText = const_cast<wchar_t*>(L"");
			}
			break;
			case e_export_column___2::e_type:
			{
				nm.item.pszText = const_cast<wchar_t*>(L"");
			}
			break;
			case e_export_column___2::e_ordinal:
			{
				nm.item.pszText = const_cast<wchar_t*>(L"");
			}
			break;
			case e_export_column___2::e_hint:
			{
				nm.item.pszText = const_cast<wchar_t*>(L"");
			}
			break;
			case e_export_column___2::e_name:
			{
				nm.item.pszText = const_cast<wchar_t*>(L"");
			}
			break;
			case e_export_column___2::e_entry_point:
			{
				nm.item.pszText = const_cast<wchar_t*>(L"");
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
		nm.item.iImage = 0;
	}
}

void export_window_impl::refresh()
{
	LRESULT const redr_off = SendMessageW(m_list_view, WM_SETREDRAW, FALSE, 0);
	assert(redr_off == 0);
	auto const fn_redraw = mk::make_scope_exit([&]()
	{
		LRESULT const auto_sized_e = SendMessageW(m_list_view, LVM_SETCOLUMNWIDTH, static_cast<int>(e_export_column___2::e_e), LVSCW_AUTOSIZE);
		assert(auto_sized_e == TRUE);
		LRESULT const type_sized = SendMessageW(m_list_view, LVM_SETCOLUMNWIDTH, static_cast<int>(e_export_column___2::e_type), get_column_type_max_width());
		assert(type_sized == TRUE);
		LRESULT const ordinal_sized = SendMessageW(m_list_view, LVM_SETCOLUMNWIDTH, static_cast<int>(e_export_column___2::e_ordinal), list_view_base::get_column_uint16_max_width(&m_list_view));
		assert(ordinal_sized == TRUE);
		LRESULT const hint_sized = SendMessageW(m_list_view, LVM_SETCOLUMNWIDTH, static_cast<int>(e_export_column___2::e_hint), list_view_base::get_column_uint16_max_width(&m_list_view));
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
	file_info const* const fi = tmp_fi->m_orig_instance ? tmp_fi->m_orig_instance : tmp_fi;
	assert(fi);
	std::uint16_t const& count = fi->m_export_table.m_count;
	LRESULT const set_size = SendMessageW(m_list_view, LVM_SETITEMCOUNT, count, 0);
	assert(set_size != 0);
}

void export_window_impl::repaint()
{
	BOOL const redrawn = RedrawWindow(m_list_view, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_FRAME);
	assert(redrawn != 0);
}

int export_window_impl::get_column_type_max_width()
{
	if(g_column_type_max_width != 0)
	{
		return g_column_type_max_width;
	}
	static constexpr std::pair<wchar_t const* const, int const> const s_strings[] =
	{
		{s_export_type_true___2, static_cast<int>(std::size(s_export_type_true___2)) - 1},
		{s_export_type_false___2, static_cast<int>(std::size(s_export_type_false___2)) - 1},
	};
	g_column_type_max_width = list_view_base::get_column_max_width(&m_list_view, &s_strings, static_cast<int>(std::size(s_strings)));
	return g_column_type_max_width;
}
