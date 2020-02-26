#pragma once


#include <cstdint>

#include "../nogui/windows_my.h"


struct file_info;


class export_window
{
public:
	enum class wm : std::uint32_t
	{
		wm_repaint = WM_USER + 1,
		wm_setfi,
	};
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
	void repaint();
	void setfi(file_info const* const& fi);
public:
	HWND const& get_hwnd() const;
private:
	HWND m_hwnd;
};

inline void swap(export_window& a, export_window& b) noexcept { a.swap(b); }
