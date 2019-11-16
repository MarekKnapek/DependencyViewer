#pragma once


class list_view_base
{
public:
	list_view_base();
	list_view_base(list_view_base const&) = delete;
	list_view_base(list_view_base&&) noexcept = delete;
	list_view_base& operator=(list_view_base const&) = delete;
	list_view_base& operator=(list_view_base&&) noexcept = delete;
	~list_view_base();
protected:
	int on_columnclick(void const* const param, int const n_headers, int const curr_sort);
	void refresh_headers(void const* const handle, int const n_headers, int const curr_sort);
};
