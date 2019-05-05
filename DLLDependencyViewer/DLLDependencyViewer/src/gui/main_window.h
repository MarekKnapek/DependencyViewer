#pragma once


class main_window
{
public:
	main_window() noexcept;
	main_window(main_window const&) = delete;
	main_window(main_window&& other) noexcept;
	main_window& operator=(main_window const&) = delete;
	main_window& operator=(main_window&& other) noexcept;
	~main_window() noexcept;
	void swap(main_window& other) noexcept;
};

inline void swap(main_window& a, main_window& b) noexcept { a.swap(b); }
