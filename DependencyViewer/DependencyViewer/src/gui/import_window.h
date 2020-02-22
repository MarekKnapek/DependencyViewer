#pragma once


#include <cstdint>

#include "../nogui/windows_my.h"


struct file_info;


class import_window
{
public:
	enum class wm : std::uint32_t
	{
		wm_setfi = WM_USER + 1,
	};
public:
	import_window() noexcept;
	import_window(HWND const& parent);
	import_window(import_window const&) = delete;
	import_window(import_window&& other) noexcept;
	import_window& operator=(import_window const&) = delete;
	import_window& operator=(import_window&& other) noexcept;
	~import_window();
	void swap(import_window& other) noexcept;
public:
	static void init();
	static void deinit();
public:
	void setfi(file_info const* const& fi);
public:
	HWND const& get_hwnd() const;
private:
	HWND m_hwnd;
};

inline void swap(import_window& a, import_window& b) noexcept { a.swap(b); }
