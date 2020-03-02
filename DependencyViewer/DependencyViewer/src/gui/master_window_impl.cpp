#include "master_window_impl.h"

#include "com_dlg.h"
#include "main.h"

#include <algorithm>

#include "../nogui/cassert_my.h"
#include "../nogui/scope_exit.h"

#include "../nogui/windows_my.h"

#include <commdlg.h>


static constexpr wchar_t const s_master_window_open_filter[] = L"Executable files and libraries (*.exe;*.dll;*.ocx)\0*.exe;*.dll;*.ocx\0All files\0*.*\0";
static constexpr wchar_t const s_master_window_error_title[] = L"DependencyViewer error.";
static constexpr wchar_t const s_master_window_error_msg[] = L"Failed to process all files.";


ATOM master_window_impl::g_class;
int master_window_impl::g_debug_instances;


master_window_impl::master_window_impl(HWND const& self) :
	m_self(self),
	m_toolbar_window(self),
	m_main_panel(self),
	m_upper_panel(m_main_panel.get_hwnd()),
	m_tree_window(m_upper_panel.get_hwnd()),
	m_right_panel(m_upper_panel.get_hwnd()),
	m_import_window(m_right_panel.get_hwnd()),
	m_export_window(m_right_panel.get_hwnd()),
	m_modules_window(m_main_panel.get_hwnd()),
	m_mo()
{
	assert(self != nullptr);

	++g_debug_instances;

	m_main_panel.setchildren(m_upper_panel.get_hwnd(), m_modules_window.get_hwnd());
	m_upper_panel.setchildren(m_tree_window.get_hwnd(), m_right_panel.get_hwnd());
	m_right_panel.setchildren(m_import_window.get_hwnd(), m_export_window.get_hwnd());

	m_main_panel.setposition(0.8f);
	m_upper_panel.setposition(0.333f);

	m_toolbar_window.setcmdopen([](toolbar_window::cmd_open_ctx_t const& param){ assert(param); master_window_impl* const self = static_cast<master_window_impl*>(param); self->on_tb_open(); }, this);
	m_toolbar_window.setcmdfullpaths([](toolbar_window::cmd_fullpaths_ctx_t const& param){ assert(param); master_window_impl* const self = static_cast<master_window_impl*>(param); self->on_tb_fullpaths(); }, this);
	m_toolbar_window.setcmdundecorate([](toolbar_window::cmd_fullpaths_ctx_t const& param){ assert(param); master_window_impl* const self = static_cast<master_window_impl*>(param); self->on_tb_undecorate(); }, this);
	m_toolbar_window.setcmdproperties([](toolbar_window::cmd_fullpaths_ctx_t const& param){ assert(param); master_window_impl* const self = static_cast<master_window_impl*>(param); self->on_tb_properties(); }, this);

	m_tree_window.setonitemchanged([](tree_window::onitemchanged_ctx_t const& param, file_info const* const& fi){ assert(param); master_window_impl* const self = static_cast<master_window_impl*>(param); self->on_tree_changed(fi); }, this);
	m_tree_window.setcmdmatching([](tree_window::cmd_matching_ctx_t const& param, file_info const* const& fi){ assert(param); master_window_impl* const self = static_cast<master_window_impl*>(param); self->on_tree_matching(fi); }, this);
	m_tree_window.setcmdproperties([](tree_window::cmd_properties_ctx_t const& param, wstring_handle const& file_path){ assert(param); master_window_impl* const self = static_cast<master_window_impl*>(param); self->on_tree_properties(file_path); }, this);

	m_import_window.setcmdmatching([](import_window::cmd_matching_ctx_t const& param, std::uint16_t const& item_idx){ assert(param); master_window_impl* const self = static_cast<master_window_impl*>(param); self->on_imports_matching(item_idx); }, this);

	m_export_window.setcmdmatching([](export_window::cmd_matching_ctx_t const& param, std::uint16_t const& item_idx){ assert(param); master_window_impl* const self = static_cast<master_window_impl*>(param); self->on_exports_matching(item_idx); }, this);

	m_modules_window.setonitemchanged([](modules_window::onitemchanged_ctx_t const& param, file_info const* const& fi){ assert(param); master_window_impl* const self = static_cast<master_window_impl*>(param); self->on_modules_changed(fi); }, this);
	m_modules_window.setcmdmatching([](modules_window::cmd_matching_ctx_t const& param, file_info const* const& fi){ assert(param); master_window_impl* const self = static_cast<master_window_impl*>(param); self->on_modules_matching(fi); }, this);
	m_modules_window.setcmdproperties([](modules_window::cmd_properties_ctx_t const& param, wstring_handle const& file_path){ assert(param); master_window_impl* const self = static_cast<master_window_impl*>(param); self->on_modules_properties(file_path); }, this);
}

master_window_impl::~master_window_impl()
{
	--g_debug_instances;
}

void master_window_impl::init()
{
	register_class();
}

void master_window_impl::deinit()
{
	unregister_class();
}

wchar_t const* master_window_impl::get_class_atom()
{
	assert(g_class != 0);
	return reinterpret_cast<wchar_t const*>(g_class);
}

void master_window_impl::register_class()
{
	WNDCLASSEXW wc{};
	wc.cbSize = sizeof(wc);
	wc.style = 0;
	wc.lpfnWndProc = &class_proc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = get_instance();
	wc.hIcon = LoadIconW(nullptr, reinterpret_cast<wchar_t const*>(IDI_APPLICATION));
	wc.hCursor = LoadCursorW(nullptr, reinterpret_cast<wchar_t const*>(IDC_ARROW));
	wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = L"master_window";
	wc.hIconSm = wc.hIcon;

	assert(g_class == 0);
	g_class = RegisterClassExW(&wc);
	assert(g_class != 0);
}

void master_window_impl::unregister_class()
{
	assert(g_debug_instances == 0);

	assert(g_class != 0);
	BOOL const unregistered = UnregisterClassW(reinterpret_cast<wchar_t const*>(g_class), get_instance());
	assert(unregistered != 0);
	g_class = 0;
}

LRESULT CALLBACK master_window_impl::class_proc(HWND const hwnd, UINT const msg, WPARAM const wparam, LPARAM const lparam)
{
	if(msg == WM_CREATE)
	{
		LRESULT const ret = on_wm_create(hwnd, wparam, lparam);
		return ret;
	}
	else if(msg == WM_DESTROY)
	{
		LRESULT const ret = on_wm_destroy(hwnd, wparam, lparam);
		return ret;
	}
	else
	{
		LONG_PTR const self_ptr = GetWindowLongPtrW(hwnd, GWLP_USERDATA);
		if(self_ptr != 0)
		{
			master_window_impl* const self = reinterpret_cast<master_window_impl*>(self_ptr);
			LRESULT const ret = self->on_message(msg, wparam, lparam);
			return ret;
		}
		else
		{
			LRESULT const ret = DefWindowProcW(hwnd, msg, wparam, lparam);
			return ret;
		}
	}
}

LRESULT master_window_impl::on_wm_create(HWND const& hwnd, WPARAM const& wparam, LPARAM const& lparam)
{
	master_window_impl* const self = new master_window_impl{hwnd};
	assert((SetLastError(0), true));
	LONG_PTR const prev = SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
	assert(prev == 0 && GetLastError() == 0);

	LRESULT const ret = DefWindowProcW(hwnd, WM_CREATE, wparam, lparam);
	return ret;
}

LRESULT master_window_impl::on_wm_destroy(HWND const& hwnd, WPARAM const& wparam, LPARAM const& lparam)
{
	LONG_PTR const prev = SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
	assert(prev != 0);
	master_window_impl* const self = reinterpret_cast<master_window_impl*>(prev);
	delete self;

	LRESULT const ret = DefWindowProcW(hwnd, WM_DESTROY, wparam, lparam);
	return ret;
}

LRESULT master_window_impl::on_message(UINT const& msg, WPARAM const& wparam, LPARAM const& lparam)
{
	switch(msg)
	{
		case WM_SIZE:
		{
			LRESULT const ret = on_wm_size(wparam, lparam);
			return ret;
		}
		break;
		default:
		{
			LRESULT const ret = DefWindowProcW(m_self, msg, wparam, lparam);
			return ret;
		}
		break;
	}
}

LRESULT master_window_impl::on_wm_size(WPARAM const& wparam, LPARAM const& lparam)
{
	on_size();

	LRESULT const ret = DefWindowProcW(m_self, WM_SIZE, wparam, lparam);
	return ret;
}

void master_window_impl::on_size()
{
	RECT my_rect;
	BOOL const got_my_rect = GetClientRect(m_self, &my_rect);
	assert(got_my_rect != 0);
	assert(my_rect.left == 0 && my_rect.top == 0);

	RECT tb_rect;
	BOOL const got_tb_rect = GetClientRect(m_toolbar_window.get_hwnd(), &tb_rect);
	assert(got_tb_rect != 0);
	assert(tb_rect.left == 0 && tb_rect.top == 0);

	if(!(my_rect.bottom > tb_rect.bottom)){ return; }
	LONG const avail_height = my_rect.bottom - tb_rect.bottom;

	BOOL const invalidated_tb = RedrawWindow(m_toolbar_window.get_hwnd(), nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_FRAME); assert(invalidated_tb);
	BOOL const moved_mp = MoveWindow(m_main_panel.get_hwnd(), 0, tb_rect.bottom, my_rect.right, avail_height, TRUE); assert(moved_mp != 0);
}

void master_window_impl::on_tb_open()
{
	cmd_open();
}

void master_window_impl::on_tb_fullpaths()
{
}

void master_window_impl::on_tb_undecorate()
{
}

void master_window_impl::on_tb_properties()
{
}

void master_window_impl::on_tree_changed(file_info const* const& fi)
{
	m_import_window.setfi(fi);
	m_export_window.setfi(fi);
}

void master_window_impl::on_tree_matching(file_info const* const& fi)
{
	m_modules_window.selectitem(fi);
}

void master_window_impl::on_tree_properties(wstring_handle const& file_path)
{
}

void master_window_impl::on_imports_matching(std::uint16_t const& item_idx)
{
	m_export_window.selectitem(item_idx);
}

void master_window_impl::on_exports_matching(std::uint16_t const& item_idx)
{
	m_import_window.selectitem(item_idx);
}

void master_window_impl::on_modules_changed(file_info const* const& fi)
{
}

void master_window_impl::on_modules_matching(file_info const* const& fi)
{
	m_tree_window.selectitem(fi);
}

void master_window_impl::on_modules_properties(wstring_handle const& file_path)
{
}

void master_window_impl::cmd_open()
{
	int const buff_len = 1 * 1024 * 1024;
	wchar_t* const buff = new wchar_t[buff_len];
	auto const fn_free_buff = mk::make_scope_exit([&](){ delete[] buff; });
	wchar_t* const buff_end = buff + buff_len;
	buff[0] = L'\0';

	OPENFILENAMEW ofn{};
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = m_self;
	ofn.hInstance = nullptr;
	ofn.lpstrFilter = s_master_window_open_filter;
	ofn.lpstrCustomFilter = nullptr;
	ofn.nMaxCustFilter = 0;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = buff;
	ofn.nMaxFile = buff_len;
	ofn.lpstrFileTitle = nullptr;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = nullptr;
	ofn.lpstrTitle = nullptr;
	ofn.Flags = OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER | OFN_ENABLESIZING;
	ofn.nFileOffset = 0;
	ofn.nFileExtension = 0;
	ofn.lpstrDefExt = nullptr;
	ofn.lCustData = 0;
	ofn.lpfnHook = nullptr;
	ofn.lpTemplateName = nullptr;
	ofn.pvReserved = nullptr;
	ofn.dwReserved = 0;
	ofn.FlagsEx = 0;

	BOOL const opened = com_dlg::GetOpenFileNameW(&ofn);
	if(opened == 0)
	{
		return;
	}

	std::vector<std::wstring> file_paths;
	auto it = std::find(buff, buff_end, L'\0');
	assert(it != buff_end);
	if(it[1] == L'\0')
	{
		auto const b = buff;
		auto const e = it;
		int const count = 1;
		int const len = static_cast<int>(e - b);
		file_paths.resize(count);
		auto& path = file_paths.back();
		path.resize(len);
		std::copy(b, e, path.begin());
	}
	else
	{
		auto const folder_b = buff;
		auto const folder_e = it;
		int const folder_len = static_cast<int>(it - buff);
		int count = 0;
		do
		{
			it = std::find(it + 1, buff_end, L'\0');
			assert(it != buff_end);
			++count;
		}while(it[1] != L'\0');
		file_paths.resize(count);
		auto paths_it = file_paths.begin();
		auto prev = folder_e;
		do
		{
			auto const b = prev + 1;
			auto const e = std::find(b, buff_end, L'\0');
			assert(e != buff_end);
			int const name_len = static_cast<int>(e - b);
			int const path_len = folder_len + 1 + name_len;
			auto& path = *paths_it;
			path.resize(path_len);
			std::copy(folder_b, folder_e, path.begin());
			path[folder_len] = L'\\';
			std::copy(b, e, path.begin() + folder_len + 1);
			prev = e;
			++paths_it;
		}while(prev[1] != L'\0');
	}
	open_files(file_paths);
}

void master_window_impl::open_files(std::vector<std::wstring> const& file_paths)
{
	main_type mo;
	bool const processed = process(file_paths, &mo);
	if(!processed)
	{
		int const msgbox = MessageBoxW(m_self, s_master_window_error_msg, s_master_window_error_title, MB_OK | MB_ICONERROR);
		return;
	}
	m_mo.swap(mo);
	m_tree_window.setfi(m_mo.m_fi);
	m_modules_window.setmodlist(m_mo.m_modules_list);
	m_tree_window.selectitem(m_mo.m_fi->m_fis + 0);
}
