#pragma once


#include "../nogui/windows_my.h"


class toolbar_window
{
public:
	toolbar_window() noexcept;
	toolbar_window(HWND const& parent);
	toolbar_window(toolbar_window const&) = delete;
	toolbar_window(toolbar_window&& other) noexcept;
	toolbar_window& operator=(toolbar_window const&) = delete;
	toolbar_window& operator=(toolbar_window&& other) noexcept;
	~toolbar_window();
	void swap(toolbar_window& other) noexcept;
public:
	static void init();
	static void deinit();
public:
	HWND const& get_hwnd() const;
private:
	HWND m_hwnd;
};

inline void swap(toolbar_window& a, toolbar_window& b) noexcept { a.swap(b); }
