#pragma once


#include <cstdint>

#include "../nogui/windows_my.h"


enum class splitter_window2_orientation
{
	horizontal,
	vertical,
};


template<splitter_window2_orientation orientation>
class splitter_window2
{
public:
	enum class wm : std::uint32_t
	{
		wm_setchildren = WM_USER + 1,
	};
public:
	splitter_window2() noexcept;
	splitter_window2(HWND const& parent);
	splitter_window2(splitter_window2 const&) = delete;
	splitter_window2(splitter_window2&& other) noexcept;
	splitter_window2& operator=(splitter_window2 const&) = delete;
	splitter_window2& operator=(splitter_window2&& other) noexcept;
	~splitter_window2();
	void swap(splitter_window2& other) noexcept;
public:
	static void init();
	static void deinit();
public:
	void setchildren(HWND const& child_a, HWND const& child_b);
public:
	HWND const& get_hwnd() const;
private:
	HWND m_hwnd;
};

template<splitter_window2_orientation orientation> inline void swap(splitter_window2<orientation>& a, splitter_window2<orientation>& b) noexcept { a.swap(b); }

typedef splitter_window2<splitter_window2_orientation::horizontal> splitter_window2_hor;
typedef splitter_window2<splitter_window2_orientation::vertical> splitter_window2_ver;
