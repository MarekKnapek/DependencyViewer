#pragma once


#include <array>
#include <string>
#include <cstdint>

#include "../nogui/my_windows.h"


class main_window;
struct pe_export_address_entry;


class export_view
{
public:
	export_view() = delete;
	export_view(HWND const parent, main_window& mw);
	export_view(export_view const&) = delete;
	export_view(export_view&&) noexcept = delete;
	export_view& operator=(export_view const&) = delete;
	export_view& operator=(export_view&&) noexcept = delete;
	~export_view();
public:
	HWND get_hwnd() const;
	void on_notify(NMHDR& nmhdr);
	void on_getdispinfow(NMHDR& nmhdr);
	void refresh();
	void repaint();
	void select_item(std::uint16_t const item_idx);
private:
	wchar_t const* on_get_col_type(pe_export_address_entry const& export_entry);
	wchar_t const* on_get_col_ordinal(pe_export_address_entry const& export_entry);
	wchar_t const* on_get_col_hint(pe_export_address_entry const& export_entry);
	wchar_t const* on_get_col_name(pe_export_address_entry const& export_entry);
	wchar_t const* on_get_col_address(pe_export_address_entry const& export_entry);
	int get_type_column_max_width();
private:
	HWND const m_hwnd;
	main_window& m_main_window;
	std::array<std::wstring, 4> m_tmp_strings;
	unsigned m_tmp_string_idx;
private:
	friend class import_view;
};
