#include "toolbar_window_impl.h"

#include "main.h"

#include "../nogui/cassert_my.h"

#include "../res/resources.h"

#include <cstdint>

#include "../nogui/windows_my.h"

#include <commctrl.h>


enum class e_toolbar : std::uint16_t
{
	e_open,
	e_full_paths,
	e_undecorate,
	e_properties,
};
static constexpr wchar_t const s_toolbar_tooltip_open[] = L"Open... (Ctrl+O)";
static constexpr wchar_t const s_toolbar_tooltip_full_paths[] = L"View Full Paths (F9)";
static constexpr wchar_t const s_toolbar_tooltip_undecorate[] = L"Undecorate C++ Functions (F10)";
static constexpr wchar_t const s_toolbar_tooltip_properties[] = L"Properties... (Alt+Enter)";


ATOM toolbar_window_impl::g_class;
int toolbar_window_impl::g_debug_instances;


toolbar_window_impl::toolbar_window_impl(HWND const& self) :
	m_self(self),
	m_toolbar(),
	m_cmd_open_fn(),
	m_cmd_open_ctx(),
	m_cmd_fullpaths_fn(),
	m_cmd_fullpaths_ctx(),
	m_cmd_undecorate_fn(),
	m_cmd_undecorate_ctx(),
	m_cmd_properties_fn(),
	m_cmd_properties_ctx()
{
	assert(self != nullptr);

	++g_debug_instances;

	LONG_PTR const got_parent = GetWindowLongPtrW(m_self, GWLP_HWNDPARENT);
	assert(got_parent != 0);
	HWND const my_parent = reinterpret_cast<HWND>(got_parent);
	assert(IsWindow(my_parent) != 0);
	RECT parent_rect;
	BOOL const got_parent_rect = GetClientRect(my_parent, &parent_rect);
	assert(got_parent_rect != 0);
	assert(parent_rect.left == 0);
	assert(parent_rect.top == 0);
	BOOL const moved_0 = MoveWindow(m_self, 0, 0, parent_rect.right, parent_rect.bottom, TRUE);
	assert(moved_0 != 0);

	DWORD const ex_style = 0;
	wchar_t const* const class_name = TOOLBARCLASSNAMEW;
	wchar_t const* const window_name = nullptr;
	DWORD const style = (WS_VISIBLE | WS_CHILD) | (TBSTYLE_TOOLTIPS);
	int const x_pos = 0;
	int const y_pos = 0;
	int const width = 0;
	int const height = 0;
	HWND const parent = m_self;
	HMENU const menu = nullptr;
	HINSTANCE const instance = get_instance();
	LPVOID const param = nullptr;
	m_toolbar = CreateWindowExW(ex_style, class_name, window_name, style, x_pos, y_pos, width, height, parent, menu, instance, param);
	assert(m_toolbar != nullptr);
	LRESULT const size_sent = SendMessageW(m_toolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

	TBADDBITMAP button_bitmap;
	button_bitmap.hInst = get_instance();
	button_bitmap.nID = s_res_icons_toolbar;
	int const s_number_of_buttons = 4;
	LRESULT const bitmap_added = SendMessageW(m_toolbar, TB_ADDBITMAP, s_number_of_buttons, reinterpret_cast<LPARAM>(&button_bitmap));
	assert(bitmap_added != -1);

	TBBUTTON buttons[s_number_of_buttons]{};
	buttons[0].iBitmap = s_res_icon_open;
	buttons[0].idCommand = static_cast<std::uint16_t>(e_toolbar::e_open);
	buttons[0].fsState = TBSTATE_ENABLED;
	buttons[0].fsStyle = BTNS_BUTTON;
	buttons[0].dwData = 0;
	buttons[0].iString = 0;
	buttons[1].iBitmap = s_res_icon_full_paths;
	buttons[1].idCommand = static_cast<std::uint16_t>(e_toolbar::e_full_paths);
	buttons[1].fsState = TBSTATE_ENABLED;
	buttons[1].fsStyle = BTNS_BUTTON;
	buttons[1].dwData = 0;
	buttons[1].iString = 0;
	buttons[2].iBitmap = s_res_icon_undecorate;
	buttons[2].idCommand = static_cast<std::uint16_t>(e_toolbar::e_undecorate);
	buttons[2].fsState = TBSTATE_ENABLED;
	buttons[2].fsStyle = BTNS_BUTTON;
	buttons[2].dwData = 0;
	buttons[2].iString = 0;
	buttons[3].iBitmap = s_res_icon_properties;
	buttons[3].idCommand = static_cast<std::uint16_t>(e_toolbar::e_properties);
	buttons[3].fsState = TBSTATE_ENABLED;
	buttons[3].fsStyle = BTNS_BUTTON;
	buttons[3].dwData = 0;
	buttons[3].iString = 0;
	LRESULT const buttons_added = SendMessageW(m_toolbar, TB_ADDBUTTONSW, s_number_of_buttons, reinterpret_cast<LPARAM>(&buttons));
	assert(buttons_added == TRUE);

	LRESULT const auto_sized = SendMessageW(m_toolbar, TB_AUTOSIZE, 0, 0);
	RECT toolbar_rect;
	BOOL const got_rect = GetClientRect(m_toolbar, &toolbar_rect);
	assert(got_rect != 0);
	assert(toolbar_rect.left == 0);
	assert(toolbar_rect.top == 0);
	BOOL const moved_1 = MoveWindow(m_self, 0, 0, toolbar_rect.right, toolbar_rect.bottom, TRUE);
	assert(moved_1 != 0);
}

toolbar_window_impl::~toolbar_window_impl()
{
	--g_debug_instances;
}

void toolbar_window_impl::init()
{
	register_class();
}

void toolbar_window_impl::deinit()
{
	unregister_class();
}

wchar_t const* toolbar_window_impl::get_class_atom()
{
	assert(g_class != 0);
	return reinterpret_cast<wchar_t const*>(g_class);
}

void toolbar_window_impl::register_class()
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
	wc.lpszClassName = L"toolbar_window";
	wc.hIconSm = nullptr;

	assert(g_class == 0);
	g_class = RegisterClassExW(&wc);
	assert(g_class != 0);
}

void toolbar_window_impl::unregister_class()
{
	assert(g_debug_instances == 0);

	assert(g_class != 0);
	BOOL const unregistered = UnregisterClassW(reinterpret_cast<wchar_t const*>(g_class), get_instance());
	assert(unregistered != 0);
	g_class = 0;
}

LRESULT CALLBACK toolbar_window_impl::class_proc(HWND const hwnd, UINT const msg, WPARAM const wparam, LPARAM const lparam)
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
			toolbar_window_impl* const self = reinterpret_cast<toolbar_window_impl*>(self_ptr);
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

LRESULT toolbar_window_impl::on_wm_create(HWND const& hwnd, WPARAM const& wparam, LPARAM const& lparam)
{
	toolbar_window_impl* const self = new toolbar_window_impl{hwnd};
	assert((SetLastError(0), true));
	LONG_PTR const prev = SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
	assert(prev == 0 && GetLastError() == 0);

	LRESULT const ret = DefWindowProcW(hwnd, WM_CREATE, wparam, lparam);
	return ret;
}

LRESULT toolbar_window_impl::on_wm_destroy(HWND const& hwnd, WPARAM const& wparam, LPARAM const& lparam)
{
	LONG_PTR const prev = SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
	assert(prev != 0);
	toolbar_window_impl* const self = reinterpret_cast<toolbar_window_impl*>(prev);
	delete self;

	LRESULT const ret = DefWindowProcW(hwnd, WM_DESTROY, wparam, lparam);
	return ret;
}

LRESULT toolbar_window_impl::on_message(UINT const& msg, WPARAM const& wparam, LPARAM const& lparam)
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
		case WM_COMMAND:
		{
			LRESULT const ret = on_wm_command(wparam, lparam);
			return ret;
		}
		break;
		case static_cast<std::uint32_t>(toolbar_window::wm::wm_setfullpathspressed):
		{
			LRESULT const ret = on_wm_setfullpathspressed(wparam, lparam);
			return ret;
		}
		break;
		case static_cast<std::uint32_t>(toolbar_window::wm::wm_setundecoratepressed):
		{
			LRESULT const ret = on_wm_setundecoratepressed(wparam, lparam);
			return ret;
		}
		break;
		case static_cast<std::uint32_t>(toolbar_window::wm::wm_setpropertiesavail):
		{
			LRESULT const ret = on_wm_setpropertiesavail(wparam, lparam);
			return ret;
		}
		break;
		case static_cast<std::uint32_t>(toolbar_window::wm::wm_setcmdopen):
		{
			LRESULT const ret = on_wm_setcmdopen(wparam, lparam);
			return ret;
		}
		break;
		case static_cast<std::uint32_t>(toolbar_window::wm::wm_setcmdfullpaths):
		{
			LRESULT const ret = on_wm_setcmdfullpaths(wparam, lparam);
			return ret;
		}
		break;
		case static_cast<std::uint32_t>(toolbar_window::wm::wm_setcmdundecorate):
		{
			LRESULT const ret = on_wm_setcmdundecorate(wparam, lparam);
			return ret;
		}
		break;
		case static_cast<std::uint32_t>(toolbar_window::wm::wm_setcmdproperties):
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

LRESULT toolbar_window_impl::on_wm_size(WPARAM const& wparam, LPARAM const& lparam)
{
	on_size();

	LRESULT const ret = DefWindowProcW(m_self, WM_NOTIFY, wparam, lparam);
	return ret;
}

LRESULT toolbar_window_impl::on_wm_notify(WPARAM const& wparam, LPARAM const& lparam)
{
	NMHDR& nmhdr = *reinterpret_cast<NMHDR*>(lparam);
	if(nmhdr.hwndFrom == m_toolbar)
	{
		switch(nmhdr.code)
		{
			case TBN_GETINFOTIPW:
			{
				on_getinfotipw(nmhdr);
			}
			break;
		}
	}

	LRESULT const ret = DefWindowProcW(m_self, WM_NOTIFY, wparam, lparam);
	return ret;
}

LRESULT toolbar_window_impl::on_wm_command(WPARAM const& wparam, LPARAM const& lparam)
{
	if(reinterpret_cast<HWND>(lparam) == m_toolbar)
	{
		on_toolbar_cmd(wparam);
	}
	LRESULT const ret = DefWindowProcW(m_self, WM_NOTIFY, wparam, lparam);
	return ret;
}

LRESULT toolbar_window_impl::on_wm_setfullpathspressed(WPARAM const& wparam, LPARAM const& lparam)
{
	assert(wparam == 0 || wparam == 1);
	bool const pressed = wparam != 0;
	setfullpathspressed(pressed);

	UINT const msg = static_cast<std::uint32_t>(toolbar_window::wm::wm_setfullpathspressed);
	LRESULT const ret = DefWindowProcW(m_self, msg, wparam, lparam);
	return ret;
}

LRESULT toolbar_window_impl::on_wm_setundecoratepressed(WPARAM const& wparam, LPARAM const& lparam)
{
	assert(wparam == 0 || wparam == 1);
	bool const pressed = wparam != 0;
	setundecoratepressed(pressed);

	UINT const msg = static_cast<std::uint32_t>(toolbar_window::wm::wm_setundecoratepressed);
	LRESULT const ret = DefWindowProcW(m_self, msg, wparam, lparam);
	return ret;
}

LRESULT toolbar_window_impl::on_wm_setpropertiesavail(WPARAM const& wparam, LPARAM const& lparam)
{
	assert(wparam == 0 || wparam == 1);
	bool const available = wparam != 0;
	setpropertiesavail(available);

	UINT const msg = static_cast<std::uint32_t>(toolbar_window::wm::wm_setpropertiesavail);
	LRESULT const ret = DefWindowProcW(m_self, msg, wparam, lparam);
	return ret;
}

LRESULT toolbar_window_impl::on_wm_setcmdopen(WPARAM const& wparam, LPARAM const& lparam)
{
	static_assert(sizeof(wparam) == sizeof(toolbar_window::cmd_open_fn_t), "");
	static_assert(sizeof(lparam) == sizeof(toolbar_window::cmd_open_ctx_t), "");
	auto const cmd_open_fn = reinterpret_cast<toolbar_window::cmd_open_fn_t>(wparam);
	auto const cmd_open_ctx = reinterpret_cast<toolbar_window::cmd_open_ctx_t>(lparam);
	m_cmd_open_fn = cmd_open_fn;
	m_cmd_open_ctx = cmd_open_ctx;

	UINT const msg = static_cast<std::uint32_t>(toolbar_window::wm::wm_setcmdopen);
	LRESULT const ret = DefWindowProcW(m_self, msg, wparam, lparam);
	return ret;
}

LRESULT toolbar_window_impl::on_wm_setcmdfullpaths(WPARAM const& wparam, LPARAM const& lparam)
{
	static_assert(sizeof(wparam) == sizeof(toolbar_window::cmd_fullpaths_fn_t), "");
	static_assert(sizeof(lparam) == sizeof(toolbar_window::cmd_fullpaths_ctx_t), "");
	auto const cmd_fullpaths_fn = reinterpret_cast<toolbar_window::cmd_fullpaths_fn_t>(wparam);
	auto const cmd_fullpaths_ctx = reinterpret_cast<toolbar_window::cmd_fullpaths_ctx_t>(lparam);
	m_cmd_fullpaths_fn = cmd_fullpaths_fn;
	m_cmd_fullpaths_ctx = cmd_fullpaths_ctx;

	UINT const msg = static_cast<std::uint32_t>(toolbar_window::wm::wm_setcmdfullpaths);
	LRESULT const ret = DefWindowProcW(m_self, msg, wparam, lparam);
	return ret;
}

LRESULT toolbar_window_impl::on_wm_setcmdundecorate(WPARAM const& wparam, LPARAM const& lparam)
{
	static_assert(sizeof(wparam) == sizeof(toolbar_window::cmd_undecorate_fn_t), "");
	static_assert(sizeof(lparam) == sizeof(toolbar_window::cmd_undecorate_ctx_t), "");
	auto const cmd_undecorate_fn = reinterpret_cast<toolbar_window::cmd_undecorate_fn_t>(wparam);
	auto const cmd_undecorate_ctx = reinterpret_cast<toolbar_window::cmd_undecorate_ctx_t>(lparam);
	m_cmd_undecorate_fn = cmd_undecorate_fn;
	m_cmd_undecorate_ctx = cmd_undecorate_ctx;

	UINT const msg = static_cast<std::uint32_t>(toolbar_window::wm::wm_setcmdundecorate);
	LRESULT const ret = DefWindowProcW(m_self, msg, wparam, lparam);
	return ret;
}

LRESULT toolbar_window_impl::on_wm_setcmdproperties(WPARAM const& wparam, LPARAM const& lparam)
{
	static_assert(sizeof(wparam) == sizeof(toolbar_window::cmd_properties_fn_t), "");
	static_assert(sizeof(lparam) == sizeof(toolbar_window::cmd_properties_ctx_t), "");
	auto const cmd_properties_fn = reinterpret_cast<toolbar_window::cmd_properties_fn_t>(wparam);
	auto const cmd_properties_ctx = reinterpret_cast<toolbar_window::cmd_properties_ctx_t>(lparam);
	m_cmd_properties_fn = cmd_properties_fn;
	m_cmd_properties_ctx = cmd_properties_ctx;

	UINT const msg = static_cast<std::uint32_t>(toolbar_window::wm::wm_setcmdproperties);
	LRESULT const ret = DefWindowProcW(m_self, msg, wparam, lparam);
	return ret;
}

void toolbar_window_impl::on_size()
{
	RECT rect;
	BOOL const got_rect = GetClientRect(m_self, &rect);
	assert(got_rect != 0);
	assert(rect.left == 0 && rect.top == 0);
	BOOL const moved = MoveWindow(m_toolbar, 0, 0, rect.right, rect.bottom, TRUE); assert(moved != 0);
}

void toolbar_window_impl::on_getinfotipw(NMHDR& nmhdr)
{
	NMTBGETINFOTIPW& tbgit = reinterpret_cast<NMTBGETINFOTIPW&>(nmhdr);
	int const toolbar_id_ = tbgit.iItem;
	assert(toolbar_id_ >= static_cast<std::uint16_t>(e_toolbar::e_open));
	assert(toolbar_id_ <= static_cast<std::uint16_t>(e_toolbar::e_properties));
	e_toolbar const toolbar_id = static_cast<e_toolbar>(toolbar_id_);
	switch(toolbar_id)
	{
		case e_toolbar::e_open:
		{
			tbgit.pszText = const_cast<wchar_t*>(s_toolbar_tooltip_open);
		}
		break;
		case e_toolbar::e_full_paths:
		{
			tbgit.pszText = const_cast<wchar_t*>(s_toolbar_tooltip_full_paths);
		}
		break;
		case e_toolbar::e_undecorate:
		{
			tbgit.pszText = const_cast<wchar_t*>(s_toolbar_tooltip_undecorate);
		}
		break;
		case e_toolbar::e_properties:
		{
			tbgit.pszText = const_cast<wchar_t*>(s_toolbar_tooltip_properties);
		}
		break;
		default:
		{
			assert(false);
		}
		break;
	}
}

void toolbar_window_impl::on_toolbar_cmd(WPARAM const& wparam)
{
	auto const toolbar_id_ = LOWORD(wparam);
	assert(toolbar_id_ >= static_cast<std::uint16_t>(e_toolbar::e_open));
	assert(toolbar_id_ <= static_cast<std::uint16_t>(e_toolbar::e_properties));
	e_toolbar const toolbar_id = static_cast<e_toolbar>(toolbar_id_);
	switch(toolbar_id)
	{
		case e_toolbar::e_open:
		{
			cmd_open();
		}
		break;
		case e_toolbar::e_full_paths:
		{
			cmd_full_paths();
		}
		break;
		case e_toolbar::e_undecorate:
		{
			cmd_undecorate();
		}
		break;
		case e_toolbar::e_properties:
		{
			cmd_properties();
		}
		break;
		default:
		{
			assert(false);
		}
		break;
	}
}

void toolbar_window_impl::setfullpathspressed(bool const& pressed)
{
	LRESULT const state_set = SendMessageW(m_toolbar, TB_SETSTATE, static_cast<std::uint16_t>(e_toolbar::e_full_paths), (pressed ? TBSTATE_PRESSED : 0) | TBSTATE_ENABLED);
	assert(state_set == TRUE);
}

void toolbar_window_impl::setundecoratepressed(bool const& pressed)
{
	LRESULT const state_set = SendMessageW(m_toolbar, TB_SETSTATE, static_cast<std::uint16_t>(e_toolbar::e_undecorate), (pressed ? TBSTATE_PRESSED : 0) | TBSTATE_ENABLED);
	assert(state_set == TRUE);
}

void toolbar_window_impl::setpropertiesavail(bool const& available)
{
	WPARAM const properties_command_id = static_cast<std::uint16_t>(e_toolbar::e_properties);
	LRESULT const curr_state = SendMessageW(m_toolbar, TB_GETSTATE, properties_command_id, 0);
	assert(curr_state != -1);
	LPARAM new_state = static_cast<LPARAM>(curr_state);
	if(available)
	{
		new_state |= TBSTATE_ENABLED;
	}
	else
	{
		new_state &=~ TBSTATE_ENABLED;
	}
	LRESULT const set = SendMessageW(m_toolbar, TB_SETSTATE, properties_command_id, new_state);
	assert(set == TRUE);
}

void toolbar_window_impl::cmd_open()
{
	assert(m_cmd_open_fn);
	m_cmd_open_fn(m_cmd_open_ctx);
}

void toolbar_window_impl::cmd_full_paths()
{
	assert(m_cmd_fullpaths_fn);
	m_cmd_fullpaths_fn(m_cmd_fullpaths_ctx);
}

void toolbar_window_impl::cmd_undecorate()
{
	assert(m_cmd_undecorate_fn);
	m_cmd_undecorate_fn(m_cmd_undecorate_ctx);
}

void toolbar_window_impl::cmd_properties()
{
	assert(m_cmd_properties_fn);
	m_cmd_properties_fn(m_cmd_properties_ctx);
}
