#pragma once


#include "../nogui/windows_my.h"


class tree_window
{
public:
	tree_window() noexcept;
	tree_window(HWND const& parent);
	tree_window(tree_window const&) = delete;
	tree_window(tree_window&& other) noexcept;
	tree_window& operator=(tree_window const&) = delete;
	tree_window& operator=(tree_window&& other) noexcept;
	~tree_window();
	void swap(tree_window& other) noexcept;
public:
	static void init();
	static void deinit();
public:
	HWND const& get_hwnd() const;
private:
	HWND m_hwnd;
};

inline void swap(tree_window& a, tree_window& b) noexcept { a.swap(b); }
