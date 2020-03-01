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


ATOM toolbar_window_impl::g_class;


toolbar_window_impl::toolbar_window_impl(HWND const& self) :
	m_self(self),
	m_toolbar()
{
	assert(self != nullptr);

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
	LRESULT const ret = DefWindowProcW(m_self, msg, wparam, lparam);
	return ret;
}
