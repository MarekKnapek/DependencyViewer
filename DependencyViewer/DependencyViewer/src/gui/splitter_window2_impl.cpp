#include "splitter_window2_impl.h"

#include "main.h"

#include "../nogui/cassert_my.h"


template<> wchar_t const* const splitter_window2_impl<splitter_window2_orientation::horizontal>::s_class_name = L"splitter_window_hor";
template<> wchar_t const* const splitter_window2_impl<splitter_window2_orientation::vertical>::s_class_name = L"splitter_window_ver";
template<> wchar_t const* const splitter_window2_impl<splitter_window2_orientation::horizontal>::s_cursor_id = reinterpret_cast<wchar_t const*>(IDC_SIZENS);
template<> wchar_t const* const splitter_window2_impl<splitter_window2_orientation::vertical>::s_cursor_id = reinterpret_cast<wchar_t const*>(IDC_SIZEWE);


template<> ATOM splitter_window2_impl<splitter_window2_orientation::horizontal>::g_class;
template<> ATOM splitter_window2_impl<splitter_window2_orientation::vertical>::g_class;


template<splitter_window2_orientation orientation>
splitter_window2_impl<orientation>::splitter_window2_impl(HWND const& self) :
	m_self(self)
{
	assert(self != nullptr);
}

template<splitter_window2_orientation orientation>
splitter_window2_impl<orientation>::~splitter_window2_impl()
{
}

template<splitter_window2_orientation orientation>
void splitter_window2_impl<orientation>::init()
{
	register_class();
}

template<splitter_window2_orientation orientation>
void splitter_window2_impl<orientation>::deinit()
{
	unregister_class();
}

template<splitter_window2_orientation orientation>
wchar_t const* splitter_window2_impl<orientation>::get_class_atom()
{
	assert(g_class != 0);
	return reinterpret_cast<wchar_t const*>(g_class);
}

template<splitter_window2_orientation orientation>
void splitter_window2_impl<orientation>::register_class()
{
	WNDCLASSEXW wc{};
	wc.cbSize = sizeof(wc);
	wc.style = 0;
	wc.lpfnWndProc = &class_proc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = get_instance();
	wc.hIcon = nullptr;
	wc.hCursor = LoadCursorW(nullptr, s_cursor_id);
	wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_MENU + 1);
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = s_class_name;
	wc.hIconSm = nullptr;

	assert(g_class == 0);
	g_class = RegisterClassExW(&wc);
	assert(g_class != 0);
}

template<splitter_window2_orientation orientation>
void splitter_window2_impl<orientation>::unregister_class()
{
	assert(g_class != 0);
	BOOL const unregistered = UnregisterClassW(reinterpret_cast<wchar_t const*>(g_class), get_instance());
	assert(unregistered != 0);
	g_class = 0;
}

template<splitter_window2_orientation orientation>
LRESULT CALLBACK splitter_window2_impl<orientation>::class_proc(HWND const hwnd, UINT const msg, WPARAM const wparam, LPARAM const lparam)
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
			splitter_window2_impl* const self = reinterpret_cast<splitter_window2_impl*>(self_ptr);
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

template<splitter_window2_orientation orientation>
LRESULT splitter_window2_impl<orientation>::on_wm_create(HWND const& hwnd, WPARAM const& wparam, LPARAM const& lparam)
{
	splitter_window2_impl* const self = new splitter_window2_impl{hwnd};
	assert((SetLastError(0), true));
	LONG_PTR const prev = SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
	assert(prev == 0 && GetLastError() == 0);

	LRESULT const ret = DefWindowProcW(hwnd, WM_CREATE, wparam, lparam);
	return ret;
}

template<splitter_window2_orientation orientation>
LRESULT splitter_window2_impl<orientation>::on_wm_destroy(HWND const& hwnd, WPARAM const& wparam, LPARAM const& lparam)
{
	LONG_PTR const prev = SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
	assert(prev != 0);
	splitter_window2_impl* const self = reinterpret_cast<splitter_window2_impl*>(prev);
	delete self;

	LRESULT const ret = DefWindowProcW(hwnd, WM_DESTROY, wparam, lparam);
	return ret;
}

template<splitter_window2_orientation orientation>
LRESULT splitter_window2_impl<orientation>::on_message(UINT const& msg, WPARAM const& wparam, LPARAM const& lparam)
{
	LRESULT const ret = DefWindowProcW(m_self, msg, wparam, lparam);
	return ret;
}


template class splitter_window2_impl<splitter_window2_orientation::horizontal>;
template class splitter_window2_impl<splitter_window2_orientation::vertical>;
