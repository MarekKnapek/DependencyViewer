#pragma once


#include "../nogui/my_string_handle.h"

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
		wm_selectitem,
		wm_iscmdpropertiesavail,
		wm_setonitemchanged,
		wm_setcmdmatching,
		wm_setcmdproperties,
	};
	using onitemchanged_ctx_t = void*;
	using onitemchanged_fn_t = void(*)(onitemchanged_ctx_t const&, file_info const* const&);
	using cmd_matching_ctx_t = void*;
	using cmd_matching_fn_t = void(*)(cmd_matching_ctx_t const&, file_info const* const&);
	using cmd_properties_ctx_t = void*;
	using cmd_properties_fn_t = void(*)(cmd_properties_ctx_t const&, wstring_handle const&);
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
	void selectitem(file_info const* const& fi);
	bool iscmdpropertiesavail(wstring_handle* const& out_file_path);
	void setonitemchanged(onitemchanged_fn_t const& onitemchanged_fn, onitemchanged_ctx_t const& onitemchanged_ctx);
	void setcmdmatching(cmd_matching_fn_t const& cmd_matching_fn, cmd_matching_ctx_t const& cmd_matching_ctx);
	void setcmdproperties(cmd_properties_fn_t const& cmd_properties_fn, cmd_properties_ctx_t const& cmd_properties_ctx);
public:
	HWND const& get_hwnd() const;
private:
	HWND m_hwnd;
};

inline void swap(modules_window& a, modules_window& b) noexcept { a.swap(b); }
