#pragma once


#include <cstdint>

#include "../nogui/windows_my.h"


class toolbar_window
{
public:
	enum class wm : std::uint32_t
	{
		wm_setcmdopen = WM_USER + 1,
		wm_setcmdfullpaths,
		wm_setcmdundecorate,
		wm_setcmdproperties,
	};
	using cmd_open_ctx_t = void*;
	using cmd_open_fn_t = void(*)(cmd_open_ctx_t const&);
	using cmd_fullpaths_ctx_t = void*;
	using cmd_fullpaths_fn_t = void(*)(cmd_fullpaths_ctx_t const&);
	using cmd_undecorate_ctx_t = void*;
	using cmd_undecorate_fn_t = void(*)(cmd_undecorate_ctx_t const&);
	using cmd_properties_ctx_t = void*;
	using cmd_properties_fn_t = void(*)(cmd_properties_ctx_t const&);
public:
	toolbar_window() noexcept;
	toolbar_window(HWND const& parent);
	toolbar_window(toolbar_window const&) = delete;
	toolbar_window(toolbar_window&& other) noexcept;
	toolbar_window& operator=(toolbar_window const&) = delete;
	toolbar_window& operator=(toolbar_window&& other) noexcept;
	~toolbar_window();
	void swap(toolbar_window& other) noexcept;
public:
	static void init();
	static void deinit();
public:
	void setcmdopen(cmd_open_fn_t const& cmd_open_fn, cmd_open_ctx_t const& cmd_open_ctx);
	void setcmdfullpaths(cmd_fullpaths_fn_t const& cmd_fullpaths_fn, cmd_fullpaths_ctx_t const& cmd_fullpaths_ctx);
	void setcmdundecorate(cmd_undecorate_fn_t const& cmd_undecorate_fn, cmd_undecorate_ctx_t const& cmd_undecorate_ctx);
	void setcmdproperties(cmd_properties_fn_t const& cmd_properties_fn, cmd_properties_ctx_t const& cmd_properties_ctx);
public:
	HWND const& get_hwnd() const;
private:
	HWND m_hwnd;
};

inline void swap(toolbar_window& a, toolbar_window& b) noexcept { a.swap(b); }
