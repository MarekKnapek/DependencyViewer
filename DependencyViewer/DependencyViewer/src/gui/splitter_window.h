#pragma once


#include <cstdint>

#include "../nogui/windows_my.h"


enum class splitter_window_orientation
{
	horizontal,
	vertical,
};


template<splitter_window_orientation orientation>
class splitter_window
{
public:
	enum class wm : std::uint32_t
	{
		wm_setchildren = WM_USER + 1,
		wm_setposition,
	};
public:
	splitter_window() noexcept;
	splitter_window(HWND const& parent);
	splitter_window(splitter_window const&) = delete;
	splitter_window(splitter_window&& other) noexcept;
	splitter_window& operator=(splitter_window const&) = delete;
	splitter_window& operator=(splitter_window&& other) noexcept;
	~splitter_window();
	void swap(splitter_window& other) noexcept;
public:
	static void init();
	static void deinit();
public:
	void setchildren(HWND const& child_a, HWND const& child_b);
	void setposition(float const& position);
public:
	HWND const& get_hwnd() const;
private:
	HWND m_hwnd;
};

template<splitter_window_orientation orientation> inline void swap(splitter_window<orientation>& a, splitter_window<orientation>& b) noexcept { a.swap(b); }

typedef splitter_window<splitter_window_orientation::horizontal> splitter_window_hor;
typedef splitter_window<splitter_window_orientation::vertical> splitter_window_ver;
