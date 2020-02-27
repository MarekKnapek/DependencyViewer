#pragma once


#include <cstdint>

#include "../nogui/windows_my.h"


struct file_info;
struct modules_list_t;


class modules_window
{
public:
	enum class wm : std::uint32_t
	{
		wm_repaint = WM_USER + 1,
		wm_translateaccelerator,
		wm_setmodlist,
		wm_setcmdmatching,
	};
	using cmd_matching_ctx_t = void*;
	using cmd_matching_fn_t = void(*)(cmd_matching_ctx_t const, file_info const* const);
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
	bool translateaccelerator(MSG& message);
	void setmodlist(modules_list_t const& ml);
	void setcmdmatching(cmd_matching_fn_t const& cmd_matching_fn, cmd_matching_ctx_t const& cmd_matching_ctx);
public:
	HWND const& get_hwnd() const;
private:
	HWND m_hwnd;
};

inline void swap(modules_window& a, modules_window& b) noexcept { a.swap(b); }
