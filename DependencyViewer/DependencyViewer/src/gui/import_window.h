#pragma once


#include <cstdint>

#include "../nogui/windows_my.h"


struct file_info;


class import_window
{
public:
	enum class wm : std::uint32_t
	{
		wm_repaint = WM_USER + 1,
		wm_translateaccelerator,
		wm_setfi,
		wm_setundecorate,
		wm_selectitem,
		wm_setcmdmatching,
	};
	using cmd_matching_ctx_t = void*;
	using cmd_matching_fn_t = void(*)(cmd_matching_ctx_t const, std::uint16_t const);
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
	void repaint();
	bool translateaccelerator(MSG& message);
	void setfi(file_info const* const& fi);
	void setundecorate(bool const& undecorate);
	void selectitem(std::uint16_t const& item_idx);
	void setcmdmatching(cmd_matching_fn_t const& cmd_matching_fn, cmd_matching_ctx_t const& cmd_matching_ctx);
public:
	HWND const& get_hwnd() const;
private:
	HWND m_hwnd;
};

inline void swap(import_window& a, import_window& b) noexcept { a.swap(b); }
