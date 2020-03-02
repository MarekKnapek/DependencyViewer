#include "master_window_impl.h"

#include "com_dlg.h"
#include "main.h"

#include "../nogui/assert_my.h"
#include "../nogui/cassert_my.h"
#include "../nogui/com_ptr.h"
#include "../nogui/scope_exit.h"
#include "../nogui/utils.h"

#include <algorithm>

#include "../nogui/windows_my.h"

#include <commdlg.h>
#include <shellapi.h> // SHELLEXECUTEINFOW ShellExecuteExW
#include <shlobj.h> // SHGetDesktopFolder
#include <shobjidl.h> // SFGAO_FILESYSTEM CMINVOKECOMMANDINFO
#include <shobjidl_core.h> // IShellFolder IContextMenu
#include <shtypes.h> // ITEMIDLIST ITEMIDLIST_RELATIVE


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

	cmd_properties_avail();
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
	wstring_handle const file_path{};
	cmd_properties(file_path);
}

void master_window_impl::on_tree_changed(file_info const* const& fi)
{
	m_import_window.setfi(fi);
	m_export_window.setfi(fi);
	cmd_properties_avail();
}

void master_window_impl::on_tree_matching(file_info const* const& fi)
{
	m_modules_window.selectitem(fi);
}

void master_window_impl::on_tree_properties(wstring_handle const& file_path)
{
	cmd_properties(file_path);
}

void master_window_impl::on_imports_matching(std::uint16_t const& item_idx)
{
	m_export_window.selectitem(item_idx);
}

void master_window_impl::on_exports_matching(std::uint16_t const& item_idx)
{
	m_import_window.selectitem(item_idx);
}

void master_window_impl::on_modules_changed([[maybe_unused]] file_info const* const& fi)
{
	cmd_properties_avail();
}

void master_window_impl::on_modules_matching(file_info const* const& fi)
{
	m_tree_window.selectitem(fi);
}

void master_window_impl::on_modules_properties(wstring_handle const& file_path)
{
	cmd_properties(file_path);
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

void master_window_impl::cmd_properties_avail()
{
	bool const tree_avail = m_tree_window.iscmdpropertiesavail(nullptr);
	bool const modules_avail = m_modules_window.iscmdpropertiesavail(nullptr);
	bool const avail = tree_avail || modules_avail;
	m_toolbar_window.setpropertiesavail(avail);
}

void master_window_impl::cmd_properties(wstring_handle const& file_path)
{
	wstring_handle const path = [&]() -> wstring_handle
	{
		if(file_path)
		{
			return file_path;
		}
		HWND const focus = GetFocus();
		wstring_handle tree_file_path; bool const tree_avail = m_tree_window.iscmdpropertiesavail(&tree_file_path);
		if((IsChild(m_tree_window.get_hwnd(), focus) != 0 || focus == m_tree_window.get_hwnd()) && tree_avail)
		{
			assert(tree_file_path);
			return tree_file_path;
		}
		wstring_handle modules_file_path; bool const modules_avail = m_modules_window.iscmdpropertiesavail(&modules_file_path);
		if((IsChild(m_modules_window.get_hwnd(), focus) != 0 || focus == m_modules_window.get_hwnd()) && tree_avail)
		{
			assert(modules_file_path);
			return modules_file_path;
		}
		if(tree_avail)
		{
			assert(tree_file_path);
			return tree_file_path;
		}
		if(modules_avail)
		{
			assert(modules_file_path);
			return modules_file_path;
		}
		return {};
	}();
	if(!path)
	{
		return;
	}
	bool const new_style_succeeded = properties_new_style(path);
	if(!new_style_succeeded)
	{
		properties_old_style(path);
	}
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

bool master_window_impl::properties_new_style(wstring_handle const& file_path)
{
	assert(file_path);

	IShellFolder* desktop_folder;
	HRESULT const got_desktop = SHGetDesktopFolder(&desktop_folder);
	WARN_M_R(got_desktop == S_OK, L"Failed to SHGetDesktopFolder.", false);
	com_ptr<IShellFolder> const desktop_sp(desktop_folder);

	wchar_t const* const file_path_b = begin(file_path);
	wchar_t const* const file_name = find_file_name(file_path_b, size(file_path));
	assert(file_name != file_path_b);
	int const file_folder_len = static_cast<int>(file_name - file_path_b);
	wchar_t* const file_folder = new wchar_t[file_folder_len + 1];
	auto const fn_free_file_folder = mk::make_scope_exit([&](){ delete[] file_folder; });
	std::copy(file_path_b, file_path_b + file_folder_len, file_folder);
	file_folder[file_folder_len] = L'\0';

	ITEMIDLIST_RELATIVE* sub_folder_rel;
	ULONG attributes_1 = SFGAO_FILESYSTEM;
	HRESULT const parsed_display_name_1 = desktop_folder->lpVtbl->ParseDisplayName(desktop_folder, nullptr, nullptr, file_folder, nullptr, &sub_folder_rel, &attributes_1);
	WARN_M_R(parsed_display_name_1 == S_OK, L"Failed to IShellFolder::ParseDisplayName.", false);
	WARN_M_R((attributes_1 & SFGAO_FILESYSTEM) != 0, L"Shell item has not SFGAO_FILESYSTEM attribute.", false);
	auto const free_sub_folder_rel = mk::make_scope_exit([&](){ CoTaskMemFree(sub_folder_rel); });

	IShellFolder* sub_folder;
	HRESULT const bound = desktop_folder->lpVtbl->BindToObject(desktop_folder, sub_folder_rel, nullptr, IID_IShellFolder, reinterpret_cast<void**>(&sub_folder));
	WARN_M_R(bound == S_OK, L"Failed to IShellFolder::BindToObject.", false);
	com_ptr<IShellFolder> const sub_folder_sp(sub_folder);

	ITEMIDLIST_RELATIVE* sub_file_rel;
	ULONG attributes_2 = SFGAO_FILESYSTEM;
	HRESULT const parsed_display_name_2 = sub_folder->lpVtbl->ParseDisplayName(sub_folder, nullptr, nullptr, const_cast<wchar_t*>(file_name), nullptr, &sub_file_rel, &attributes_2);
	WARN_M_R(parsed_display_name_2 == S_OK, L"Failed to IShellFolder::ParseDisplayName.", false);
	WARN_M_R((attributes_2 & SFGAO_FILESYSTEM) != 0, L"Shell item has not SFGAO_FILESYSTEM attribute.", false);
	auto const free_sub_file_rel = mk::make_scope_exit([&](){ CoTaskMemFree(sub_file_rel); });

	ITEMIDLIST const* chidren[1];
	chidren[0] = sub_file_rel;
	IContextMenu* context_menu;
	HRESULT const got_ui_object = sub_folder->lpVtbl->GetUIObjectOf(sub_folder, nullptr, 1, chidren, IID_IContextMenu, nullptr, reinterpret_cast<void**>(&context_menu));
	WARN_M_R(got_ui_object == S_OK, L"Failed to IShellFolder::GetUIObjectOf.", false);
	com_ptr<IContextMenu> const context_menu_sp(context_menu);

	HMENU const menu = CreatePopupMenu();
	assert(menu);
	auto const destroy_menu = mk::make_scope_exit([&](){ BOOL const destroyed = DestroyMenu(menu); assert(destroyed != 0); });
	HRESULT const menu_queried = context_menu->lpVtbl->QueryContextMenu(context_menu, menu, 0, 1, 0x7FF, CMF_NORMAL);
	WARN_M_R(SUCCEEDED(menu_queried), L"Failed to IContextMenu::QueryContextMenu.", false);

	CMINVOKECOMMANDINFO info{};
	info.cbSize = sizeof(info);
	info.lpVerb = "properties";
	HRESULT const command_invoked = context_menu->lpVtbl->InvokeCommand(context_menu, &info);
	WARN_M_R(command_invoked == S_OK, L"Failed to IContextMenu::InvokeCommand.", false);

	return true;
}

void master_window_impl::properties_old_style(wstring_handle const& file_path)
{
	assert(file_path);
	SHELLEXECUTEINFOW info{};
	info.cbSize = sizeof(info);
	info.fMask = SEE_MASK_INVOKEIDLIST;
	info.lpVerb = L"properties";
	info.lpFile = file_path.m_string->m_str;
	info.nShow = SW_SHOWNORMAL;
	BOOL const executed = ShellExecuteExW(&info);
	assert(executed != FALSE);
	assert(static_cast<int>(reinterpret_cast<std::uintptr_t>(info.hInstApp)) > 32);
}
