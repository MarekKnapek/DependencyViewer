#pragma once


#include <cstddef>

#include "../nogui/windows_my.h"


class master_window
{
public:
	master_window() noexcept;
	master_window(std::nullptr_t const&);
	master_window(master_window const&) = delete;
	master_window(master_window&& other) noexcept;
	master_window& operator=(master_window const&) = delete;
	master_window& operator=(master_window&& other) noexcept;
	~master_window();
	void swap(master_window& other) noexcept;
public:
	static void init();
	static void deinit();
public:
	HWND const& get_hwnd() const;
private:
	HWND m_hwnd;
};

inline void swap(master_window& a, master_window& b) noexcept { a.swap(b); }
