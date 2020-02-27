#pragma once


#include "../nogui/windows_my.h"


class modules_window
{
public:
	modules_window() noexcept;
	modules_window(HWND const& parent);
	modules_window(modules_window const&) = delete;
	modules_window(modules_window&& other) noexcept;
	modules_window& operator=(modules_window const&) = delete;
	modules_window& operator=(modules_window&& other) noexcept;
	~modules_window();
	void swap(modules_window& other) noexcept;
public:
	static void init();
	static void deinit();
public:
	HWND const& get_hwnd() const;
private:
	HWND m_hwnd;
};

inline void swap(modules_window& a, modules_window& b) noexcept { a.swap(b); }
