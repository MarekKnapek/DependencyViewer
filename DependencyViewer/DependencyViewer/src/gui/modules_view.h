#pragma once


#include "../nogui/my_windows.h"
#include "../nogui/string_converter.h"

#include <cstdint>
#include <vector>


class main_window;


class modules_view
{
public:
	modules_view() = delete;
	modules_view(HWND const parent, main_window& mw);
	modules_view(modules_view const&) = delete;
	modules_view(modules_view&&) noexcept = delete;
	modules_view& operator=(modules_view const&) = delete;
	modules_view& operator=(modules_view&&) noexcept = delete;
	~modules_view();
public:
	HWND const& get_hwnd() const;
	void on_notify(NMHDR& nmhdr);
	void refresh();
	void repaint();
private:
	void on_getdispinfow(NMHDR& nmhdr);
	void on_columnclick(NMHDR& nmhdr);
	wstring on_get_col_name(std::uint32_t const& row);
	wstring on_get_col_path(std::uint32_t const& row);
	wstring on_get_col_name_unsorted(std::uint32_t const& row);
	wstring on_get_col_path_unsorted(std::uint32_t const& row);
	void refresh_headers();
	void sort_view();
private:
	HWND const m_hwnd;
	main_window& m_main_window;
	string_converter m_string_converter;
	std::uint8_t m_sort_direction;
	std::vector<std::uint32_t> m_sort;
};
