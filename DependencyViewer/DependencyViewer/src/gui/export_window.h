#pragma once


#include "../nogui/windows_my.h"


class export_window
{
public:
	export_window() noexcept;
	export_window(HWND const& parent);
	export_window(export_window const&) = delete;
	export_window(export_window&& other) noexcept;
	export_window& operator=(export_window const&) = delete;
	export_window& operator=(export_window&& other) noexcept;
	~export_window();
	void swap(export_window& other) noexcept;
public:
	static void init();
	static void deinit();
public:
	HWND const& get_hwnd() const;
private:
	HWND m_hwnd;
};

inline void swap(export_window& a, export_window& b) noexcept { a.swap(b); }
