#pragma once


#include <windows.h>


class main_window
{
public:
	static void register_class();
public:
	main_window();
	main_window(main_window const&) = delete;
	main_window(main_window&&) = delete;
	main_window& operator=(main_window const&) = delete;
	main_window& operator=(main_window&&) = delete;
	~main_window();
public:
	HWND get_hwnd() const;
private:
	static LRESULT CALLBACK class_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
private:
	LRESULT on_message(UINT msg, WPARAM wparam, LPARAM lparam);
	LRESULT on_wm_destroy(WPARAM wparam, LPARAM lparam);
private:
	static ATOM m_s_class;
private:
	HWND m_hwnd;
};
