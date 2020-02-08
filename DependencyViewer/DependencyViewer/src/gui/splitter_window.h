#pragma once


#include "../nogui/my_windows.h"


enum class splitter_window_orientation
{
	horizontal,
	vertical,
};


template<splitter_window_orientation orientation>
class splitter_window
{
public:
	static void register_class();
public:
	splitter_window(HWND const& parent);
	splitter_window(splitter_window const&) = delete;
	splitter_window(splitter_window&&) = delete;
	splitter_window& operator=(splitter_window const&) = delete;
	splitter_window& operator=(splitter_window&&) = delete;
	~splitter_window();
public:
	HWND const& get_hwnd() const;
	void set_elements(HWND const& first, HWND const& second);
	void set_position(float const& position);
private:
	static LRESULT CALLBACK class_proc(HWND const hwnd, UINT const msg, WPARAM const wparam, LPARAM const lparam);
	LRESULT on_message(UINT const& msg, WPARAM const& wparam, LPARAM const&lparam);
	LRESULT on_wm_destroy(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_size(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_notify(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_contextmenu(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_mousemove(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_lbuttondown(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_lbuttonup(WPARAM const& wparam, LPARAM const& lparam);
	void refresh_children();
private:
	static wchar_t const* const s_window_class_name;
	static wchar_t const* const s_cursor_id;
	static ATOM g_class;
private:
	HWND m_hwnd;
	HWND m_first;
	HWND m_second;
	float m_position;
	bool m_sizing;
};

typedef splitter_window<splitter_window_orientation::horizontal> splitter_window_hor;
typedef splitter_window<splitter_window_orientation::vertical> splitter_window_ver;
