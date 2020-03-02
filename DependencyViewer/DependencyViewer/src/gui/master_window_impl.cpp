#include "master_window_impl.h"

#include "main.h"

#include "../nogui/cassert_my.h"


ATOM master_window_impl::g_class;
int master_window_impl::g_debug_instances;


master_window_impl::master_window_impl(HWND const& self) :
	m_self(self),
	m_toolbar_window(self),
	m_main_panel(self),
	m_upper_panel(m_main_panel.get_hwnd()),
	m_tree_window(m_upper_panel.get_hwnd()),
	m_right_panel(m_upper_panel.get_hwnd()),
	m_import_window(m_right_panel.get_hwnd()),
	m_export_window(m_right_panel.get_hwnd()),
	m_modules_window(m_main_panel.get_hwnd())
{
	assert(self != nullptr);

	++g_debug_instances;

	m_main_panel.setchildren(m_upper_panel.get_hwnd(), m_modules_window.get_hwnd());
	m_upper_panel.setchildren(m_tree_window.get_hwnd(), m_right_panel.get_hwnd());
	m_right_panel.setchildren(m_import_window.get_hwnd(), m_export_window.get_hwnd());

	m_main_panel.setposition(0.8f);
	m_upper_panel.setposition(0.333f);
}

master_window_impl::~master_window_impl()
{
	--g_debug_instances;
}

void master_window_impl::init()
{
	register_class();
}

void master_window_impl::deinit()
{
	unregister_class();
}

wchar_t const* master_window_impl::get_class_atom()
{
	assert(g_class != 0);
	return reinterpret_cast<wchar_t const*>(g_class);
}

void master_window_impl::register_class()
{
	WNDCLASSEXW wc{};
	wc.cbSize = sizeof(wc);
	wc.style = 0;
	wc.lpfnWndProc = &class_proc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = get_instance();
	wc.hIcon = LoadIconW(nullptr, reinterpret_cast<wchar_t const*>(IDI_APPLICATION));
	wc.hCursor = LoadCursorW(nullptr, reinterpret_cast<wchar_t const*>(IDC_ARROW));
	wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = L"master_window";
	wc.hIconSm = wc.hIcon;

	assert(g_class == 0);
	g_class = RegisterClassExW(&wc);
	assert(g_class != 0);
}

void master_window_impl::unregister_class()
{
	assert(g_debug_instances == 0);

	assert(g_class != 0);
	BOOL const unregistered = UnregisterClassW(reinterpret_cast<wchar_t const*>(g_class), get_instance());
	assert(unregistered != 0);
	g_class = 0;
}

LRESULT CALLBACK master_window_impl::class_proc(HWND const hwnd, UINT const msg, WPARAM const wparam, LPARAM const lparam)
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
			master_window_impl* const self = reinterpret_cast<master_window_impl*>(self_ptr);
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

LRESULT master_window_impl::on_wm_create(HWND const& hwnd, WPARAM const& wparam, LPARAM const& lparam)
{
	master_window_impl* const self = new master_window_impl{hwnd};
	assert((SetLastError(0), true));
	LONG_PTR const prev = SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
	assert(prev == 0 && GetLastError() == 0);

	LRESULT const ret = DefWindowProcW(hwnd, WM_CREATE, wparam, lparam);
	return ret;
}

LRESULT master_window_impl::on_wm_destroy(HWND const& hwnd, WPARAM const& wparam, LPARAM const& lparam)
{
	LONG_PTR const prev = SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
	assert(prev != 0);
	master_window_impl* const self = reinterpret_cast<master_window_impl*>(prev);
	delete self;

	LRESULT const ret = DefWindowProcW(hwnd, WM_DESTROY, wparam, lparam);
	return ret;
}

LRESULT master_window_impl::on_message(UINT const& msg, WPARAM const& wparam, LPARAM const& lparam)
{
	switch(msg)
	{
		case WM_SIZE:
		{
			LRESULT const ret = on_wm_size(wparam, lparam);
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

LRESULT master_window_impl::on_wm_size(WPARAM const& wparam, LPARAM const& lparam)
{
	on_size();

	LRESULT const ret = DefWindowProcW(m_self, WM_SIZE, wparam, lparam);
	return ret;
}

void master_window_impl::on_size()
{
	RECT my_rect;
	BOOL const got_my_rect = GetClientRect(m_self, &my_rect);
	assert(got_my_rect != 0);
	assert(my_rect.left == 0 && my_rect.top == 0);

	RECT tb_rect;
	BOOL const got_tb_rect = GetClientRect(m_toolbar_window.get_hwnd(), &tb_rect);
	assert(got_tb_rect != 0);
	assert(tb_rect.left == 0 && tb_rect.top == 0);

	if(!(my_rect.bottom > tb_rect.bottom)){ return; }
	LONG const avail_height = my_rect.bottom - tb_rect.bottom;

	BOOL const invalidated_tb = RedrawWindow(m_toolbar_window.get_hwnd(), nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_FRAME); assert(invalidated_tb);
	BOOL const moved_mp = MoveWindow(m_main_panel.get_hwnd(), 0, tb_rect.bottom, my_rect.right, avail_height, TRUE); assert(moved_mp != 0);
}
