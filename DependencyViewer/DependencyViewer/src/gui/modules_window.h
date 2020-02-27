#pragma once


#include <cstdint>

#include "../nogui/windows_my.h"


class modules_window
{
public:
	enum class wm : std::uint32_t
	{
		wm_repaint = WM_USER + 1,
	};
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
	void repaint();
public:
	HWND const& get_hwnd() const;
private:
	HWND m_hwnd;
};

inline void swap(modules_window& a, modules_window& b) noexcept { a.swap(b); }
