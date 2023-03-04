#include "dark_mode.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "../nogui/my_windows.h"

#include <uxtheme.h>


enum set_preferred_app_mode_kind_e
{
	set_preferred_app_mode_kind_e_default /* allow_dark_mode_for_app false */,
	set_preferred_app_mode_kind_e_allow_dark /* allow_dark_mode_for_app true */,
	set_preferred_app_mode_kind_e_force_dark,
	set_preferred_app_mode_kind_e_force_light
};
typedef enum set_preferred_app_mode_kind_e set_preferred_app_mode_kind_t;

enum window_composition_attrib_e
{
	window_composition_attrib_e_undefined,
	window_composition_attrib_e_ncrendering_enabled,
	window_composition_attrib_e_ncrendering_policy,
	window_composition_attrib_e_transitions_forcedisabled,
	window_composition_attrib_e_allow_ncpaint,
	window_composition_attrib_e_caption_button_bounds,
	window_composition_attrib_e_nonclient_rtl_layout,
	window_composition_attrib_e_force_iconic_representation,
	window_composition_attrib_e_extended_frame_bounds,
	window_composition_attrib_e_has_iconic_bitmap,
	window_composition_attrib_e_theme_attributes,
	window_composition_attrib_e_ncrendering_exiled,
	window_composition_attrib_e_ncadornmentinfo,
	window_composition_attrib_e_excluded_from_livepreview,
	window_composition_attrib_e_video_overlay_active,
	window_composition_attrib_e_force_activewindow_appearance,
	window_composition_attrib_e_disallow_peek,
	window_composition_attrib_e_cloak,
	window_composition_attrib_e_cloaked,
	window_composition_attrib_e_accent_policy,
	window_composition_attrib_e_freeze_representation,
	window_composition_attrib_e_ever_uncloaked,
	window_composition_attrib_e_visual_owner,
	window_composition_attrib_e_holographic,
	window_composition_attrib_e_excluded_from_dda,
	window_composition_attrib_e_passive_update_mode,
	window_composition_attrib_e_use_dark_mode_colors
};
typedef enum window_composition_attrib_e window_composition_attrib_t;

struct window_composition_attrib_data_s
{
	DWORD m_attrib;
	PVOID m_ptr;
	SIZE_T m_size;
};
typedef struct window_composition_attrib_data_s window_composition_attrib_data_t;

static wchar_t const* const s_set_prop_w_kind_use_immersive_dark_mode_colors = L"UseImmersiveDarkModeColors";

static wchar_t const* const s_set_window_theme_sub_app_name_kind_item_view = L"ItemsView";

static wchar_t const* const s_open_theme_data_kind_items_view = L"ItemsView";
static wchar_t const* const s_open_theme_data_kind_header = L"Header";

enum part_id_header_e
{
	part_id_header_e_item = 1,
	part_id_header_e_item_left,
	part_id_header_e_item_right,
	part_id_header_e_sort_arrow,
	part_id_header_e_drop_down,
	part_id_header_e_drop_down_filter,
	part_id_header_e_over_flow
};
typedef enum part_id_header_e part_id_header_t;

enum theme_property_id_e
{
	theme_property_id_e_fill_color = 3802,
	theme_property_id_e_text_color = 3803
};
typedef enum theme_property_id_e theme_property_id_t;

enum dwm_window_attribute_e
{
	dwm_window_attribute_e_ncrendering_enabled = 1,
	dwm_window_attribute_e_ncrendering_policy,
	dwm_window_attribute_e_transitions_forcedisabled,
	dwm_window_attribute_e_allow_ncpaint,
	dwm_window_attribute_e_caption_button_bounds,
	dwm_window_attribute_e_nonclient_rtl_layout,
	dwm_window_attribute_e_force_iconic_representation,
	dwm_window_attribute_e_flip3d_policy,
	dwm_window_attribute_e_extended_frame_bounds,
	dwm_window_attribute_e_has_iconic_bitmap,
	dwm_window_attribute_e_disallow_peek,
	dwm_window_attribute_e_excluded_from_peek,
	dwm_window_attribute_e_cloak,
	dwm_window_attribute_e_cloaked,
	dwm_window_attribute_e_freeze_representation,
	dwm_window_attribute_e_passive_update_mode,
	dwm_window_attribute_e_use_hostbackdropbrush,
	dwm_window_attribute_e_use_immersive_dark_mode = 20,
	dwm_window_attribute_e_window_corner_preference = 33,
	dwm_window_attribute_e_border_color,
	dwm_window_attribute_e_caption_color,
	dwm_window_attribute_e_text_color,
	dwm_window_attribute_e_visible_frame_border_thickness,
	dwm_window_attribute_e_systembackdrop_type
};
typedef enum dwm_window_attribute_e dwm_window_attribute_t;

typedef HMODULE(__stdcall*load_library_ex_w_t)(LPCWSTR, HANDLE, DWORD);
typedef BOOL(__stdcall*system_parameters_info_w_t)(UINT, UINT, PVOID, UINT);
typedef BOOL(__stdcall*set_window_composition_attribute_t)(HWND, window_composition_attrib_data_t*);
typedef BOOL(__stdcall*set_prop_w_t)(HWND, LPCWSTR, HANDLE);
typedef HRESULT(__stdcall*dwm_set_window_attribute_t)(HWND, DWORD, LPCVOID, DWORD);
typedef HRESULT(__stdcall*set_window_theme_t)(HWND, LPCWSTR, LPCWSTR);
typedef HTHEME(__stdcall*open_theme_data_t)(HWND, LPCWSTR);
typedef HRESULT(__stdcall*close_theme_data_t)(HTHEME);
typedef HRESULT(__stdcall*get_theme_color_t)(HTHEME, int, int, int, COLORREF*);
typedef void(__stdcall*refresh_immersive_color_policy_state_t)(void);
typedef BOOL(__stdcall*should_apps_use_dark_mode_t)(void);
typedef BOOL(__stdcall*allow_dark_mode_for_window_t)(HWND, BOOL);
typedef void(__stdcall*allow_dark_mode_for_app_t)(BOOL);
typedef void(__stdcall*set_preferred_app_mode_t)(DWORD);
typedef BOOL(__stdcall*set_window_sub_class_t)(HWND, SUBCLASSPROC, UINT_PTR, DWORD_PTR);
typedef LRESULT(__stdcall*def_sub_class_proc_t)(HWND, UINT, WPARAM, LPARAM);

static wchar_t const* const s_kernel32_name = L"kernel32.dll";
static wchar_t const* const s_user32_name = L"user32.dll";
static wchar_t const* const s_dwmapi_name = L"dwmapi.dll";
static wchar_t const* const s_uxtheme_name = L"uxtheme.dll";
static wchar_t const* const s_comctl32_name = L"comctl32.dll";
static char const* const s_load_library_ex_w_name = "LoadLibraryExW";
static char const* const s_system_parameters_info_w_name = "SystemParametersInfoW";
static char const* const s_set_window_composition_attribute_name = "SetWindowCompositionAttribute";
static char const* const s_set_prop_w_name = "SetPropW";
static char const* const s_dwm_set_window_attribute_name = "DwmSetWindowAttribute";
static char const* const s_set_window_theme_name = "SetWindowTheme";
static char const* const s_open_theme_data_name = "OpenThemeData";
static char const* const s_close_theme_data_name = "CloseThemeData";
static char const* const s_get_theme_color_name = "GetThemeColor";
static char const* const s_refresh_immersive_color_policy_state_name = ((char const*)(((uintptr_t)(((unsigned short)(((unsigned short)(104)) & ((unsigned short)(0xffffu))))))));
static char const* const s_should_apps_use_dark_mode_name = ((char const*)(((uintptr_t)(((unsigned short)(((unsigned short)(132)) & ((unsigned short)(0xffffu))))))));
static char const* const s_allow_dark_mode_for_app_name = ((char const*)(((uintptr_t)(((unsigned short)(((unsigned short)(135)) & ((unsigned short)(0xffffu))))))));
static char const* const s_allow_dark_mode_for_window_name = ((char const*)(((uintptr_t)(((unsigned short)(((unsigned short)(133)) & ((unsigned short)(0xffffu))))))));
static char const* const s_set_preferred_app_mode_name = ((char const*)(((uintptr_t)(((unsigned short)(((unsigned short)(135)) & ((unsigned short)(0xffffu))))))));
static char const* const s_set_window_sub_class_name = "SetWindowSubclass";
static char const* const s_def_sub_class_proc_name = "DefSubclassProc";

BOOL g_dark_mode_available;
BOOL g_dark_mode_active;
COLORREF g_color_list_view_header_text;

HMODULE g_kernel;
load_library_ex_w_t g_load_library_ex_w;
HMODULE g_user;
system_parameters_info_w_t g_system_parameters_info_w;
set_window_composition_attribute_t g_set_window_composition_attribute;
set_prop_w_t g_set_prop_w;
HMODULE g_dwmapi;
dwm_set_window_attribute_t g_dwm_set_window_attribute;
HMODULE g_uxtheme;
set_window_theme_t g_set_window_theme;
open_theme_data_t g_open_theme_data;
close_theme_data_t g_close_theme_data;
get_theme_color_t g_get_theme_color;
set_preferred_app_mode_t g_set_preferred_app_mode;
refresh_immersive_color_policy_state_t g_refresh_immersive_color_policy_state;
should_apps_use_dark_mode_t g_should_apps_use_dark_mode;
allow_dark_mode_for_window_t g_allow_dark_mode_for_window;
HMODULE g_comctl;
set_window_sub_class_t g_set_window_sub_class;
def_sub_class_proc_t g_def_sub_class_proc;


LRESULT __stdcall dark_mode_ctrl_list_view_proc(HWND const list_view, UINT const msg, WPARAM const wparam, LPARAM const lparam, UINT_PTR const id, DWORD_PTR const data)
{
	LPNMHDR nmhdr;
	LPNMCUSTOMDRAW custom_draw;
	COLORREF prev_color;
	HWND header;
	BOOL b;
	HTHEME theme;
	HRESULT hr;
	COLORREF color;
	LRESULT lr;

	assert(g_dark_mode_available);
	assert(list_view);
	((void)(id));
	((void)(data));

	switch(msg)
	{
		case WM_NOTIFY:
		{
			nmhdr = ((LPNMHDR)(lparam));
			switch(nmhdr->code)
			{
				case NM_CUSTOMDRAW:
				{
					custom_draw = ((LPNMCUSTOMDRAW)(nmhdr));
					switch(custom_draw->dwDrawStage)
					{
						case CDDS_PREPAINT:
						{
							return CDRF_NOTIFYITEMDRAW;
						}
						break;
						case CDDS_ITEMPREPAINT:
						{
							prev_color = SetTextColor(custom_draw->hdc, g_color_list_view_header_text); assert(prev_color != CLR_INVALID);
							return CDRF_DODEFAULT;
						}
						break;
					}
				}
				break;
			}
		}
		break;
		case WM_THEMECHANGED:
		{
			header = ((HWND)(SendMessageW(list_view, LVM_GETHEADER, 0, 0))); assert(header);
			b = g_allow_dark_mode_for_window(list_view, g_dark_mode_active); ((void)(b));
			b = g_allow_dark_mode_for_window(header, g_dark_mode_active); ((void)(b));
			theme = g_open_theme_data(NULL, s_open_theme_data_kind_items_view);
			if(theme)
			{
				hr = g_get_theme_color(theme, 0, 0, theme_property_id_e_text_color, &color); assert(hr == S_OK);
				lr = SendMessageW(list_view, LVM_SETTEXTCOLOR, 0, ((LPARAM)(color))); assert(lr == TRUE);
				hr = g_get_theme_color(theme, 0, 0, theme_property_id_e_fill_color, &color); assert(hr == S_OK);
				lr = SendMessageW(list_view, LVM_SETTEXTBKCOLOR, 0, ((LPARAM)(color))); assert(lr == TRUE);
				lr = SendMessageW(list_view, LVM_SETBKCOLOR, 0, ((LPARAM)(color))); assert(lr == TRUE);
				hr = g_close_theme_data(theme); assert(hr == S_OK);
			}
			theme = g_open_theme_data(header, s_open_theme_data_kind_header);
			if(theme)
			{
				hr = g_get_theme_color(theme, part_id_header_e_item, 0, theme_property_id_e_text_color, &g_color_list_view_header_text); assert(hr == S_OK);
				hr = g_close_theme_data(theme); assert(hr == S_OK);
			}
			lr = SendMessageW(header, WM_THEMECHANGED, wparam, lparam); ((void)(lr));
			b = RedrawWindow(list_view, NULL, NULL, RDW_FRAME | RDW_INVALIDATE); assert(b != 0);
		}
		break;
	}
	return g_def_sub_class_proc(list_view, msg, wparam, lparam);
}

LRESULT __stdcall dark_mode_ctrl_tree_view_proc(HWND const tree_view, UINT const msg, WPARAM const wparam, LPARAM const lparam, UINT_PTR const id, DWORD_PTR const data)
{
	BOOL b;
	HTHEME theme;
	HRESULT hr;
	COLORREF color;
	LRESULT lr;

	assert(g_dark_mode_available);
	assert(tree_view);
	((void)(id));
	((void)(data));

	switch(msg)
	{
		case WM_THEMECHANGED:
		{
			b = g_allow_dark_mode_for_window(tree_view, g_dark_mode_active); ((void)(b));
			theme = g_open_theme_data(NULL, s_open_theme_data_kind_items_view);
			if(theme)
			{
				hr = g_get_theme_color(theme, 0, 0, theme_property_id_e_text_color, &color); assert(hr == S_OK);
				lr = SendMessageW(tree_view, TVM_SETTEXTCOLOR, 0, ((LPARAM)(color))); ((void)(lr));
				hr = g_get_theme_color(theme, 0, 0, theme_property_id_e_fill_color, &color); assert(hr == S_OK);
				lr = SendMessageW(tree_view, TVM_SETBKCOLOR, 0, ((LPARAM)(color))); ((void)(lr));
				hr = g_close_theme_data(theme); assert(hr == S_OK);
			}
			b = RedrawWindow(tree_view, NULL, NULL, RDW_FRAME | RDW_INVALIDATE); assert(b != 0);
		}
		break;
	}
	return g_def_sub_class_proc(tree_view, msg, wparam, lparam);
}


void dark_mode_init(void)
{
	g_dark_mode_available = FALSE;
	g_dark_mode_active = FALSE;

	g_kernel = GetModuleHandleW(s_kernel32_name); assert(g_kernel);
	g_load_library_ex_w = ((load_library_ex_w_t)(GetProcAddress(g_kernel, s_load_library_ex_w_name))); if(!g_load_library_ex_w) return;
	g_user = GetModuleHandleW(s_user32_name); assert(g_user);
	g_system_parameters_info_w = ((system_parameters_info_w_t)(GetProcAddress(g_user, s_system_parameters_info_w_name))); if(!g_system_parameters_info_w) return;
	g_set_window_composition_attribute = ((set_window_composition_attribute_t)(GetProcAddress(g_user, s_set_window_composition_attribute_name))); if(!g_set_window_composition_attribute) return;
	g_set_prop_w = ((set_prop_w_t)(GetProcAddress(g_user, s_set_prop_w_name))); if(!g_set_prop_w) return;
	g_uxtheme = g_load_library_ex_w(s_uxtheme_name, NULL, LOAD_LIBRARY_SEARCH_SYSTEM32); if(!g_uxtheme) return;
	g_set_window_theme = ((set_window_theme_t)(GetProcAddress(g_uxtheme, s_set_window_theme_name))); if(!g_set_window_theme) return;
	g_open_theme_data = ((open_theme_data_t)(GetProcAddress(g_uxtheme, s_open_theme_data_name))); if(!g_open_theme_data) return;
	g_close_theme_data = ((close_theme_data_t)(GetProcAddress(g_uxtheme, s_close_theme_data_name))); if(!g_close_theme_data) return;
	g_get_theme_color = ((get_theme_color_t)(GetProcAddress(g_uxtheme, s_get_theme_color_name))); if(!g_get_theme_color) return;
	g_set_preferred_app_mode = ((set_preferred_app_mode_t)(GetProcAddress(g_uxtheme, s_set_preferred_app_mode_name))); if(!g_set_preferred_app_mode) return;
	g_refresh_immersive_color_policy_state = ((refresh_immersive_color_policy_state_t)(GetProcAddress(g_uxtheme, s_refresh_immersive_color_policy_state_name))); if(!g_refresh_immersive_color_policy_state) return;
	g_should_apps_use_dark_mode = ((should_apps_use_dark_mode_t)(GetProcAddress(g_uxtheme, s_should_apps_use_dark_mode_name))); if(!g_should_apps_use_dark_mode) return;
	g_allow_dark_mode_for_window = ((allow_dark_mode_for_window_t)(GetProcAddress(g_uxtheme, s_allow_dark_mode_for_window_name))); if(!g_allow_dark_mode_for_window) return;
	g_dwmapi = g_load_library_ex_w(s_dwmapi_name, NULL, LOAD_LIBRARY_SEARCH_SYSTEM32); if(!g_dwmapi) return;
	g_dwm_set_window_attribute = ((dwm_set_window_attribute_t)(GetProcAddress(g_dwmapi, s_dwm_set_window_attribute_name))); if(!g_dwm_set_window_attribute) return;
	g_comctl = GetModuleHandleW(s_comctl32_name); assert(g_comctl);
	g_set_window_sub_class = ((set_window_sub_class_t)(GetProcAddress(g_comctl, s_set_window_sub_class_name))); if(!g_set_window_sub_class) return;
	g_def_sub_class_proc = ((def_sub_class_proc_t)(GetProcAddress(g_comctl, s_def_sub_class_proc_name))); if(!g_def_sub_class_proc) return;

	g_dark_mode_available = TRUE;	

	dark_mode_refresh();
}

void dark_mode_reinit(void)
{
	HIGHCONTRASTW high_contrast;

	if(!g_dark_mode_available) return;

	g_dark_mode_active = FALSE;

	if(!g_should_apps_use_dark_mode()) return;
	memset(&high_contrast, 0, sizeof(high_contrast)); high_contrast.cbSize = sizeof(high_contrast); if(!g_system_parameters_info_w(SPI_GETHIGHCONTRAST, sizeof(high_contrast), &high_contrast, FALSE)) return; if(high_contrast.dwFlags & HCF_HIGHCONTRASTON) return;

	g_dark_mode_active = TRUE;
}

void dark_mode_refresh(void)
{
	if(!g_dark_mode_available) return;

	g_set_preferred_app_mode(set_preferred_app_mode_kind_e_allow_dark);
	g_refresh_immersive_color_policy_state();

	dark_mode_reinit();

	/* todo fix scroll bars */
}

void dark_mode_window(HWND const hwnd)
{
	BOOL b;
	window_composition_attrib_data_t data;
	HRESULT hr;

	if(!g_dark_mode_available) return;

	b = g_set_prop_w(hwnd, s_set_prop_w_kind_use_immersive_dark_mode_colors, ((HANDLE)(((uintptr_t)(g_dark_mode_active))))); assert(b != 0);
	memset(&data, 0, sizeof(data)); data.m_attrib = window_composition_attrib_e_use_dark_mode_colors; data.m_ptr = &g_dark_mode_active; data.m_size = sizeof(g_dark_mode_active); b = g_set_window_composition_attribute(hwnd, &data); assert(b != 0);
	b = g_allow_dark_mode_for_window(hwnd, g_dark_mode_active); ((void)(b));
	hr = g_dwm_set_window_attribute(hwnd, dwm_window_attribute_e_use_immersive_dark_mode, &g_dark_mode_active, sizeof(g_dark_mode_active)); assert(hr == S_OK);
}

void dark_mode_ctrl_list_view(HWND const list_view)
{
	BOOL b;
	HWND header;
	HRESULT hr;

	assert(list_view);

	if(!g_dark_mode_available) return;

	b = g_set_window_sub_class(list_view, &dark_mode_ctrl_list_view_proc, 0, 0); assert(b == TRUE);
	header = ((HWND)(SendMessageW(list_view, LVM_GETHEADER, 0, 0))); assert(header);
	hr = g_set_window_theme(header, s_set_window_theme_sub_app_name_kind_item_view, 0); assert(hr == S_OK);
	hr = g_set_window_theme(list_view, s_set_window_theme_sub_app_name_kind_item_view, 0); assert(hr == S_OK);
}

void dark_mode_ctrl_tree_view(HWND const tree_view)
{
	BOOL b;
	HRESULT hr;

	assert(tree_view);

	if(!g_dark_mode_available) return;

	b = g_set_window_sub_class(tree_view, &dark_mode_ctrl_tree_view_proc, 0, 0); assert(b == TRUE);
	hr = g_set_window_theme(tree_view, s_set_window_theme_sub_app_name_kind_item_view, 0); assert(hr == S_OK);
}

void dark_mode_deinit(void)
{
}
