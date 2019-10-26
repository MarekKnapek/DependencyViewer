#include "main_window.h"

#include "constants.h"
#include "main.h"
#include "smart_dc.h"
#include "test.h"

#include "../nogui/array_bool.h"
#include "../nogui/dbg_provider.h"
#include "../nogui/memory_mapped_file.h"
#include "../nogui/pe.h"
#include "../nogui/scope_exit.h"
#include "../nogui/smart_local_free.h"
#include "../nogui/utils.h"

#include "../res/resources.h"

#include <atomic>
#include <cassert>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iterator>

#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <windowsx.h>


static constexpr wchar_t const s_window_class_name[] = L"main_window";
static constexpr wchar_t const s_window_title[] = L"DLLDependencyViewer";
static constexpr wchar_t const s_menu_file[] = L"&File";
static constexpr wchar_t const s_menu_file_open[] = L"&Open...\tCtrl+O";
static constexpr wchar_t const s_menu_file_exit[] = L"E&xit\tCtrl+W";
static constexpr wchar_t const s_menu_view[] = L"&View";
static constexpr wchar_t const s_menu_view_paths[] = L"&Full Paths\tF9";
static constexpr wchar_t const s_menu_view_refresh[] = L"&Refresh\tF5";
static constexpr wchar_t const s_open_file_dialog_file_name_filter[] = L"Executable files and libraries (*.exe;*.dll;*.ocx)\0*.exe;*.dll;*.ocx\0All files\0*.*\0";
static constexpr wchar_t const s_msg_error[] = L"DLLDependencyViewer error.";
static constexpr wchar_t const s_toolbar_open_tooltip[] = L"Open... (Ctrl+O)";
static constexpr wchar_t const s_toolbar_full_paths_tooltip[] = L"View Full Paths (F9)";
static constexpr char const s_export_name_debug_na[] = "N/A";
static constexpr string const s_export_name_debug_na2 = {s_export_name_debug_na, static_cast<int>(std::size(s_export_name_debug_na)) - 1};
enum class e_main_menu_id : std::uint16_t
{
	e_open = s_main_view_menu_min,
	e_exit,
	e_full_paths,
	e_refresh,
};
enum class e_toolbar : std::uint16_t
{
	e_open,
	e_full_paths,
};
enum class e_accel : std::uint16_t
{
	e_main_open,
	e_main_exit,
	e_main_paths,
	e_main_refresh,
	e_main_matching,
	e_tree_orig,
};
static constexpr ACCEL const s_accel_table[] =
{
	{FVIRTKEY | FCONTROL, 'O',   static_cast<std::uint16_t>(e_accel::e_main_open    )},
	{FVIRTKEY | FCONTROL, 'W',   static_cast<std::uint16_t>(e_accel::e_main_exit    )},
	{FVIRTKEY,            VK_F9, static_cast<std::uint16_t>(e_accel::e_main_paths   )},
	{FVIRTKEY,            VK_F5, static_cast<std::uint16_t>(e_accel::e_main_refresh )},
	{FVIRTKEY | FCONTROL, 'M',   static_cast<std::uint16_t>(e_accel::e_main_matching)},
	{FVIRTKEY | FCONTROL, 'K',   static_cast<std::uint16_t>(e_accel::e_tree_orig    )},
};


static int g_ordinal_column_max_width = 0;


struct cancellable_task_param
{
	cancellable_task_param() : m_canceled(false){}
	std::atomic<bool> m_canceled;
};

struct generic_marshaller : public cancellable_task_param
{
public:
	generic_marshaller(HWND const mw) : cancellable_task_param(), m_mw(mw){}
public:
	HWND m_mw;
};

template<typename marshaller_t, typename fn_worker_t, typename fn_main_t>
void request_helper(main_window* const self, dbg_provider* const dbg, marshaller_t&& mrshllr, fn_worker_t const fn_worker_, fn_main_t const fn_main_)
{
	using fn_worker_tt = void(*)(marshaller_t&);
	using fn_main_tt = void(*)(main_window&, marshaller_t&);
	fn_worker_tt const fn_worker = fn_worker_;
	fn_main_tt const fn_main = fn_main_;

	struct marshaller_concrete : public generic_marshaller, public marshaller_t
	{
	public:
		marshaller_concrete(HWND const hwnd, marshaller_t&& m, fn_worker_tt const fn_worker, fn_main_tt const fn_main) : generic_marshaller(hwnd), marshaller_t(std::move(m)), m_fn_worker(fn_worker), m_fn_main(fn_main){}
	public:
		fn_worker_tt m_fn_worker;
		fn_main_tt m_fn_main;
	};

	auto const fn_thread_generic = [](thread_worker_param_t const param)
	{
		auto const fn_main_generic = [](main_window& self, idle_task_param_t const param)
		{
			assert(param);
			marshaller_concrete* const mc = static_cast<marshaller_concrete*>(param);
			std::unique_ptr<marshaller_concrete> const sp_mc(mc);
			auto const unregister_task = mk::make_scope_exit([&](){ self.unregister_dbg_task(param); });
			marshaller_t& m = *mc;
			if(mc->m_canceled.load() == false)
			{
				mc->m_fn_main(self, m);
			}
		};

		assert(param);
		marshaller_concrete* const mc = static_cast<marshaller_concrete*>(param);
		marshaller_t& m = *mc;
		if(mc->m_canceled.load() == false)
		{
			mc->m_fn_worker(m);
		}
		idle_task_t const fn_main_generic_ = fn_main_generic;
		idle_task_param_t const fn_main_generic_param = mc;
		BOOL const posted = PostMessageW(mc->m_mw, wm_main_window_add_idle_task, reinterpret_cast<WPARAM>(fn_main_generic_), reinterpret_cast<LPARAM>(fn_main_generic_param));
		assert(posted != 0);
	};

	auto mc = std::make_unique<marshaller_concrete>(self->m_hwnd, std::move(mrshllr), fn_worker, fn_main);

	thread_worker_function_t const fn_thread_generic_ = fn_thread_generic;
	thread_worker_param_t const fn_thread_generic_param = mc.release();
	self->register_dbg_task(fn_thread_generic_param);
	dbg->add_task(fn_thread_generic_, fn_thread_generic_param);
}


void main_window::register_class()
{
	WNDCLASSEXW wc;
	wc.cbSize = sizeof(wc);
	wc.style = 0;
	wc.lpfnWndProc = &main_window::class_proc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = get_instance();
	wc.hIcon = LoadIconW(nullptr, reinterpret_cast<wchar_t const*>(IDI_APPLICATION));
	wc.hCursor = LoadCursorW(nullptr, reinterpret_cast<wchar_t const*>(IDC_ARROW));
	wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = s_window_class_name;
	wc.hIconSm = LoadIconW(nullptr, reinterpret_cast<wchar_t const*>(IDI_APPLICATION));

	ATOM const klass = RegisterClassExW(&wc);

	g_class = klass;
}

void main_window::create_accel_table()
{
	assert(!g_accel);
	HACCEL const haccel = CreateAcceleratorTableW(const_cast<ACCEL*>(s_accel_table), static_cast<int>(std::size(s_accel_table)));
	assert(haccel != nullptr);
	g_accel = haccel;
}

HACCEL main_window::get_accell_table()
{
	assert(g_accel);
	return g_accel;
}

void main_window::destroy_accel_table()
{
	assert(g_accel);
	BOOL const destroyed = DestroyAcceleratorTable(g_accel);
	assert(destroyed != 0);
	g_accel = nullptr;
}

main_window::main_window() :
	m_hwnd(CreateWindowExW(0, reinterpret_cast<wchar_t const*>(g_class), s_window_title, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, create_menu(), get_instance(), nullptr)),
	m_toolbar(create_toolbar(m_hwnd)),
	m_splitter_hor(m_hwnd),
	m_tree_view(m_splitter_hor.get_hwnd(), *this),
	m_splitter_ver(m_splitter_hor.get_hwnd()),
	m_import_view(m_splitter_ver.get_hwnd(), *this),
	m_export_view(m_splitter_ver.get_hwnd(), *this),
	m_idle_tasks(),
	m_dbg_tasks(),
	m_mo(),
	m_settings()
{
	LONG_PTR const set = SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
	DragAcceptFiles(m_hwnd, TRUE);

	m_splitter_ver.set_elements(m_import_view.get_hwnd(), m_export_view.get_hwnd());
	m_splitter_hor.set_elements(m_tree_view.get_hwnd(), m_splitter_ver.get_hwnd());

	RECT r;
	BOOL const got_rect = GetClientRect(m_hwnd, &r);
	LRESULT const moved = on_wm_size(0, ((static_cast<unsigned>(r.bottom) & 0xFFFFu) << 16) | (static_cast<unsigned>(r.right) & 0xFFFFu));

	auto const process_cmd_line_task = [](main_window& self, idle_task_param_t const /*param*/) -> void { self.process_command_line(); };
	add_idle_task(process_cmd_line_task, nullptr);

	m_settings.m_full_paths = false;
}

main_window::~main_window()
{
	assert(m_idle_tasks.empty());
	assert(m_dbg_tasks.empty());
}

HWND main_window::get_hwnd() const
{
	return m_hwnd;
}

HMENU main_window::create_menu()
{
	HMENU const menu_bar = CreateMenu();
	assert(menu_bar != nullptr);

	HMENU const menu_file = CreatePopupMenu();
	assert(menu_file != nullptr);
	BOOL const menu_file_appended = AppendMenuW(menu_bar, MF_POPUP, reinterpret_cast<UINT_PTR>(menu_file), s_menu_file);
	assert(menu_file_appended != 0);

	BOOL const menu_file_open_appended = AppendMenuW(menu_file, MF_STRING, static_cast<std::uint16_t>(e_main_menu_id::e_open), s_menu_file_open);
	assert(menu_file_open_appended != 0);
	BOOL const menu_file_exit_appended = AppendMenuW(menu_file, MF_STRING, static_cast<std::uint16_t>(e_main_menu_id::e_exit), s_menu_file_exit);
	assert(menu_file_exit_appended != 0);

	HMENU const menu_view = CreatePopupMenu();
	assert(menu_view != nullptr);
	BOOL const menu_view_appended = AppendMenuW(menu_bar, MF_POPUP, reinterpret_cast<UINT_PTR>(menu_view), s_menu_view);
	assert(menu_view_appended != 0);

	BOOL const menu_view_paths_appended = AppendMenuW(menu_view, MF_STRING, static_cast<std::uint16_t>(e_main_menu_id::e_full_paths), s_menu_view_paths);
	assert(menu_view_paths_appended != 0);
	BOOL const menu_view_refresh_appended = AppendMenuW(menu_view, MF_STRING, static_cast<std::uint16_t>(e_main_menu_id::e_refresh), s_menu_view_refresh);
	assert(menu_view_refresh_appended != 0);

	return menu_bar;
}

HWND main_window::create_toolbar(HWND const& parent)
{
	HWND const toolbar = CreateWindowExW(0, TOOLBARCLASSNAMEW, nullptr, WS_CHILD | TBSTYLE_WRAPABLE | TBSTYLE_TOOLTIPS, 0, 0, 0, 0, parent, nullptr, get_instance(), nullptr);
	LRESULT const size_sent = SendMessageW(toolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
	TBADDBITMAP button_bitmap;
	button_bitmap.hInst = get_instance();
	button_bitmap.nID = s_res_icons_toolbar;
	LRESULT const bitmap_added = SendMessageW(toolbar, TB_ADDBITMAP, 2, reinterpret_cast<LPARAM>(&button_bitmap));
	assert(bitmap_added != -1);
	TBBUTTON buttons[2]{};
	buttons[0].iBitmap = s_res_icon_open;
	buttons[0].idCommand = static_cast<std::uint16_t>(e_toolbar::e_open);
	buttons[0].fsState = TBSTATE_ENABLED;
	buttons[0].fsStyle = BTNS_BUTTON;
	buttons[0].dwData = 0;
	buttons[0].iString = 0;
	buttons[1].iBitmap = s_res_icon_full_paths;
	buttons[1].idCommand = static_cast<std::uint16_t>(e_toolbar::e_full_paths);
	buttons[1].fsState = TBSTATE_ENABLED;
	buttons[1].fsStyle = BTNS_BUTTON;
	buttons[1].dwData = 0;
	buttons[1].iString = 0;
	LRESULT const buttons_added = SendMessageW(toolbar, TB_ADDBUTTONSW, 2, reinterpret_cast<LPARAM>(&buttons));
	assert(buttons_added == TRUE);
	SendMessageW(toolbar, TB_AUTOSIZE, 0, 0);
	BOOL const shown = ShowWindow(toolbar, TRUE);
	return toolbar;
}

LRESULT CALLBACK main_window::class_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if(LONG_PTR const ptr = GetWindowLongPtrW(hwnd, GWLP_USERDATA))
	{
		main_window& win = *reinterpret_cast<main_window*>(ptr);
		LRESULT const ret = win.on_message(msg, wparam, lparam);
		return ret;
	}
	LRESULT const ret = DefWindowProcW(hwnd, msg, wparam, lparam);
	return ret;
}

LRESULT main_window::on_message(UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch(msg)
	{
		case WM_DESTROY:
		{
			return on_wm_destroy(wparam, lparam);
		}
		break;
		case WM_SIZE:
		{
			return on_wm_size(wparam, lparam);
		}
		break;
		case WM_CLOSE:
		{
			return on_wm_close(wparam, lparam);
		}
		break;
		case WM_NOTIFY:
		{
			return on_wm_notify(wparam, lparam);
		}
		break;
		case WM_CONTEXTMENU:
		{
			return on_wm_contextmenu(wparam, lparam);
		}
		break;
		case WM_COMMAND:
		{
			return on_wm_command(wparam, lparam);
		}
		break;
		case WM_DROPFILES:
		{
			return on_wm_dropfiles(wparam, lparam);
		}
		break;
		case wm_main_window_add_idle_task:
		{
			return on_wm_main_window_add_idle_task(wparam, lparam);
		}
		break;
		case wm_main_window_process_on_idle:
		{
			return on_wm_main_window_process_on_idle(wparam, lparam);
		}
		break;
		default:
		{
			return DefWindowProcW(m_hwnd, msg, wparam, lparam);
		}
		break;
	}
	__assume(false);
}

LRESULT main_window::on_wm_destroy(WPARAM wparam, LPARAM lparam)
{
	LONG_PTR const set = SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(nullptr));
	HWND const old_hwnd = m_hwnd;
	m_hwnd = nullptr;

	PostQuitMessage(EXIT_SUCCESS);

	return DefWindowProcW(old_hwnd, WM_DESTROY, wparam, lparam);
}

LRESULT main_window::on_wm_size(WPARAM wparam, LPARAM lparam)
{
	int const w = LOWORD(lparam);
	int const h = HIWORD(lparam);
	SendMessageW(m_toolbar, TB_AUTOSIZE, 0, 0);
	RECT toolbar_rect;
	BOOL const got_toolbar_rect = GetClientRect(m_toolbar, &toolbar_rect);
	assert(toolbar_rect.left == 0);
	assert(toolbar_rect.top == 0);
	BOOL const moved = MoveWindow(m_splitter_hor.get_hwnd(), 0, 0 + toolbar_rect.bottom, w, h - toolbar_rect.bottom, TRUE);
	return DefWindowProcW(m_hwnd, WM_SIZE, wparam, lparam);
}

LRESULT main_window::on_wm_close(WPARAM wparam, LPARAM lparam)
{
	cancel_all_dbg_tasks();
	if(m_idle_tasks.empty() && m_dbg_tasks.empty())
	{
		return DefWindowProcW(m_hwnd, WM_CLOSE, wparam, lparam);
	}
	else
	{
		request_close();
		BOOL const hidden = ShowWindow(m_hwnd, SW_HIDE);
		return 0;
	}
}

LRESULT main_window::on_wm_notify(WPARAM wparam, LPARAM lparam)
{
	NMHDR& nmhdr = *reinterpret_cast<NMHDR*>(lparam);
	if(nmhdr.hwndFrom == m_tree_view.get_hwnd())
	{
		m_tree_view.on_notify(nmhdr);
	}
	else if(nmhdr.hwndFrom == m_import_view.get_hwnd())
	{
		m_import_view.on_notify(nmhdr);
	}
	else if(nmhdr.hwndFrom == m_export_view.get_hwnd())
	{
		m_export_view.on_notify(nmhdr);
	}
	else if(nmhdr.hwndFrom == m_toolbar)
	{
		on_toolbar_notify(nmhdr);
	}
	return DefWindowProcW(m_hwnd, WM_NOTIFY, wparam, lparam);
}

LRESULT main_window::on_wm_contextmenu(WPARAM wparam, LPARAM lparam)
{
	HWND const hwnd = reinterpret_cast<HWND>(wparam);
	if(hwnd == m_tree_view.get_hwnd())
	{
		m_tree_view.on_context_menu(lparam);
	}
	else if(hwnd == m_import_view.get_hwnd())
	{
		m_import_view.on_context_menu(lparam);
	}
	else if(hwnd == m_export_view.get_hwnd())
	{
		m_export_view.on_context_menu(lparam);
	}
	return DefWindowProcW(m_hwnd, WM_CONTEXTMENU, wparam, lparam);
}

LRESULT main_window::on_wm_command(WPARAM wparam, LPARAM lparam)
{
	if(HIWORD(wparam) == 0 && lparam == 0)
	{
		on_menu(wparam);
	}
	else if(HIWORD(wparam) == 1 && lparam == 0)
	{
		on_accelerator(wparam);
	}
	else if(reinterpret_cast<HWND>(lparam) == m_toolbar)
	{
		on_toolbar(wparam);
	}
	return DefWindowProcW(m_hwnd, WM_COMMAND, wparam, lparam);
}

LRESULT main_window::on_wm_dropfiles(WPARAM wparam, LPARAM lparam)
{
	static_assert(sizeof(WPARAM) == sizeof(HDROP), "");
	static_assert(sizeof(HDROP) == sizeof(void*), "");
	HDROP const hdrop = reinterpret_cast<HDROP>(wparam);
	auto const fn_finish_drop = mk::make_scope_exit([&](){ DragFinish(hdrop); });
	UINT const queried_1 = DragQueryFileW(hdrop, 0xFFFFFFFF, nullptr, 0);
	assert(queried_1 != 0);
	int const n = static_cast<int>(queried_1);
	auto file_names = std::make_unique<std::vector<std::wstring>>();
	file_names->resize(n);
	for(int i = 0; i != n; ++i)
	{
		UINT const queried_2 = DragQueryFileW(hdrop, i, nullptr, 0);
		assert(queried_2 != 0);
		int const size = static_cast<int>(queried_2);
		auto& file_name = (*file_names)[i];
		file_name.resize(size);
		UINT const queried_3 = DragQueryFileW(hdrop, i, const_cast<wchar_t*>(file_name.data()), size + 1); // TODO: Remove the cast.
		assert(queried_3 == queried_2);
	}
	idle_task_t const drop_task = [](main_window& self, idle_task_param_t const param)
	{
		assert(param);
		auto const typed_param = static_cast<std::vector<std::wstring>*>(param);
		std::unique_ptr<std::vector<std::wstring>> const file_names(typed_param);
		self.open_files(*file_names);
	};
	idle_task_param_t const drop_param = file_names.release();
	add_idle_task(drop_task, drop_param);
	return DefWindowProcW(m_hwnd, WM_DROPFILES, wparam, lparam);
}

LRESULT main_window::on_wm_main_window_add_idle_task(WPARAM wparam, LPARAM lparam)
{
	static_assert(sizeof(WPARAM) == sizeof(idle_task_t), "");
	static_assert(sizeof(LPARAM) == sizeof(idle_task_param_t), "");
	idle_task_t const task = reinterpret_cast<idle_task_t>(wparam);
	idle_task_param_t const param = reinterpret_cast<idle_task_param_t>(lparam);
	add_idle_task(task, param);
	return DefWindowProcW(m_hwnd, wm_main_window_add_idle_task, wparam, lparam);
}

LRESULT main_window::on_wm_main_window_process_on_idle(WPARAM wparam, LPARAM lparam)
{
	on_idle();
	return DefWindowProcW(m_hwnd, wm_main_window_process_on_idle, wparam, lparam);
}

void main_window::on_menu(WPARAM const wparam)
{
	std::uint16_t const menu_id = static_cast<std::uint16_t>(LOWORD(wparam));
	if(menu_id >= s_main_view_menu_min && menu_id < s_main_view_menu_max)
	{
		on_menu(menu_id);
	}
	else if(menu_id >= s_tree_view_menu_min && menu_id < s_tree_view_menu_max)
	{
		m_tree_view.on_menu(menu_id);
	}
	else if(menu_id >= s_import_view_menu_min && menu_id < s_import_view_menu_max)
	{
		m_import_view.on_menu(menu_id);
	}
	else if(menu_id >= s_export_view_menu_min && menu_id < s_export_view_menu_max)
	{
		m_export_view.on_menu(menu_id);
	}
}

void main_window::on_menu(std::uint16_t const menu_id)
{
	e_main_menu_id const e_menu = static_cast<e_main_menu_id>(menu_id);
	switch(e_menu)
	{
		case e_main_menu_id::e_open:
		{
			on_menu_open();
		}
		break;
		case e_main_menu_id::e_exit:
		{
			on_menu_exit();
		}
		break;
		case e_main_menu_id::e_full_paths:
		{
			on_menu_paths();
		}
		break;
		case e_main_menu_id::e_refresh:
		{
			on_menu_refresh();
		}
		break;
	}
}

void main_window::on_accelerator(WPARAM const wparam)
{
	e_accel const accel_id = static_cast<e_accel>(static_cast<std::uint16_t>(LOWORD(wparam)));
	switch(accel_id)
	{
		case e_accel::e_main_open:
		{
			on_accel_open();
		}
		break;
		case e_accel::e_main_exit:
		{
			on_accel_exit();
		}
		break;
		case e_accel::e_main_paths:
		{
			on_accel_paths();
		}
		break;
		case e_accel::e_main_refresh:
		{
			on_accel_refresh();
		}
		break;
		case e_accel::e_main_matching:
		{
			on_accel_matching();
		}
		break;
		case e_accel::e_tree_orig:
		{
			m_tree_view.on_accel_orig();
		}
		break;
		default:
		{
			assert(false);
		}
		break;
	}
}

void main_window::on_toolbar(WPARAM const wparam)
{
	e_toolbar const toolbar_id = static_cast<e_toolbar>(static_cast<std::uint16_t>(LOWORD(wparam)));
	switch(toolbar_id)
	{
		case e_toolbar::e_open:
		{
			on_toolbar_open();
		}
		break;
		case e_toolbar::e_full_paths:
		{
			on_toolbar_full_paths();
		}
		break;
		default:
		{
			assert(false);
		}
		break;
	}
}

void main_window::on_tree_selchangedw()
{
	m_import_view.refresh();
	m_export_view.refresh();
}

void main_window::on_toolbar_notify(NMHDR& nmhdr)
{
	switch(nmhdr.code)
	{
		case TBN_GETINFOTIPW:
		{
			NMTBGETINFOTIPW& tbgit = reinterpret_cast<NMTBGETINFOTIPW&>(nmhdr);
			e_toolbar const toolbar_id = static_cast<e_toolbar>(static_cast<std::uint16_t>(tbgit.iItem));
			switch(toolbar_id)
			{
				case e_toolbar::e_open:
				{
					tbgit.pszText = const_cast<wchar_t*>(s_toolbar_open_tooltip);
				}
				break;
				case e_toolbar::e_full_paths:
				{
					tbgit.pszText = const_cast<wchar_t*>(s_toolbar_full_paths_tooltip);
				}
				break;
			}
		}
		break;
	}
}

void main_window::on_menu_open()
{
	open();
}

void main_window::on_menu_exit()
{
	exit();
}

void main_window::on_menu_paths()
{
	full_paths();
}

void main_window::on_menu_refresh()
{
	refresh();
}

void main_window::on_accel_open()
{
	open();
}

void main_window::on_accel_exit()
{
	exit();
}

void main_window::on_accel_paths()
{
	full_paths();
}

void main_window::on_accel_refresh()
{
	refresh();
}

void main_window::on_accel_matching()
{
	HWND const focus = GetFocus();
	if(!focus)
	{
		return;
	}
	if(focus == m_import_view.get_hwnd())
	{
		m_import_view.on_accel_matching();
	}
	else if(focus == m_export_view.get_hwnd())
	{
		m_export_view.on_accel_matching();
	}
}

void main_window::on_toolbar_open()
{
	open();
}

void main_window::on_toolbar_full_paths()
{
	full_paths();
}

void main_window::open()
{
	auto const buff = std::make_unique<std::array<wchar_t, 1 * 1024 * 1024>>();
	(*buff)[0] = L'\0';

	OPENFILENAMEW ofn;
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = m_hwnd;
	ofn.hInstance = nullptr;
	ofn.lpstrFilter = s_open_file_dialog_file_name_filter;
	ofn.lpstrCustomFilter = nullptr;
	ofn.nMaxCustFilter = 0;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = buff->data();
	ofn.nMaxFile = static_cast<int>(buff->size());
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

	BOOL const opened = GetOpenFileNameW(&ofn);
	if(opened == 0)
	{
		return;
	}

	std::vector<std::wstring> file_paths;
	auto it = std::find(cbegin(*buff), cend(*buff), L'\0');
	auto const end = cend(*buff);
	assert(it != end);
	if(*(it + 1) == L'\0')
	{
		file_paths.resize(1);
		wchar_t const* const b = buff->data();
		wchar_t const* const e = &*it;
		file_paths[0].assign(b, e);
	}
	else
	{
		wchar_t const* const folder_b = buff->data();
		wchar_t const* const folder_e = &*it;
		auto next = it;
		it = std::find(it + 1, end, L'\0');
		do
		{
			wchar_t const* const b = &*(next + 1);
			wchar_t const* const e = &*it;
			file_paths.push_back(std::wstring{folder_b, folder_e} + std::wstring{L"\\"} + std::wstring{b, e});
			next = it;
			it = std::find(it + 1, end, L'\0');
		}while(it != next + 1);
	}
	open_files(file_paths);
}

void main_window::open_files(std::vector<std::wstring> const& file_paths)
{
	main_type mo;
	try
	{
		mo = process(file_paths);
	}
	catch(wchar_t const* const ex)
	{
		int const msgbox = MessageBoxW(m_hwnd, ex, s_msg_error, MB_OK | MB_ICONERROR);
		return;
	}
	refresh(std::move(mo));
}

void main_window::exit()
{
	LRESULT const sent = SendMessageW(m_hwnd, WM_CLOSE, 0, 0);
}

void main_window::refresh(main_type mo)
{
	cancel_all_dbg_tasks();

	auto tmp = std::make_unique<main_type>();
	tmp->swap(m_mo);
	m_mo.swap(mo);
	request_mo_deletion(std::move(tmp));

	m_tree_view.refresh();
	SetFocus(m_tree_view.get_hwnd());
}

void main_window::full_paths()
{
	m_settings.m_full_paths = !m_settings.m_full_paths;
	LRESULT const state_set = SendMessageW(m_toolbar, TB_SETSTATE, static_cast<std::uint16_t>(e_toolbar::e_full_paths), (m_settings.m_full_paths ? TBSTATE_PRESSED : 0) | TBSTATE_ENABLED);
	assert(state_set == TRUE);
	m_tree_view.repaint();
}

void main_window::refresh()
{
	if(m_mo.m_fi.m_sub_file_infos.empty())
	{
		return;
	}
	int const n = static_cast<int>(m_mo.m_fi.m_sub_file_infos.size());
	assert(n >= 1);
	std::vector<std::wstring> file_paths;
	file_paths.resize(n);
	for(int i = 0; i != n; ++i)
	{
		assert(m_mo.m_fi.m_sub_file_infos[i].m_sub_file_infos.size() == 1);
		wstring const* const& name = m_mo.m_fi.m_sub_file_infos[i].m_sub_file_infos[0].m_file_path;
		assert(name->m_str);
		assert(name->m_len > 0);
		file_paths[i].assign(name->m_str, name->m_str + name->m_len);
	}
	open_files(file_paths);
}

int main_window::get_ordinal_column_max_width()
{
	static constexpr std::uint16_t const s_ordinals[] =
	{
		11111,
		22222,
		33333,
		44444,
		55555,
		0x1111,
		0x2222,
		0x3333,
		0x4444,
		0x5555,
		0x6666,
		0x7777,
		0x8888,
		0x9999,
		0xAAAA,
		0xBBBB,
		0xCCCC,
		0xDDDD,
		0xEEEE,
		0xFFFF,
	};

	if(g_ordinal_column_max_width != 0)
	{
		return g_ordinal_column_max_width;
	}

	HWND const imprt = m_import_view.get_hwnd();
	HDC const dc = GetDC(imprt);
	assert(dc != NULL);
	smart_dc const sdc(imprt, dc);
	auto const orig_font = SelectObject(dc, reinterpret_cast<HFONT>(SendMessageW(imprt, WM_GETFONT, 0, 0)));
	auto const fn_revert = mk::make_scope_exit([&](){ SelectObject(dc, orig_font); });

	int maximum = 0;
	for(auto const& oridnal : s_ordinals)
	{
		static_assert(sizeof(std::uint16_t) == sizeof(unsigned short int), "");
		std::array<wchar_t, 15> buff;
		auto const n = static_cast<unsigned short int>(oridnal);
		int const printed = std::swprintf(buff.data(), buff.size(), L"%hu (0x%04hx)", n, n);
		assert(printed >= 0);

		SIZE size;
		BOOL const got = GetTextExtentPointW(dc, buff.data(), printed, &size);
		assert(got != 0);

		maximum = (std::max)(maximum, static_cast<int>(size.cx));
	}

	static constexpr int const s_trailing_label_padding = 12;
	g_ordinal_column_max_width = maximum + s_trailing_label_padding;
	return g_ordinal_column_max_width;
}

std::pair<file_info const*, POINT> main_window::get_file_info_under_cursor()
{
	POINT cursor_screen;
	BOOL const got_cursor_pos = GetCursorPos(&cursor_screen);
	assert(got_cursor_pos != 0);
	POINT cursor_client = cursor_screen;
	BOOL const converted = ScreenToClient(m_tree_view.get_hwnd(), &cursor_client);
	assert(converted != 0);
	TVHITTESTINFO hti;
	hti.pt = cursor_client;
	LPARAM const hit_tested = SendMessageW(m_tree_view.get_hwnd(), TVM_HITTEST, 0, reinterpret_cast<LPARAM>(&hti));
	assert(reinterpret_cast<HTREEITEM>(hit_tested) == hti.hItem);
	if(!(hti.hItem && (hti.flags & (TVHT_ONITEM | TVHT_ONITEMBUTTON | TVHT_ONITEMICON | TVHT_ONITEMLABEL | TVHT_ONITEMSTATEICON)) != 0))
	{
		return {nullptr, {}};
	}
	TVITEMEXW ti;
	ti.hItem = hti.hItem;
	ti.mask = TVIF_PARAM;
	LRESULT const got_item = SendMessageW(m_tree_view.get_hwnd(), TVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&ti));
	assert(got_item == TRUE);
	assert(ti.lParam);
	file_info const* const fi = reinterpret_cast<file_info*>(ti.lParam);
	return {fi, cursor_screen};
}

void main_window::add_idle_task(idle_task_t const task, idle_task_param_t const param)
{
	m_idle_tasks.push({task, param});
	BOOL const posted = PostMessageW(m_hwnd, wm_main_window_process_on_idle, 0, 0);
	assert(posted != 0);
}

void main_window::on_idle()
{
	if(m_idle_tasks.empty())
	{
		return;
	}
	auto const task_with_param = m_idle_tasks.front();
	m_idle_tasks.pop();
	auto const& task = task_with_param.first;
	auto const& param = task_with_param.second;
	(*task)(*this, param);
	if(!m_idle_tasks.empty())
	{
		BOOL const posted = PostMessageW(m_hwnd, wm_main_window_process_on_idle, 0, 0);
		assert(posted != 0);
	}
}

void main_window::process_command_line()
{
	wchar_t const* const cmd_line = GetCommandLineW();
	int argc;
	wchar_t** const argv = CommandLineToArgvW(cmd_line, &argc);
	smart_local_free const sp_argv(reinterpret_cast<void*>(argv));
	if(argc == 1)
	{
		return;
	}
	if(argc == 3 && std::wcsncmp(argv[1], s_cmd_arg_test, std::size(s_cmd_arg_test) - 1) == 0)
	{
		return;
	}
	std::vector<std::wstring> file_paths;
	file_paths.resize(argc - 1);
	for(int i = 1; i != argc; ++i)
	{
		file_paths[i - 1].assign(argv[i]);
	}
	open_files(file_paths);
}

void main_window::register_dbg_task(thread_worker_param_t const param)
{
	assert(param);
	m_dbg_tasks.push_back(param);
}

void main_window::unregister_dbg_task(thread_worker_param_t const param)
{
	assert(param);
	assert(!m_dbg_tasks.empty());
	assert(m_dbg_tasks.front() == param);
	(void)param;
	m_dbg_tasks.pop_front();
}

void main_window::cancel_all_dbg_tasks()
{
	std::for_each(m_dbg_tasks.begin(), m_dbg_tasks.end(), [](auto& e){ static_cast<cancellable_task_param*>(e)->m_canceled.store(true); });
}

void main_window::request_mo_deletion(std::unique_ptr<main_type>&& mo)
{
	struct marshaller
	{
		std::unique_ptr<main_type> m_mo;
	};
	marshaller m;
	m.m_mo.swap(mo);
	auto const fn_worker = [](marshaller&)
	{
	};
	auto const fn_main = [](main_window&, marshaller&)
	{
	};
	request_helper(this, dbg_provider::get(), std::move(m), fn_worker, fn_main);
}

void main_window::request_close()
{
	struct marshaller
	{
	};
	marshaller m;
	auto const fn_worker = [](marshaller&)
	{
	};
	auto const fn_main = [](main_window& self, marshaller&)
	{
		BOOL const posted = PostMessageW(self.m_hwnd, WM_CLOSE, 0, 0);
		assert(posted != 0);
	};
	request_helper(this, dbg_provider::get(), std::move(m), fn_worker, fn_main);
}

void main_window::request_symbols_from_addresses(file_info& fi)
{
	pe_export_table_info* const eti = &fi.m_export_table;
	auto const fn_is_unnamed = [](bool const is_rva, string const* const name){ return is_rva && !name; };
	std::uint16_t n = 0;
	for(std::uint16_t i = 0; i != fi.m_export_table.m_count; ++i)
	{
		bool const is_rva = array_bool_tst(fi.m_export_table.m_are_rvas, i);
		string const* const name = fi.m_export_table.m_names[i];
		if(fn_is_unnamed(is_rva, name))
		{
			++n;
		}
	}
	if(n == 0)
	{
		return;
	}
	std::vector<std::uint16_t> indexes;
	indexes.resize(n);
	std::uint16_t j = 0;
	for(std::uint16_t i = 0; i != fi.m_export_table.m_count; ++i)
	{
		bool const is_rva = array_bool_tst(fi.m_export_table.m_are_rvas, i);
		string const* const name = fi.m_export_table.m_names[i];
		if(!fn_is_unnamed(is_rva, name))
		{
			continue;
		}
		indexes[j] = i;
		++j;
	}

	struct marshaller
	{
		dbg_provider* m_dbg_provider;
		symbols_from_addresses_param_t m_param;
	};
	marshaller m;
	m.m_dbg_provider = dbg_provider::get();
	m.m_param.m_module_path = fi.m_file_path;
	m.m_param.m_eti = eti;
	m.m_param.m_indexes.swap(indexes);
	m.m_param.m_strings.resize(n);
	auto const fn_worker = [](marshaller& m)
	{
		m.m_dbg_provider->get_symbols_from_addresses_task(m.m_param);
	};
	auto const fn_main = [](main_window& self, marshaller& m)
	{
		self.finish_symbols_from_addresses(m.m_param);
	};
	request_helper(this, dbg_provider::get(), std::move(m), fn_worker, fn_main);
}

void main_window::finish_symbols_from_addresses(symbols_from_addresses_param_t const& param)
{
	assert(param.m_indexes.size() == param.m_strings.size());
	std::uint16_t const n = static_cast<std::uint16_t>(param.m_indexes.size());
	for(std::uint16_t i = 0; i != n; ++i)
	{
		std::uint16_t const idx = param.m_indexes[i];
		string const*& dbg_name = param.m_eti->m_debug_names[idx];
		if(!param.m_strings[i].empty())
		{
			dbg_name = m_mo.m_mm.m_strs.add_string(param.m_strings[i].c_str(), static_cast<int>(param.m_strings[i].size()), m_mo.m_mm.m_alc);
		}
		else
		{
			dbg_name = &s_export_name_debug_na2;
		}
	}
	HTREEITEM const selected = reinterpret_cast<HTREEITEM>(SendMessageW(m_tree_view.get_hwnd(), TVM_GETNEXTITEM, TVGN_CARET, 0));
	if(selected)
	{
		TVITEMW ti;
		ti.mask = TVIF_PARAM;
		ti.hItem = selected;
		LRESULT const got = SendMessageW(m_tree_view.get_hwnd(), TVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&ti));
		assert(got == TRUE);
		file_info const& fi_tmp = *reinterpret_cast<file_info*>(ti.lParam);
		file_info const& fi = fi_tmp.m_orig_instance ? *fi_tmp.m_orig_instance : fi_tmp;
		if(wstring_equal{}(fi.m_file_path, param.m_module_path))
		{
			m_import_view.repaint();
			m_export_view.repaint();
		}
	}
}

ATOM main_window::g_class = 0;
HACCEL main_window::g_accel = nullptr;
