#include "tree_window_impl.h"

#include "common_controls.h"
#include "file_info_getters.h"
#include "main.h"
#include "processor.h"
#include "tree_algos.h"

#include "../nogui/cassert_my.h"
#include "../nogui/scope_exit.h"

#include "../res/resources.h"

#include <iterator>

#include "../nogui/windows_my.h"

#include <commctrl.h>
#include <windowsx.h>


enum class e_tree_menu_id___2 : std::uint16_t
{
	e_matching,
	e_orig,
	e_prev,
	e_next,
	e_expand,
	e_collapse,
	e_properties,
};
static constexpr wchar_t const s_tree_menu_str_matching[] = L"&Highlight Matching Module In List\tCtrl+M";
static constexpr wchar_t const s_tree_menu_str_orig___2[] = L"Highlight &Original Instance\tCtrl+K";
static constexpr wchar_t const s_tree_menu_str_prev___2[] = L"Highlight Previous &Instance\tCtrl+B";
static constexpr wchar_t const s_tree_menu_str_next___2[] = L"Highlight &Next Instance\tCtrl+N";
static constexpr wchar_t const s_tree_menu_str_expand___2[] = L"&Expand All\tCtrl+E";
static constexpr wchar_t const s_tree_menu_str_collapse___2[] = L"Co&llapse All\tCtrl+W";
static constexpr wchar_t const s_tree_menu_str_properties___2[] = L"&Properties...\tAlt+Enter";

enum class e_tree_accel_id : std::uint16_t
{
	e_matching,
	e_orig,
	e_prev,
	e_next,
	e_expand,
	e_collapse,
	e_properties,
};
static constexpr ACCEL const s_tree_accel_table[] =
{
	{FVIRTKEY | FCONTROL, 'M', static_cast<std::uint16_t>(e_tree_accel_id::e_matching)},
	{FVIRTKEY | FCONTROL, 'K', static_cast<std::uint16_t>(e_tree_accel_id::e_orig)},
	{FVIRTKEY | FCONTROL, 'B', static_cast<std::uint16_t>(e_tree_accel_id::e_prev)},
	{FVIRTKEY | FCONTROL, 'N', static_cast<std::uint16_t>(e_tree_accel_id::e_next)},
	{FVIRTKEY | FCONTROL, 'E', static_cast<std::uint16_t>(e_tree_accel_id::e_expand)},
	{FVIRTKEY | FCONTROL, 'W', static_cast<std::uint16_t>(e_tree_accel_id::e_collapse)},
	{FVIRTKEY | FALT, VK_RETURN, static_cast<std::uint16_t>(e_tree_accel_id::e_properties)},
};


ATOM tree_window_impl::g_class;
HACCEL tree_window_impl::g_accel;


tree_window_impl::tree_window_impl(HWND const& self) :
	m_self(self),
	m_tree_view(),
	m_fi(),
	m_fullpaths(false),
	m_string_converter(),
	m_context_menu(),
	m_onitemchanged_fn(),
	m_onitemchanged_ctx(),
	m_cmd_matching_fn(),
	m_cmd_matching_ctx(),
	m_cmd_properties_fn(),
	m_cmd_properties_ctx()
{
	assert(self != nullptr);

	DWORD const ex_style = WS_EX_CLIENTEDGE;
	wchar_t const* const class_name = WC_TREEVIEWW;
	wchar_t const* const window_name = nullptr;
	DWORD const style = (WS_VISIBLE | WS_CHILD | WS_TABSTOP) | (TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_DISABLEDRAGDROP | TVS_SHOWSELALWAYS);
	int const x_pos = 0;
	int const y_pos = 0;
	int const width = 0;
	int const height = 0;
	HWND const parent = m_self;
	HMENU const menu = nullptr;
	HINSTANCE const instance = get_instance();
	LPVOID const param = nullptr;
	m_tree_view = CreateWindowExW(ex_style, class_name, window_name, style, x_pos, y_pos, width, height, parent, menu, instance, param);
	assert(m_tree_view != nullptr);

	unsigned const extended_tv_styles = TVS_EX_DOUBLEBUFFER;
	LRESULT const style_set = SendMessageW(m_tree_view, TVM_SETEXTENDEDSTYLE, extended_tv_styles, extended_tv_styles);
	assert(style_set == S_OK);

	HIMAGELIST const tree_img_list = common_controls::ImageList_LoadImageW(get_instance(), MAKEINTRESOURCEW(s_res_icons_tree), 26, 0, CLR_DEFAULT, IMAGE_BITMAP, LR_DEFAULTCOLOR);
	assert(tree_img_list);
	LRESULT const prev_img_list = SendMessageW(m_tree_view, TVM_SETIMAGELIST, TVSIL_NORMAL, reinterpret_cast<LPARAM>(tree_img_list));
	assert(!prev_img_list);
}

tree_window_impl::~tree_window_impl()
{
}

void tree_window_impl::init()
{
	register_class();
	create_accel_table();
}

void tree_window_impl::deinit()
{
	destroy_accel_table();
	unregister_class();
}

wchar_t const* tree_window_impl::get_class_atom()
{
	assert(g_class != 0);
	return reinterpret_cast<wchar_t const*>(g_class);
}

void tree_window_impl::register_class()
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
	wc.lpszClassName = L"tree_window";
	wc.hIconSm = nullptr;

	assert(g_class == 0);
	g_class = RegisterClassExW(&wc);
	assert(g_class != 0);
}

void tree_window_impl::unregister_class()
{
	assert(g_class != 0);
	BOOL const unregistered = UnregisterClassW(reinterpret_cast<wchar_t const*>(g_class), get_instance());
	assert(unregistered != 0);
	g_class = 0;
}

void tree_window_impl::create_accel_table()
{
	assert(g_accel == nullptr);
	g_accel = CreateAcceleratorTableW(const_cast<ACCEL*>(s_tree_accel_table), static_cast<int>(std::size(s_tree_accel_table)));
	assert(g_accel != nullptr);
}

void tree_window_impl::destroy_accel_table()
{
	assert(g_accel != nullptr);
	BOOL const destroyed = DestroyAcceleratorTable(g_accel);
	assert(destroyed != 0);
	g_accel = nullptr;
}

LRESULT CALLBACK tree_window_impl::class_proc(HWND const hwnd, UINT const msg, WPARAM const wparam, LPARAM const lparam)
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
			tree_window_impl* const self = reinterpret_cast<tree_window_impl*>(self_ptr);
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

LRESULT tree_window_impl::on_wm_create(HWND const& hwnd, WPARAM const& wparam, LPARAM const& lparam)
{
	tree_window_impl* const self = new tree_window_impl{hwnd};
	assert((SetLastError(0), true));
	LONG_PTR const prev = SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
	assert(prev == 0 && GetLastError() == 0);

	LRESULT const ret = DefWindowProcW(hwnd, WM_CREATE, wparam, lparam);
	return ret;
}

LRESULT tree_window_impl::on_wm_destroy(HWND const& hwnd, WPARAM const& wparam, LPARAM const& lparam)
{
	LONG_PTR const prev = SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
	assert(prev != 0);
	tree_window_impl* const self = reinterpret_cast<tree_window_impl*>(prev);
	delete self;

	LRESULT const ret = DefWindowProcW(hwnd, WM_DESTROY, wparam, lparam);
	return ret;
}

LRESULT tree_window_impl::on_message(UINT const& msg, WPARAM const& wparam, LPARAM const& lparam)
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
		case WM_CONTEXTMENU:
		{
			LRESULT const ret = on_wm_contextmenu(wparam, lparam);
			return ret;
		}
		break;
		case WM_COMMAND:
		{
			LRESULT const ret = on_wm_command(wparam, lparam);
			return ret;
		}
		break;
		case static_cast<std::uint32_t>(tree_window::wm::wm_repaint):
		{
			LRESULT const ret = on_wm_repaint(wparam, lparam);
			return ret;
		}
		break;
		case static_cast<std::uint32_t>(tree_window::wm::wm_translateaccelerator):
		{
			LRESULT const ret = on_wm_translateaccelerator(wparam, lparam);
			return ret;
		}
		break;
		case static_cast<std::uint32_t>(tree_window::wm::wm_setfi):
		{
			LRESULT const ret = on_wm_setfi(wparam, lparam);
			return ret;
		}
		break;
		case static_cast<std::uint32_t>(tree_window::wm::wm_getselection):
		{
			LRESULT const ret = on_wm_getselection(wparam, lparam);
			return ret;
		}
		break;
		case static_cast<std::uint32_t>(tree_window::wm::wm_selectitem):
		{
			LRESULT const ret = on_wm_selectitem(wparam, lparam);
			return ret;
		}
		break;
		case static_cast<std::uint32_t>(tree_window::wm::wm_iscmdpropertiesavail):
		{
			LRESULT const ret = on_wm_iscmdpropertiesavail(wparam, lparam);
			return ret;
		}
		break;
		case static_cast<std::uint32_t>(tree_window::wm::wm_setfullpaths):
		{
			LRESULT const ret = on_wm_setfullpaths(wparam, lparam);
			return ret;
		}
		break;
		case static_cast<std::uint32_t>(tree_window::wm::wm_setonitemchanged):
		{
			LRESULT const ret = on_wm_setonitemchanged(wparam, lparam);
			return ret;
		}
		break;
		case static_cast<std::uint32_t>(tree_window::wm::wm_setcmdmatching):
		{
			LRESULT const ret = on_wm_setcmdmatching(wparam, lparam);
			return ret;
		}
		break;
		case static_cast<std::uint32_t>(tree_window::wm::wm_setcmdproperties):
		{
			LRESULT const ret = on_wm_setcmdproperties(wparam, lparam);
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

LRESULT tree_window_impl::on_wm_size(WPARAM const& wparam, LPARAM const& lparam)
{
	RECT rect;
	BOOL const got = GetClientRect(m_self, &rect);
	assert(got != 0);
	assert(rect.left == 0 && rect.top == 0);
	BOOL const moved = MoveWindow(m_tree_view, 0, 0, rect.right, rect.bottom, TRUE);
	assert(moved != 0);

	LRESULT const ret = DefWindowProcW(m_self, WM_SIZE, wparam, lparam);
	return ret;
}

LRESULT tree_window_impl::on_wm_notify(WPARAM const& wparam, LPARAM const& lparam)
{
	NMHDR& nmhdr = *reinterpret_cast<NMHDR*>(lparam);
	if(nmhdr.hwndFrom == m_tree_view)
	{
		switch(nmhdr.code)
		{
			case TVN_GETDISPINFOW:
			{
				on_getdispinfow(nmhdr);
			}
			break;
			case TVN_SELCHANGEDW:
			{
				on_selchangedw(nmhdr);
			}
			break;
		}
	}

	LRESULT const ret = DefWindowProcW(m_self, WM_NOTIFY, wparam, lparam);
	return ret;
}

LRESULT tree_window_impl::on_wm_contextmenu(WPARAM const& wparam, LPARAM const& lparam)
{
	auto const fn_on_wm_contextmenu = [&]()
	{
		file_info const* fi;
		POINT screen_pos;
		bool const got_menu = get_fi_and_point_for_ctx_menu(lparam, &fi, &screen_pos);
		if(!got_menu)
		{
			return;
		}
		if(!m_context_menu)
		{
			m_context_menu = create_context_menu();
		}
		HMENU const menu = reinterpret_cast<HMENU>(m_context_menu.get());

		bool const avail_matching = cmd_matching_avail(fi, nullptr);
		BOOL const prev_matching = EnableMenuItem(menu, static_cast<std::uint16_t>(e_tree_menu_id___2::e_matching), MF_BYCOMMAND | (avail_matching ? MF_ENABLED : (MF_GRAYED | MF_DISABLED)));
		assert(prev_matching != -1 && (prev_matching == MF_ENABLED || prev_matching == (MF_GRAYED | MF_DISABLED)));

		bool const avail_orig = cmd_orig_avail(fi, nullptr);
		BOOL const prev_orig = EnableMenuItem(menu, static_cast<std::uint16_t>(e_tree_menu_id___2::e_orig), MF_BYCOMMAND | (avail_orig ? MF_ENABLED : (MF_GRAYED | MF_DISABLED)));
		assert(prev_orig != -1 && (prev_orig == MF_ENABLED || prev_orig == (MF_GRAYED | MF_DISABLED)));

		bool const avail_prev = cmd_prev_avail(fi, nullptr);
		BOOL const prev_prev = EnableMenuItem(menu, static_cast<std::uint16_t>(e_tree_menu_id___2::e_prev), MF_BYCOMMAND | (avail_prev ? MF_ENABLED : (MF_GRAYED | MF_DISABLED)));
		assert(prev_prev != -1 && (prev_prev == MF_ENABLED || prev_prev == (MF_GRAYED | MF_DISABLED)));

		bool const avail_next = cmd_next_avail(fi, nullptr);
		BOOL const prev_next = EnableMenuItem(menu, static_cast<std::uint16_t>(e_tree_menu_id___2::e_next), MF_BYCOMMAND | (avail_next ? MF_ENABLED : (MF_GRAYED | MF_DISABLED)));
		assert(prev_next != -1 && (prev_next == MF_ENABLED || prev_next == (MF_GRAYED | MF_DISABLED)));

		bool const avail_properties = cmd_properties_avail(fi, nullptr);
		BOOL const prev_properties = EnableMenuItem(menu, static_cast<std::uint16_t>(e_tree_menu_id___2::e_properties), MF_BYCOMMAND | (avail_properties ? MF_ENABLED : (MF_GRAYED | MF_DISABLED)));
		assert(prev_properties != -1 && (prev_properties == MF_ENABLED || prev_properties == (MF_GRAYED | MF_DISABLED)));

		BOOL const tracked = TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON | TPM_NOANIMATION, screen_pos.x, screen_pos.y, 0, m_self, nullptr);
		assert(tracked != 0);
	};
	fn_on_wm_contextmenu();

	LRESULT const ret = DefWindowProcW(m_self, WM_CONTEXTMENU, wparam, lparam);
	return ret;
}

LRESULT tree_window_impl::on_wm_command(WPARAM const& wparam, LPARAM const& lparam)
{
	if(HIWORD(wparam) == 0 && lparam == 0)
	{
		on_menu(wparam);
	}
	else if(HIWORD(wparam) == 1 && lparam == 0)
	{
		on_accelerator(wparam);
	}
	LRESULT const ret = DefWindowProcW(m_self, WM_COMMAND, wparam, lparam);
	return ret;
}

LRESULT tree_window_impl::on_wm_repaint(WPARAM const& wparam, LPARAM const& lparam)
{
	repaint();

	UINT const msg = static_cast<std::uint32_t>(tree_window::wm::wm_repaint);
	LRESULT const ret = DefWindowProcW(m_self, msg, wparam, lparam);
	return ret;
}

LRESULT tree_window_impl::on_wm_translateaccelerator(WPARAM const& wparam, LPARAM const& lparam)
{
	bool& translated = *reinterpret_cast<bool*>(wparam);
	MSG* const message = reinterpret_cast<MSG*>(lparam);

	assert(g_accel != nullptr);
	int const trnsltd = TranslateAcceleratorW(m_self, g_accel, message);
	translated = trnsltd != 0;

	UINT const msg = static_cast<std::uint32_t>(tree_window::wm::wm_translateaccelerator);
	LRESULT const ret = DefWindowProcW(m_self, msg, wparam, lparam);
	return ret;
}

LRESULT tree_window_impl::on_wm_setfi(WPARAM const& wparam, LPARAM const& lparam)
{
	file_info* const fi = reinterpret_cast<file_info*>(lparam);
	m_fi = fi;
	refresh(fi);

	UINT const msg = static_cast<std::uint32_t>(tree_window::wm::wm_setfi);
	LRESULT const ret = DefWindowProcW(m_self, msg, wparam, lparam);
	return ret;
}

LRESULT tree_window_impl::on_wm_getselection(WPARAM const& wparam, LPARAM const& lparam)
{
	file_info const** const pfi = reinterpret_cast<file_info const**>(lparam);
	file_info const* const fi = get_selection();
	assert(pfi);
	*pfi = fi;

	UINT const msg = static_cast<std::uint32_t>(tree_window::wm::wm_getselection);
	LRESULT const ret = DefWindowProcW(m_self, msg, wparam, lparam);
	return ret;
}

LRESULT tree_window_impl::on_wm_selectitem(WPARAM const& wparam, LPARAM const& lparam)
{
	file_info const* const fi = reinterpret_cast<file_info const*>(lparam);
	select_item(fi);

	UINT const msg = static_cast<std::uint32_t>(tree_window::wm::wm_selectitem);
	LRESULT const ret = DefWindowProcW(m_self, msg, wparam, lparam);
	return ret;
}

LRESULT tree_window_impl::on_wm_iscmdpropertiesavail(WPARAM const& wparam, LPARAM const& lparam)
{
	auto const fn_is_avail = [&]() -> bool
	{
		file_info const* const selection_fi = get_selection();
		if(!selection_fi)
		{
			return false;
		}
		bool const avail = cmd_properties_avail(selection_fi, nullptr);
		return avail;
	};

	bool& avail = *reinterpret_cast<bool*>(lparam);
	avail = fn_is_avail();

	UINT const msg = static_cast<std::uint32_t>(tree_window::wm::wm_iscmdpropertiesavail);
	LRESULT const ret = DefWindowProcW(m_self, msg, wparam, lparam);
	return ret;
}

LRESULT tree_window_impl::on_wm_setfullpaths(WPARAM const& wparam, LPARAM const& lparam)
{
	assert(wparam == 0 || wparam == 1);
	bool const fullpaths = wparam != 0;
	m_fullpaths = fullpaths;
	repaint();

	UINT const msg = static_cast<std::uint32_t>(tree_window::wm::wm_setfullpaths);
	LRESULT const ret = DefWindowProcW(m_self, msg, wparam, lparam);
	return ret;
}

LRESULT tree_window_impl::on_wm_setonitemchanged(WPARAM const& wparam, LPARAM const& lparam)
{
	static_assert(sizeof(wparam) == sizeof(tree_window::onitemchanged_fn_t), "");
	static_assert(sizeof(lparam) == sizeof(tree_window::onitemchanged_ctx_t), "");
	auto const onitemchanged_fn = reinterpret_cast<tree_window::onitemchanged_fn_t>(wparam);
	auto const onitemchanged_ctx = reinterpret_cast<tree_window::onitemchanged_ctx_t>(lparam);
	m_onitemchanged_fn = onitemchanged_fn;
	m_onitemchanged_ctx = onitemchanged_ctx;

	UINT const msg = static_cast<std::uint32_t>(tree_window::wm::wm_setonitemchanged);
	LRESULT const ret = DefWindowProcW(m_self, msg, wparam, lparam);
	return ret;
}

LRESULT tree_window_impl::on_wm_setcmdmatching(WPARAM const& wparam, LPARAM const& lparam)
{
	static_assert(sizeof(wparam) == sizeof(tree_window::cmd_matching_fn_t), "");
	static_assert(sizeof(lparam) == sizeof(tree_window::cmd_matching_ctx_t), "");
	auto const cmd_matching_fn = reinterpret_cast<tree_window::cmd_matching_fn_t>(wparam);
	auto const cmd_matching_ctx = reinterpret_cast<tree_window::cmd_matching_ctx_t>(lparam);
	m_cmd_matching_fn = cmd_matching_fn;
	m_cmd_matching_ctx = cmd_matching_ctx;

	UINT const msg = static_cast<std::uint32_t>(tree_window::wm::wm_setcmdmatching);
	LRESULT const ret = DefWindowProcW(m_self, msg, wparam, lparam);
	return ret;
}

LRESULT tree_window_impl::on_wm_setcmdproperties(WPARAM const& wparam, LPARAM const& lparam)
{
	static_assert(sizeof(wparam) == sizeof(tree_window::cmd_properties_fn_t), "");
	static_assert(sizeof(lparam) == sizeof(tree_window::cmd_properties_ctx_t), "");
	auto const cmd_properties_fn = reinterpret_cast<tree_window::cmd_properties_fn_t>(wparam);
	auto const cmd_properties_ctx = reinterpret_cast<tree_window::cmd_properties_ctx_t>(lparam);
	m_cmd_properties_fn = cmd_properties_fn;
	m_cmd_properties_ctx = cmd_properties_ctx;

	UINT const msg = static_cast<std::uint32_t>(tree_window::wm::wm_setcmdproperties);
	LRESULT const ret = DefWindowProcW(m_self, msg, wparam, lparam);
	return ret;
}

void tree_window_impl::on_getdispinfow(NMHDR& nmhdr)
{
	NMTVDISPINFOW& di = reinterpret_cast<NMTVDISPINFOW&>(nmhdr);
	assert(di.item.lParam != 0);
	file_info const* const fi = reinterpret_cast<file_info const*>(di.item.lParam);
	if((di.item.mask & TVIF_TEXT) != 0)
	{
		if(m_fullpaths)
		{
			wstring const str = get_tree_fi_path(fi, m_string_converter);
			assert(str);
			di.item.pszText = const_cast<wchar_t*>(str.m_str);
		}
		else
		{
			wstring const str = get_tree_fi_name(fi, m_string_converter);
			assert(str);
			di.item.pszText = const_cast<wchar_t*>(str.m_str);
		}
	}
	if((di.item.mask & (TVIF_IMAGE | TVIF_SELECTEDIMAGE)) != 0)
	{
		di.item.iImage = fi->m_icon;
		di.item.iSelectedImage = di.item.iImage;
	}
}

void tree_window_impl::on_menu(WPARAM const& wparam)
{
	std::uint16_t const menu_id_ = static_cast<std::uint16_t>(LOWORD(wparam));
	assert(menu_id_ >= static_cast<std::uint16_t>(e_tree_menu_id___2::e_matching));
	assert(menu_id_ <= static_cast<std::uint16_t>(e_tree_menu_id___2::e_properties));
	e_tree_menu_id___2 const menu_id = static_cast<e_tree_menu_id___2>(menu_id_);
	switch(menu_id)
	{
		case e_tree_menu_id___2::e_matching:
		{
			on_menu_matching();
		}
		break;
		case e_tree_menu_id___2::e_orig:
		{
			on_menu_orig();
		}
		break;
		case e_tree_menu_id___2::e_prev:
		{
			on_menu_prev();
		}
		break;
		case e_tree_menu_id___2::e_next:
		{
			on_menu_next();
		}
		break;
		case e_tree_menu_id___2::e_expand:
		{
			on_menu_expand();
		}
		break;
		case e_tree_menu_id___2::e_collapse:
		{
			on_menu_collapse();
		}
		break;
		case e_tree_menu_id___2::e_properties:
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

void tree_window_impl::on_accelerator(WPARAM const& wparam)
{
	std::uint16_t const accel_id_ = static_cast<std::uint16_t>(LOWORD(wparam));
	assert(accel_id_ >= static_cast<std::uint16_t>(e_tree_accel_id::e_matching));
	assert(accel_id_ <= static_cast<std::uint16_t>(e_tree_accel_id::e_properties));
	e_tree_accel_id const accel_id = static_cast<e_tree_accel_id>(accel_id_);
	switch(accel_id)
	{
		case e_tree_accel_id::e_matching:
		{
			on_accel_matching();
		}
		break;
		case e_tree_accel_id::e_orig:
		{
			on_accel_orig();
		}
		break;
		case e_tree_accel_id::e_prev:
		{
			on_accel_prev();
		}
		break;
		case e_tree_accel_id::e_next:
		{
			on_accel_next();
		}
		break;
		case e_tree_accel_id::e_expand:
		{
			on_accel_expand();
		}
		break;
		case e_tree_accel_id::e_collapse:
		{
			on_accel_collapse();
		}
		break;
		case e_tree_accel_id::e_properties:
		{
			on_accel_properties();
		}
		break;
		default:
		{
			assert(false);
		}
		break;
	}
}

void tree_window_impl::on_menu_matching()
{
	cmd_matching();
}

void tree_window_impl::on_menu_orig()
{
	cmd_orig();
}

void tree_window_impl::on_menu_prev()
{
	cmd_prev();
}

void tree_window_impl::on_menu_next()
{
	cmd_next();
}

void tree_window_impl::on_menu_expand()
{
	cmd_expand();
}

void tree_window_impl::on_menu_collapse()
{
	cmd_collapse();
}

void tree_window_impl::on_menu_properties()
{
	cmd_properties();
}

void tree_window_impl::on_accel_matching()
{
	cmd_matching();
}

void tree_window_impl::on_accel_orig()
{
	cmd_orig();
}

void tree_window_impl::on_accel_prev()
{
	cmd_prev();
}

void tree_window_impl::on_accel_next()
{
	cmd_next();
}

void tree_window_impl::on_accel_expand()
{
	cmd_expand();
}

void tree_window_impl::on_accel_collapse()
{
	cmd_collapse();
}

void tree_window_impl::on_accel_properties()
{
	cmd_properties();
}

void tree_window_impl::on_selchangedw([[maybe_unused]] NMHDR& nmhdr)
{
	file_info const* const fi = get_selection();
	if(m_onitemchanged_fn)
	{
		m_onitemchanged_fn(m_onitemchanged_ctx, fi);
	}
}

file_info const* tree_window_impl::get_selection()
{
	LRESULT const selected_ = SendMessageW(m_tree_view, TVM_GETNEXTITEM, TVGN_CARET, 0);
	if(selected_ == 0)
	{
		return nullptr;
	}
	HTREEITEM const selected = reinterpret_cast<HTREEITEM>(selected_);
	file_info const* const fi = htreeitem_2_file_info(reinterpret_cast<htreeitem>(selected));
	assert(fi);
	return fi;
}

void tree_window_impl::select_item(file_info const* const& fi)
{
	assert(fi);
	assert(fi->m_tree_item != nullptr);
	LRESULT const visibled = SendMessageW(m_tree_view, TVM_ENSUREVISIBLE, 0, reinterpret_cast<LPARAM>(fi->m_tree_item));
	LRESULT const selected = SendMessageW(m_tree_view, TVM_SELECTITEM, TVGN_CARET, reinterpret_cast<LPARAM>(fi->m_tree_item));
	assert(selected == TRUE);
	[[maybe_unused]] HWND const prev_focus = SetFocus(m_tree_view);
	assert(prev_focus != nullptr);
}

smart_menu tree_window_impl::create_context_menu()
{
	static constexpr wchar_t const* const s_strings[] =
	{
		s_tree_menu_str_matching,
		s_tree_menu_str_orig___2,
		s_tree_menu_str_prev___2,
		s_tree_menu_str_next___2,
		s_tree_menu_str_expand___2,
		s_tree_menu_str_collapse___2,
		s_tree_menu_str_properties___2,
	};
	static constexpr e_tree_menu_id___2 const s_ids[] =
	{
		e_tree_menu_id___2::e_matching,
		e_tree_menu_id___2::e_orig,
		e_tree_menu_id___2::e_prev,
		e_tree_menu_id___2::e_next,
		e_tree_menu_id___2::e_expand,
		e_tree_menu_id___2::e_collapse,
		e_tree_menu_id___2::e_properties,
	};
	static_assert(std::size(s_strings) == std::size(s_ids), "");

	HMENU const menu = CreatePopupMenu();
	assert(menu);
	smart_menu menu_sp{menu};
	for(int i = 0; i != static_cast<int>(std::size(s_strings)); ++i)
	{
		MENUITEMINFOW mi{};
		mi.cbSize = sizeof(mi);
		mi.fMask = MIIM_ID | MIIM_STRING | MIIM_FTYPE;
		mi.fType = MFT_STRING;
		mi.wID = static_cast<std::uint16_t>(s_ids[i]);
		mi.dwTypeData = const_cast<wchar_t*>(s_strings[i]);
		BOOL const inserted = InsertMenuItemW(menu, i, TRUE, &mi);
		assert(inserted != 0);
	}
	return menu_sp;
}

bool tree_window_impl::cmd_matching_avail(file_info const* const& fi, file_info const** const& out_fi)
{
	if(!fi)
	{
		return false;
	}
	file_info const* const proper_fi = fi->m_orig_instance ? fi->m_orig_instance : fi;
	assert(proper_fi);
	bool const avail = true;
	if(avail && out_fi)
	{
		*out_fi = proper_fi;
	}
	return avail;
}

void tree_window_impl::cmd_matching()
{
	file_info const* const curr_fi = get_selection();
	file_info const* fi;
	bool const avail = cmd_matching_avail(curr_fi, &fi);
	if(!avail)
	{
		return;
	}
	assert(fi);
	assert(m_cmd_matching_fn);
	m_cmd_matching_fn(m_cmd_matching_ctx, fi);
}

bool tree_window_impl::cmd_orig_avail(file_info const* const& fi, file_info const** const& out_fi)
{
	if(!fi)
	{
		return false;
	}
	file_info const* const orig = fi->m_orig_instance;
	bool const avail = !!orig;
	if(avail && out_fi)
	{
		*out_fi = orig;
	}
	return avail;
}

void tree_window_impl::cmd_orig()
{
	file_info const* const curr_fi = get_selection();
	file_info const* fi;
	bool const avail = cmd_orig_avail(curr_fi, &fi);
	if(!avail)
	{
		return;
	}
	assert(fi);
	select_item(fi);
}

bool tree_window_impl::cmd_prev_avail(file_info const* const& fi, file_info const** const& out_fi)
{
	if(!fi)
	{
		return false;
	}
	file_info const* const orig = fi->m_orig_instance;
	if(!orig)
	{
		return false;
	}
	file_info const* const prev = fi->m_prev_instance;
	assert(prev);
	if(out_fi)
	{
		*out_fi = prev;
	}
	return true;
}

void tree_window_impl::cmd_prev()
{
	file_info const* const curr_fi = get_selection();
	file_info const* fi;
	bool const avail = cmd_prev_avail(curr_fi, &fi);
	if(!avail)
	{
		return;
	}
	assert(fi);
	select_item(fi);
}

bool tree_window_impl::cmd_next_avail(file_info const* const& fi, file_info const** const& out_fi)
{
	if(!fi)
	{
		return false;
	}
	file_info const* const next = fi->m_next_instance;
	if(!next)
	{
		return false;
	}
	file_info const* const orig = fi->m_orig_instance;
	if(next == orig)
	{
		return false;
	}
	if(out_fi)
	{
		*out_fi = next;
	}
	return true;
}

void tree_window_impl::cmd_next()
{
	file_info const* const curr_fi = get_selection();
	file_info const* fi;
	bool const avail = cmd_next_avail(curr_fi, &fi);
	if(!avail)
	{
		return;
	}
	assert(fi);
	select_item(fi);
}

void tree_window_impl::cmd_expand()
{
	static constexpr auto const expand_fn = [](file_info const* const& fi, void* const& param)
	{
		assert(fi);
		assert(param);
		HWND const& hwnd = *static_cast<HWND const*>(param);
		assert(IsWindow(hwnd) != 0);
		HTREEITEM const& item = reinterpret_cast<HTREEITEM>(fi->m_tree_item);
		[[maybe_unused]] LRESULT expanded = SendMessageW(hwnd, TVM_EXPAND, TVE_EXPAND, reinterpret_cast<LPARAM>(item));
	};

	file_info const* const fi = m_fi;
	if(!fi)
	{
		return;
	}
	file_info const* const selectioin = get_selection();
	LRESULT const redr_off = SendMessageW(m_tree_view, WM_SETREDRAW, FALSE, 0);
	assert(redr_off == 0);
	auto const fn_red_on = mk::make_scope_exit([&]()
	{
		LRESULT const redr_on = SendMessageW(m_tree_view, WM_SETREDRAW, TRUE, 0);
		assert(redr_on == 0);
		repaint();
		if(selectioin)
		{
			select_item(selectioin);
		}
	});
	depth_first_visit(fi, expand_fn, static_cast<void*>(&m_tree_view));
}

void tree_window_impl::cmd_collapse()
{
	static constexpr auto const collapse_fn = [](file_info const* const& fi, void* const& param)
	{
		assert(fi);
		assert(param);
		HWND const& hwnd = *static_cast<HWND const*>(param);
		assert(IsWindow(hwnd) != 0);
		HTREEITEM const& item = reinterpret_cast<HTREEITEM>(fi->m_tree_item);
		[[maybe_unused]] LRESULT expanded = SendMessageW(hwnd, TVM_EXPAND, TVE_COLLAPSE, reinterpret_cast<LPARAM>(item));
	};

	file_info const* const fi = m_fi;
	if(!fi)
	{
		return;
	}
	LRESULT const redr_off = SendMessageW(m_tree_view, WM_SETREDRAW, FALSE, 0);
	assert(redr_off == 0);
	auto const fn_red_on = mk::make_scope_exit([&]()
	{
		LRESULT const redr_on = SendMessageW(m_tree_view, WM_SETREDRAW, TRUE, 0);
		assert(redr_on == 0);
		repaint();
		select_item(fi->m_fis + 0);
	});
	select_item(fi->m_fis + 0);
	children_first_visit(fi, collapse_fn, static_cast<void*>(&m_tree_view));
}

bool tree_window_impl::cmd_properties_avail(file_info const* const& tmp_fi, wstring_handle* const& out_file_path)
{
	if(!tmp_fi)
	{
		return false;
	}
	file_info const* const fi = tmp_fi->m_orig_instance ? tmp_fi->m_orig_instance : tmp_fi;
	wstring_handle const& file_path = fi->m_file_path;
	if(!file_path)
	{
		return false;
	}
	if(out_file_path)
	{
		*out_file_path = file_path;
	}
	return true;
}

void tree_window_impl::cmd_properties()
{
	file_info const* const selection_fi = get_selection();
	if(!selection_fi)
	{
		return;
	}
	wstring_handle file_path;
	bool const avail = cmd_properties_avail(selection_fi, &file_path);
	if(!avail)
	{
		return;
	}
	if(m_cmd_properties_fn)
	{
		m_cmd_properties_fn(m_cmd_properties_ctx, file_path);
	}
}

void tree_window_impl::refresh(file_info* const& fi)
{
	LRESULT const redr_off = SendMessageW(m_tree_view, WM_SETREDRAW, FALSE, 0);
	assert(redr_off == 0);
	auto const fn_redraw = mk::make_scope_exit([&]()
	{
		LRESULT const redr_on = SendMessageW(m_tree_view, WM_SETREDRAW, TRUE, 0);
		assert(redr_on == 0);
		repaint();
	});
	LRESULT const deselected = SendMessageW(m_tree_view, TVM_SELECTITEM, TVGN_CARET, reinterpret_cast<LPARAM>(nullptr));
	assert(deselected == TRUE);
	LRESULT const deleted = SendMessageW(m_tree_view, TVM_DELETEITEM, 0, reinterpret_cast<LPARAM>(TVI_ROOT));
	assert(deleted == TRUE);

	if(!fi)
	{
		return;
	}
	htreeitem const parent = reinterpret_cast<htreeitem>(TVI_ROOT);
	refresh_r(fi, parent);
	std::uint16_t const n = fi->m_import_table.m_normal_dll_count + fi->m_import_table.m_delay_dll_count;
	for(std::uint16_t i = 0; i != n; ++i)
	{
		file_info* const& sub_fi = fi->m_fis + i;
		assert(sub_fi->m_tree_item != nullptr);
		LRESULT const expanded = SendMessageW(m_tree_view, TVM_EXPAND, TVE_EXPAND, reinterpret_cast<LPARAM>(sub_fi->m_tree_item));
	}
}

void tree_window_impl::refresh_r(file_info* const& fi, htreeitem const& parent_ti)
{
	if(!fi)
	{
		return;
	}
	std::uint16_t const n = fi->m_import_table.m_normal_dll_count + fi->m_import_table.m_delay_dll_count;
	for(std::uint16_t i = 0; i != n; ++i)
	{
		file_info* const& sub_fi = fi->m_fis + i;
		refresh_e(sub_fi, parent_ti);
	}
}

void tree_window_impl::refresh_e(file_info* const& fi, htreeitem const& parent_ti)
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
	tvi.itemex.lParam = reinterpret_cast<LPARAM>(fi);
	tvi.itemex.iIntegral = 0;
	tvi.itemex.uStateEx = 0;
	tvi.itemex.hwnd = nullptr;
	tvi.itemex.iExpandedImage = 0;
	tvi.itemex.iReserved = 0;
	LRESULT const ti_ = SendMessageW(m_tree_view, TVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&tvi));
	HTREEITEM const ti = reinterpret_cast<HTREEITEM>(ti_);
	assert(ti != nullptr);
	assert(fi->m_tree_item == nullptr);
	fi->m_tree_item = reinterpret_cast<htreeitem>(ti);
	htreeitem const parent = reinterpret_cast<htreeitem>(ti);
	refresh_r(fi, parent);
}

void tree_window_impl::repaint()
{
	BOOL const redrawn = RedrawWindow(m_tree_view, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_FRAME);
	assert(redrawn != 0);
}

bool tree_window_impl::get_fi_and_point_for_ctx_menu(LPARAM const& lparam, file_info const** const out_fi, POINT* const out_point)
{
	assert(out_fi);
	assert(out_point);
	if(lparam == LPARAM{-1})
	{
		file_info const* const selection = get_selection();
		if(!selection)
		{
			return false;
		}
		RECT rect;
		*reinterpret_cast<HTREEITEM*>(&rect) = reinterpret_cast<HTREEITEM>(selection->m_tree_item);
		LRESULT const got_rect = SendMessageW(m_tree_view, TVM_GETITEMRECT, TRUE, reinterpret_cast<LPARAM>(&rect));
		if(got_rect == FALSE)
		{
			return false;
		}
		POINT cursor_screen;
		cursor_screen.x = rect.left + (rect.right - rect.left) / 2;
		cursor_screen.y = rect.top + (rect.bottom - rect.top) / 2;
		BOOL const converted = ClientToScreen(m_tree_view, &cursor_screen);
		assert(converted != 0);
		*out_point = cursor_screen;
		*out_fi = selection;
		return true;
	}
	else
	{
		POINT cursor_screen;
		cursor_screen.x = GET_X_LPARAM(lparam);
		cursor_screen.y = GET_Y_LPARAM(lparam);
		POINT cursor_client = cursor_screen;
		BOOL const converted = ScreenToClient(m_tree_view, &cursor_client);
		assert(converted != 0);
		TVHITTESTINFO hti;
		hti.pt = cursor_client;
		LRESULT const hit_tested = SendMessageW(m_tree_view, TVM_HITTEST, 0, reinterpret_cast<LPARAM>(&hti));
		if(hit_tested != 0 && (hti.flags & (TVHT_ONITEM | TVHT_ONITEMBUTTON | TVHT_ONITEMICON | TVHT_ONITEMLABEL | TVHT_ONITEMSTATEICON)) != 0)
		{
			assert(reinterpret_cast<HTREEITEM>(hit_tested) == hti.hItem);
			file_info const* const fi = htreeitem_2_file_info(reinterpret_cast<htreeitem>(hti.hItem));
			assert(fi);
			select_item(fi);
			*out_fi = fi;
			*out_point = cursor_screen;
			return true;
		}
		else
		{
			*out_fi = nullptr;
			*out_point = cursor_screen;
			return true;
		}
	}
}

file_info const* tree_window_impl::htreeitem_2_file_info(htreeitem const& hti)
{
	assert(hti);
	TVITEMW ti;
	ti.hItem = reinterpret_cast<HTREEITEM>(hti);
	ti.mask = TVIF_PARAM;
	LRESULT const got_item = SendMessageW(m_tree_view, TVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&ti));
	assert(got_item == TRUE);
	assert(ti.lParam != 0);
	file_info const* const ret = reinterpret_cast<file_info const*>(ti.lParam);
	return ret;
}
