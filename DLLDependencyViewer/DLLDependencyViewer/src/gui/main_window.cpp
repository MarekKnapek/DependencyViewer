#include "main_window.h"

#include "constants.h"
#include "main.h"
#include "smart_dc.h"

#include "../nogui/dbg.h"
#include "../nogui/dbg_provider.h"
#include "../nogui/memory_mapped_file.h"
#include "../nogui/pe.h"
#include "../nogui/scope_exit.h"
#include "../nogui/smart_local_free.h"
#include "../nogui/utils.h"

#include "../res/resources.h"

#include <cassert>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iterator>

#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>


static constexpr wchar_t const s_window_class_name[] = L"main_window";
static constexpr wchar_t const s_window_title[] = L"DLLDependencyViewer";
static constexpr wchar_t const s_menu_file[] = L"&File";
static constexpr wchar_t const s_menu_file_open[] = L"&Open...\tCtrl+O";
static constexpr wchar_t const s_menu_file_exit[] = L"E&xit\tCtrl+W";
static constexpr wchar_t const s_menu_view[] = L"&View";
static constexpr wchar_t const s_menu_view_paths[] = L"&Full Paths\tF9";
static constexpr wchar_t const s_open_file_dialog_file_name_filter[] = L"Executable files and libraries (*.exe;*.dll;*.ocx)\0*.exe;*.dll;*.ocx\0All files\0*.*\0";
static constexpr wchar_t const s_msg_error[] = L"DLLDependencyViewer error.";
static constexpr wchar_t const* const s_import_headers[] = {L"PI", L"type", L"ordinal", L"hint", L"name"};
static constexpr wchar_t const s_import_type_true[] = L"ordinal";
static constexpr wchar_t const s_import_type_false[] = L"name";
static constexpr wchar_t const s_import_ordinal_na[] = L"N/A";
static constexpr wchar_t const s_import_hint_na[] = L"N/A";
static constexpr wchar_t const s_import_name_na[] = L"N/A";
static constexpr wchar_t const* const s_export_headers[] = {L"type", L"ordinal", L"hint", L"name", L"entry point"};
static constexpr wchar_t const s_toolbar_open_tooltip[] = L"Open... (Ctrl+O)";
static constexpr wchar_t const s_toolbar_full_paths_tooltip[] = L"View Full Paths (F9)";
static constexpr wchar_t const s_export_name_na[] = L"N/A";
static constexpr wstring const s_export_name_debug_na ={s_export_name_na, static_cast<int>(std::size(s_export_name_na)) - 1};
static constexpr int const s_menu_open_id = 2000;
static constexpr int const s_menu_exit_id = 2001;
static constexpr int const s_menu_paths_id = 2010;

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
	e_tree_orig,
	e_import_orig,
};
static constexpr ACCEL const s_accel_table[] =
{
	{FVIRTKEY | FCONTROL, 'O',   static_cast<std::uint16_t>(e_accel::e_main_open  )},
	{FVIRTKEY | FCONTROL, 'W',   static_cast<std::uint16_t>(e_accel::e_main_exit  )},
	{FVIRTKEY,            VK_F9, static_cast<std::uint16_t>(e_accel::e_main_paths )},
	{FVIRTKEY | FCONTROL, 'K',   static_cast<std::uint16_t>(e_accel::e_tree_orig  )},
	{FVIRTKEY | FCONTROL, 'M',   static_cast<std::uint16_t>(e_accel::e_import_orig)},
};

static constexpr int const s_import_menu_orig_id = 5002;
static constexpr wchar_t const s_import_menu_orig_str[] = L"&Highlight Matching Export Function\tCtrl+M";


static int g_import_type_column_max_width = 0;
static int g_twobyte_column_max_width = 0;


enum class e_import_column
{
	e_pi,
	e_type,
	e_ordinal,
	e_hint,
	e_name
};

struct drop_deleter
{
	void operator()(void* const& obj) const;
};
using smart_drop = std::unique_ptr<void, drop_deleter>;


void drop_deleter::operator()(void* const& obj) const
{
	DragFinish(reinterpret_cast<HDROP>(obj));
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
	wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
	wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = s_window_class_name;
	wc.hIconSm = LoadIconW(nullptr, IDI_APPLICATION);

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
	m_import_list(CreateWindowExW(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE, WC_LISTVIEWW, nullptr, WS_VISIBLE | WS_CHILD | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_OWNERDATA, 0, 0, 0, 0, m_splitter_ver.get_hwnd(), nullptr, get_instance(), nullptr)),
	m_export_view(m_splitter_ver.get_hwnd(), *this),
	m_import_menu(create_import_menu()),
	m_idle_tasks(),
	m_symbol_tasks(),
	m_import_tmp_strings(),
	m_export_tmp_strings(),
	m_import_tmp_string_idx(),
	m_export_tmp_string_idx(),
	m_mo(),
	m_settings()
{
	LONG_PTR const set = SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
	DragAcceptFiles(m_hwnd, TRUE);

	unsigned const extended_lv_styles = LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP | LVS_EX_DOUBLEBUFFER;
	LRESULT const set_import = SendMessageW(m_import_list, LVM_SETEXTENDEDLISTVIEWSTYLE, extended_lv_styles, extended_lv_styles);
	for(int i = 0; i != static_cast<int>(std::size(s_import_headers)); ++i)
	{
		LVCOLUMNW cl;
		cl.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		cl.fmt = LVCFMT_LEFT;
		cl.cx = 100;
		cl.pszText = const_cast<LPWSTR>(s_import_headers[i]);
		cl.cchTextMax = 0;
		cl.iSubItem = i;
		cl.iImage = 0;
		cl.iOrder = i;
		cl.cxMin = 0;
		cl.cxDefault = 0;
		cl.cxIdeal = 0;
		auto const inserted = SendMessageW(m_import_list, LVM_INSERTCOLUMNW, i, reinterpret_cast<LPARAM>(&cl));
	}
	HIMAGELIST const import_img_list = ImageList_LoadImageW(get_instance(), MAKEINTRESOURCEW(s_res_icons_import), 30, 0, CLR_DEFAULT, IMAGE_BITMAP, LR_DEFAULTCOLOR);
	assert(import_img_list);
	LRESULT const img_list_set_import = SendMessageW(m_import_list, LVM_SETIMAGELIST, LVSIL_SMALL, reinterpret_cast<LPARAM>(import_img_list));

	m_splitter_ver.set_elements(m_import_list, m_export_view.get_hwnd());
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
	assert(m_symbol_tasks.empty());
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

	BOOL const menu_file_open_appended = AppendMenuW(menu_file, MF_STRING, s_menu_open_id, s_menu_file_open);
	assert(menu_file_open_appended != 0);
	BOOL const menu_file_exit_appended = AppendMenuW(menu_file, MF_STRING, s_menu_exit_id, s_menu_file_exit);
	assert(menu_file_exit_appended != 0);

	HMENU const menu_view = CreatePopupMenu();
	assert(menu_view != nullptr);
	BOOL const menu_view_appended = AppendMenuW(menu_bar, MF_POPUP, reinterpret_cast<UINT_PTR>(menu_view), s_menu_view);
	assert(menu_view_appended != 0);

	BOOL const menu_view_paths_appended = AppendMenuW(menu_view, MF_STRING, s_menu_paths_id, s_menu_view_paths);
	assert(menu_view_paths_appended != 0);

	return menu_bar;
}

HMENU main_window::create_import_menu()
{
	HMENU const import_menu = CreatePopupMenu();
	assert(import_menu);
	MENUITEMINFOW mi{};
	mi.cbSize = sizeof(mi);
	mi.fMask = MIIM_ID | MIIM_STRING | MIIM_FTYPE;
	mi.fType = MFT_STRING;
	mi.wID = s_import_menu_orig_id;
	mi.dwTypeData = const_cast<wchar_t*>(s_import_menu_orig_str);
	BOOL const inserted = InsertMenuItemW(import_menu, 0, TRUE, &mi);
	assert(inserted != 0);
	return import_menu;
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
	SendMessage(toolbar, TB_AUTOSIZE, 0, 0);
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
		case wm_main_window_take_finished_dbg_task:
		{
			return on_wm_main_window_take_finished_dbg_task(wparam, lparam);
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
	SendMessage(m_toolbar, TB_AUTOSIZE, 0, 0);
	RECT toolbar_rect;
	BOOL const got_toolbar_rect = GetClientRect(m_toolbar, &toolbar_rect);
	assert(toolbar_rect.left == 0);
	assert(toolbar_rect.top == 0);
	BOOL const moved = MoveWindow(m_splitter_hor.get_hwnd(), 0, 0 + toolbar_rect.bottom, w, h - toolbar_rect.bottom, TRUE);
	return DefWindowProcW(m_hwnd, WM_SIZE, wparam, lparam);
}

LRESULT main_window::on_wm_close(WPARAM wparam, LPARAM lparam)
{
	request_cancellation_of_all_dbg_tasks();
	if(!m_idle_tasks.empty() || !m_symbol_tasks.empty())
	{
		MessageBoxW(m_hwnd, L"Please wait for background tasks to finish.", L"Could not close.", MB_OK);
		return 0;
	}
	return DefWindowProcW(m_hwnd, WM_CLOSE, wparam, lparam);
}

LRESULT main_window::on_wm_notify(WPARAM wparam, LPARAM lparam)
{
	NMHDR& nmhdr = *reinterpret_cast<NMHDR*>(lparam);
	if(nmhdr.hwndFrom == m_tree_view.get_hwnd())
	{
		m_tree_view.on_notify(nmhdr);
	}
	else if(nmhdr.hwndFrom == m_import_list)
	{
		on_import_notify(nmhdr);
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
	else if(hwnd == m_import_list)
	{
		on_import_context_menu(wparam, lparam);
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
	smart_drop sp_drop(hdrop);
	UINT const queried_1 = DragQueryFileW(hdrop, 0xFFFFFFFF, nullptr, 0);
	if(queried_1 != 1)
	{
		return DefWindowProcW(m_hwnd, WM_DROPFILES, wparam, lparam);
	}
	auto buff = std::make_unique<std::array<wchar_t, 32 * 1024>>();
	UINT const queried_2 = DragQueryFileW(hdrop, 0, buff->data(), static_cast<int>(buff->size()));
	idle_task_t const drop_task = [](main_window& self, idle_task_param_t const param)
	{
		assert(param);
		std::unique_ptr<std::array<wchar_t, 32 * 1024>> const buff(reinterpret_cast<std::array<wchar_t, 32 * 1024>*>(param));
		self.open_file(buff->data());
	};
	idle_task_param_t const drop_task_param = buff.release();
	add_idle_task(drop_task, drop_task_param);
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

LRESULT main_window::on_wm_main_window_take_finished_dbg_task(WPARAM wparam, LPARAM lparam)
{
	assert(lparam != 0);
	get_symbols_from_addresses_task_t* const task = reinterpret_cast<get_symbols_from_addresses_task_t*>(lparam);
	auto const fn_process_finished_dbg_task = [](main_window& self, idle_task_param_t const param) -> void { self.process_finished_dbg_task(reinterpret_cast<get_symbols_from_addresses_task_t*>(param)); };
	add_idle_task(fn_process_finished_dbg_task, task);
	return DefWindowProcW(m_hwnd, wm_main_window_take_finished_dbg_task, wparam, lparam);
}

void main_window::on_menu(WPARAM const wparam)
{
	std::uint16_t const menu_id = static_cast<std::uint16_t>(LOWORD(wparam));
	if(menu_id >= s_tree_view_menu_min && menu_id < s_tree_view_menu_max)
	{
		m_tree_view.on_menu(menu_id);
	}
	switch(menu_id)
	{
		case s_menu_open_id:
		{
			on_menu_open();
		}
		break;
		case s_menu_exit_id:
		{
			on_menu_exit();
		}
		break;
		case s_menu_paths_id:
		{
			on_menu_paths();
		}
		break;
		case s_import_menu_orig_id:
		{
			on_import_menu_orig();
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
		case e_accel::e_tree_orig:
		{
			m_tree_view.on_accel_orig();
		}
		break;
		case e_accel::e_import_orig:
		{
			on_accel_import_orig();
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
	m_export_view.refresh();

	LRESULT const redr_off_1 = SendMessageW(m_import_list, WM_SETREDRAW, FALSE, 0);
	LRESULT const deleted_1 = SendMessageW(m_import_list, LVM_DELETEALLITEMS, 0, 0);
	assert(deleted_1 == TRUE);
	auto const redraw_import = mk::make_scope_exit([&]()
	{
		LRESULT const redr_on = SendMessageW(m_import_list, WM_SETREDRAW, TRUE, 0);
		BOOL const redrawn = RedrawWindow(m_import_list, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_FRAME);
		assert(redrawn != 0);
	});

	HTREEITEM const selected = reinterpret_cast<HTREEITEM>(SendMessageW(m_tree_view.get_hwnd(), TVM_GETNEXTITEM, TVGN_CARET, 0));
	if(!selected)
	{
		return;
	}
	TVITEMEXW ti;
	ti.hItem = selected;
	ti.mask = TVIF_PARAM;
	LRESULT const got_1 = SendMessageW(m_tree_view.get_hwnd(), TVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&ti));
	assert(got_1 == TRUE);
	file_info const& fi_tmp = *reinterpret_cast<file_info*>(ti.lParam);
	HTREEITEM const parent = reinterpret_cast<HTREEITEM>(SendMessageW(m_tree_view.get_hwnd(), TVM_GETNEXTITEM, TVGN_PARENT, reinterpret_cast<LPARAM>(selected)));
	if(parent)
	{
		TVITEMEXW ti_2;
		ti_2.hItem = parent;
		ti_2.mask = TVIF_PARAM;
		LRESULT const got_2 = SendMessageW(m_tree_view.get_hwnd(), TVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&ti_2));
		assert(got_2 == TRUE);
		file_info const& parent_fi = *reinterpret_cast<file_info*>(ti_2.lParam);
		int const idx = static_cast<int>(&fi_tmp - parent_fi.m_sub_file_infos.data());
		LRESULT const set_size = SendMessageW(m_import_list, LVM_SETITEMCOUNT, parent_fi.m_import_table.m_dlls[idx].m_entries.size(), 0);
		assert(set_size != 0);
	}

	int const import_type_column_max_width = get_import_type_column_max_width();
	int const ordinal_column_max_width = get_ordinal_column_max_width();

	LRESULT const auto_sized_pi = SendMessageW(m_import_list, LVM_SETCOLUMNWIDTH, static_cast<int>(e_import_column::e_pi), LVSCW_AUTOSIZE);
	assert(auto_sized_pi == TRUE);
	LRESULT const type_sized_1 = SendMessageW(m_import_list, LVM_SETCOLUMNWIDTH, static_cast<int>(e_import_column::e_type), import_type_column_max_width);
	assert(type_sized_1 == TRUE);
	LRESULT const ordinal_sized_1 = SendMessageW(m_import_list, LVM_SETCOLUMNWIDTH, static_cast<int>(e_import_column::e_ordinal), ordinal_column_max_width);
	assert(ordinal_sized_1 == TRUE);
	LRESULT const hint_sized_1 = SendMessageW(m_import_list, LVM_SETCOLUMNWIDTH, static_cast<int>(e_import_column::e_hint), ordinal_column_max_width);
	assert(hint_sized_1 == TRUE);
}

void main_window::on_import_notify(NMHDR& nmhdr)
{
	if(nmhdr.code == LVN_GETDISPINFOW)
	{
		on_import_getdispinfow(nmhdr);
	}
}

void main_window::on_import_getdispinfow(NMHDR& nmhdr)
{
	NMLVDISPINFOW& nm = reinterpret_cast<NMLVDISPINFOW&>(nmhdr);
	HTREEITEM const selected = reinterpret_cast<HTREEITEM>(SendMessageW(m_tree_view.get_hwnd(), TVM_GETNEXTITEM, TVGN_CARET, 0));
	if(!selected)
	{
		return;
	}
	HTREEITEM const parent = reinterpret_cast<HTREEITEM>(SendMessageW(m_tree_view.get_hwnd(), TVM_GETNEXTITEM, TVGN_PARENT, reinterpret_cast<LPARAM>(selected)));
	if(!parent)
	{
		return;
	}
	TVITEMEXW ti;
	ti.hItem = parent;
	ti.mask = TVIF_PARAM;
	LRESULT const got_parent = SendMessageW(m_tree_view.get_hwnd(), TVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&ti));
	file_info const& parent_fi = *reinterpret_cast<file_info*>(ti.lParam);
	ti.hItem = selected;
	ti.mask = TVIF_PARAM;
	LRESULT const got_selected = SendMessageW(m_tree_view.get_hwnd(), TVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&ti));
	file_info const& fi = *reinterpret_cast<file_info*>(ti.lParam);
	file_info const& fi_orig = fi.m_orig_instance ? *fi.m_orig_instance : fi;
	int const idx = static_cast<int>(&fi - parent_fi.m_sub_file_infos.data());
	int const row = nm.item.iItem;
	int const col = nm.item.iSubItem;
	pe_import_entry const& import_entry = parent_fi.m_import_table.m_dlls[idx].m_entries[row];
	e_import_column const ecol = static_cast<e_import_column>(col);
	if((nm.item.mask | LVIF_TEXT) != 0)
	{
		switch(ecol)
		{
			case e_import_column::e_pi:
			{
				nm.item.pszText = const_cast<wchar_t*>(L"");
			}
			break;
			case e_import_column::e_type:
			{
				nm.item.pszText = const_cast<wchar_t*>(on_import_get_col_type(import_entry));
			}
			break;
			case e_import_column::e_ordinal:
			{
				nm.item.pszText = const_cast<wchar_t*>(on_import_get_col_ordinal(import_entry, fi_orig));
			}
			break;
			case e_import_column::e_hint:
			{
				nm.item.pszText = const_cast<wchar_t*>(on_import_get_col_hint(import_entry, fi_orig));
			}
			break;
			case e_import_column::e_name:
			{
				nm.item.pszText = const_cast<wchar_t*>(on_import_get_col_name(import_entry, fi_orig));
			}
			break;
			default:
			{
				assert(false);
			}
			break;
		}
	}
	if((nm.item.mask & LVIF_IMAGE) != 0)
	{
		bool const matched = import_entry.m_matched_export != 0xffff;
		bool const ordinal = import_entry.m_is_ordinal;
		if(matched && ordinal)
		{
			nm.item.iImage = s_res_icon_import_found_o;
		}
		else if(matched && !ordinal)
		{
			nm.item.iImage = s_res_icon_import_found_c;
		}
		else if(!matched && ordinal)
		{
			nm.item.iImage = s_res_icon_import_not_found_o;
		}
		else if(!matched && !ordinal)
		{
			nm.item.iImage = s_res_icon_import_not_found_c;
		}
		else
		{
			__assume(false);
		}
	}
}

wchar_t const* main_window::on_import_get_col_type(pe_import_entry const& import_entry)
{
	if(import_entry.m_is_ordinal)
	{
		return s_import_type_true;
	}
	else
	{
		return s_import_type_false;
	}
}

wchar_t const* main_window::on_import_get_col_ordinal(pe_import_entry const& import_entry, file_info const& fi)
{
	if(import_entry.m_is_ordinal)
	{
		static_assert(sizeof(std::uint16_t) == sizeof(unsigned short int), "");
		std::array<wchar_t, 32> buff;
		int const formatted = std::swprintf(buff.data(), buff.size(), L"%hu (0x%04hx)", static_cast<unsigned short int>(import_entry.m_ordinal_or_hint), static_cast<unsigned short int>(import_entry.m_ordinal_or_hint));
		std::wstring& tmpstr = m_import_tmp_strings[m_import_tmp_string_idx++ % m_import_tmp_strings.size()];
		tmpstr.assign(buff.data(), buff.data() + formatted);
		return tmpstr.c_str();
	}
	else
	{
		if(import_entry.m_matched_export != 0xffff)
		{
			return m_export_view.on_get_col_ordinal(fi.m_export_table.m_export_address_table[import_entry.m_matched_export]);
		}
		else
		{
			return s_import_ordinal_na;
		}
	}
}

wchar_t const* main_window::on_import_get_col_hint(pe_import_entry const& import_entry, file_info const& fi)
{
	if(import_entry.m_is_ordinal)
	{
		if(import_entry.m_matched_export != 0xffff)
		{
			return m_export_view.on_get_col_hint(fi.m_export_table.m_export_address_table[import_entry.m_matched_export]);
		}
		else
		{
			return s_import_hint_na;
		}
	}
	else
	{
		static_assert(sizeof(std::uint16_t) == sizeof(unsigned short int), "");
		std::array<wchar_t, 32> buff;
		int const formatted = std::swprintf(buff.data(), buff.size(), L"%hu (0x%04hx)", static_cast<unsigned short int>(import_entry.m_ordinal_or_hint), static_cast<unsigned short int>(import_entry.m_ordinal_or_hint));
		std::wstring& tmpstr = m_import_tmp_strings[m_import_tmp_string_idx++ % m_import_tmp_strings.size()];
		tmpstr.assign(buff.data(), buff.data() + formatted);
		return tmpstr.c_str();
	}
}

wchar_t const* main_window::on_import_get_col_name(pe_import_entry const& import_entry, file_info const& fi)
{
	if(import_entry.m_is_ordinal)
	{
		if(import_entry.m_matched_export != 0xffff)
		{
			return m_export_view.on_get_col_name(fi.m_export_table.m_export_address_table[import_entry.m_matched_export]);
		}
		else
		{
			return s_import_name_na;
		}
	}
	else
	{
		std::wstring& tmpstr = m_import_tmp_strings[m_import_tmp_string_idx++ % m_import_tmp_strings.size()];
		tmpstr.resize(import_entry.m_name->m_len);
		std::transform(cbegin(import_entry.m_name), cend(import_entry.m_name), begin(tmpstr), [](char const& e) -> wchar_t { return static_cast<wchar_t>(e); });
		return tmpstr.c_str();
	}
}

void main_window::on_import_context_menu(WPARAM wparam, LPARAM lparam)
{
	assert(reinterpret_cast<HWND>(wparam) == m_import_list);
	(void)wparam;
	POINT cursor_screen;
	int ith_import;
	if(lparam == 0xffffffff)
	{
		LRESULT const sel = SendMessageW(m_import_list, LVM_GETNEXTITEM, WPARAM{0} - 1, LVNI_SELECTED);
		if(sel == -1)
		{
			return;
		}
		RECT rect;
		rect.top = 1;
		rect.left = LVIR_BOUNDS;
		LRESULT const got_rect = SendMessageW(m_import_list, LVM_GETSUBITEMRECT, sel, reinterpret_cast<LPARAM>(&rect));
		if(got_rect == 0)
		{
			return;
		}
		cursor_screen.x = rect.left + (rect.right - rect.left) / 2;
		cursor_screen.y = rect.top + (rect.bottom - rect.top) / 2;
		BOOL const converted = ClientToScreen(m_import_list, &cursor_screen);
		assert(converted != 0);
		ith_import = static_cast<int>(sel);
	}
	else
	{
		cursor_screen.x = GET_X_LPARAM(lparam);
		cursor_screen.y = GET_Y_LPARAM(lparam);
		POINT cursor_client = cursor_screen;
		BOOL const converted = ScreenToClient(m_import_list, &cursor_client);
		assert(converted != 0);
		LVHITTESTINFO hti;
		hti.pt = cursor_client;
		hti.flags = LVHT_ONITEM;
		LPARAM const hit_tested = SendMessageW(m_import_list, LVM_HITTEST, 0, reinterpret_cast<LPARAM>(&hti));
		if(hit_tested == -1)
		{
			return;
		}
		assert(hit_tested == hti.iItem);
		ith_import = hti.iItem;
	}
	LRESULT const tree_sel = SendMessageW(m_tree_view.get_hwnd(), TVM_GETNEXTITEM, TVGN_CARET, 0);
	HTREEITEM const tree_selected = reinterpret_cast<HTREEITEM>(tree_sel);
	if(!tree_selected)
	{
		return;
	}
	LRESULT const got_tree_parent = SendMessageW(m_tree_view.get_hwnd(), TVM_GETNEXTITEM, TVGN_PARENT, reinterpret_cast<LPARAM>(tree_selected));
	HTREEITEM const tree_parent = reinterpret_cast<HTREEITEM>(got_tree_parent);
	if(!tree_parent)
	{
		return;
	}
	TVITEMEXW ti;
	ti.hItem = tree_selected;
	ti.mask = TVIF_PARAM;
	LRESULT const got_item = SendMessageW(m_tree_view.get_hwnd(), TVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&ti));
	assert(got_item == TRUE);
	assert(ti.lParam);
	file_info const& fi = *reinterpret_cast<file_info*>(ti.lParam);
	TVITEMEXW ti_2;
	ti_2.hItem = tree_parent;
	ti_2.mask = TVIF_PARAM;
	LRESULT const got_item_2 = SendMessageW(m_tree_view.get_hwnd(), TVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&ti_2));
	assert(got_item_2 == TRUE);
	assert(ti_2.lParam);
	file_info const& parent_fi = *reinterpret_cast<file_info*>(ti_2.lParam);
	int const ith_dll = static_cast<int>(&fi - parent_fi.m_sub_file_infos.data());
	std::uint16_t const& matched = parent_fi.m_import_table.m_dlls[ith_dll].m_entries[ith_import].m_matched_export;
	bool const enable_goto_orig = matched != 0xffff;
	HMENU const import_menu = reinterpret_cast<HMENU>(m_import_menu.get());
	BOOL const enabled = EnableMenuItem(import_menu, s_import_menu_orig_id, MF_BYCOMMAND | (enable_goto_orig ? MF_ENABLED : MF_GRAYED));
	assert(enabled != -1 && (enabled == MF_ENABLED || enabled == MF_GRAYED));
	BOOL const tracked = TrackPopupMenu(import_menu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON | TPM_NOANIMATION, cursor_screen.x, cursor_screen.y, 0, m_hwnd, nullptr);
	assert(tracked != 0);
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
	LRESULT const sent = SendMessageW(m_hwnd, WM_CLOSE, 0, 0);
}

void main_window::on_menu_paths()
{
	full_paths();
}

void main_window::on_import_menu_orig()
{
	import_select_original_instance();
}

void main_window::on_accel_open()
{
	open();
}

void main_window::on_accel_exit()
{
	LRESULT const sent = SendMessageW(m_hwnd, WM_CLOSE, 0, 0);
}

void main_window::on_accel_paths()
{
	full_paths();
}

void main_window::on_accel_import_orig()
{
	import_select_original_instance();
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
	auto const buff = std::make_unique<std::array<wchar_t, 32 * 1024>>();
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
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
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

	open_file(buff->data());
}

void main_window::open_file(wchar_t const* const file_path)
{
	main_type mo;
	try
	{
		mo = process(file_path);
	}
	catch(wchar_t const* const ex)
	{
		int const msgbox = MessageBoxW(m_hwnd, ex, s_msg_error, MB_OK | MB_ICONERROR);
		return;
	}
	refresh(std::move(mo));
}

void main_window::refresh(main_type&& mo)
{
	request_cancellation_of_all_dbg_tasks();
	m_mo = std::move(mo);

	LRESULT const redr_off_1 = SendMessageW(m_tree_view.get_hwnd(), WM_SETREDRAW, FALSE, 0);
	LRESULT const redr_off_2 = SendMessageW(m_import_list, WM_SETREDRAW, FALSE, 0);

	LRESULT const deleted_1 = SendMessageW(m_tree_view.get_hwnd(), TVM_DELETEITEM, 0, reinterpret_cast<LPARAM>(TVI_ROOT));
	LRESULT const deleted_2 = SendMessageW(m_import_list, LVM_DELETEALLITEMS, 0, 0);

	assert(m_mo.m_fi.m_sub_file_infos.size() == 1);
	refresh_view_recursive(m_mo.m_fi, TVI_ROOT);

	LRESULT const expanded = SendMessageW(m_tree_view.get_hwnd(), TVM_EXPAND, TVE_EXPAND, reinterpret_cast<LPARAM>(m_mo.m_fi.m_sub_file_infos[0].m_tree_item));
	LRESULT const selected = SendMessageW(m_tree_view.get_hwnd(), TVM_SELECTITEM, TVGN_CARET, reinterpret_cast<LPARAM>(m_mo.m_fi.m_sub_file_infos[0].m_tree_item));

	LRESULT const redr_on_1 = SendMessageW(m_tree_view.get_hwnd(), WM_SETREDRAW, TRUE, 0);
	LRESULT const redr_on_2 = SendMessageW(m_import_list, WM_SETREDRAW, TRUE, 0);
	m_export_view.refresh();
}

void main_window::refresh_view_recursive(file_info& parent_fi, HTREEITEM const& parent_ti)
{
	for(auto& fi : parent_fi.m_sub_file_infos)
	{
		TVINSERTSTRUCTW tvi;
		tvi.hParent = parent_ti;
		tvi.hInsertAfter = TVI_LAST;
		tvi.itemex.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_PARAM | TVIF_SELECTEDIMAGE;
		tvi.itemex.hItem = nullptr;
		tvi.itemex.state = 0;
		tvi.itemex.stateMask = 0;
		tvi.itemex.pszText = LPSTR_TEXTCALLBACKW;
		tvi.itemex.cchTextMax = 0;
		tvi.itemex.iImage = I_IMAGECALLBACK;
		tvi.itemex.iSelectedImage = I_IMAGECALLBACK;
		tvi.itemex.cChildren = 0;
		tvi.itemex.lParam = reinterpret_cast<LPARAM>(&fi);
		tvi.itemex.iIntegral = 0;
		tvi.itemex.uStateEx = 0;
		tvi.itemex.hwnd = nullptr;
		tvi.itemex.iExpandedImage = 0;
		tvi.itemex.iReserved = 0;
		HTREEITEM const ti = reinterpret_cast<HTREEITEM>(SendMessageW(m_tree_view.get_hwnd(), TVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&tvi)));
		fi.m_tree_item = ti;
		request_symbol_traslation(fi);
		refresh_view_recursive(fi, ti);
	}
}

void main_window::full_paths()
{
	m_settings.m_full_paths = !m_settings.m_full_paths;
	LRESULT const state_set = SendMessageW(m_toolbar, TB_SETSTATE, static_cast<std::uint16_t>(e_toolbar::e_full_paths), (m_settings.m_full_paths ? TBSTATE_PRESSED : 0) | TBSTATE_ENABLED);
	assert(state_set == TRUE);

	BOOL const tree_invalidated = InvalidateRect(m_tree_view.get_hwnd(), nullptr, TRUE);
	assert(tree_invalidated != 0);
}

void main_window::import_select_original_instance()
{
	LRESULT const sel = SendMessageW(m_import_list, LVM_GETNEXTITEM, WPARAM{0} - 1, LVNI_SELECTED);
	if(sel == -1)
	{
		return;
	}
	int const ith_import = static_cast<int>(sel);
	LRESULT const tree_sel = SendMessageW(m_tree_view.get_hwnd(), TVM_GETNEXTITEM, TVGN_CARET, 0);
	HTREEITEM const tree_selected = reinterpret_cast<HTREEITEM>(tree_sel);
	if(!tree_selected)
	{
		return;
	}
	LRESULT const got_tree_parent = SendMessageW(m_tree_view.get_hwnd(), TVM_GETNEXTITEM, TVGN_PARENT, reinterpret_cast<LPARAM>(tree_selected));
	HTREEITEM const tree_parent = reinterpret_cast<HTREEITEM>(got_tree_parent);
	if(!tree_parent)
	{
		return;
	}
	TVITEMEXW ti;
	ti.hItem = tree_selected;
	ti.mask = TVIF_PARAM;
	LRESULT const got_item = SendMessageW(m_tree_view.get_hwnd(), TVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&ti));
	assert(got_item == TRUE);
	assert(ti.lParam);
	file_info const& fi = *reinterpret_cast<file_info*>(ti.lParam);
	TVITEMEXW ti_2;
	ti_2.hItem = tree_parent;
	ti_2.mask = TVIF_PARAM;
	LRESULT const got_item_2 = SendMessageW(m_tree_view.get_hwnd(), TVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&ti_2));
	assert(got_item_2 == TRUE);
	assert(ti_2.lParam);
	file_info const& parent_fi = *reinterpret_cast<file_info*>(ti_2.lParam);
	int const ith_dll = static_cast<int>(&fi - parent_fi.m_sub_file_infos.data());
	std::uint16_t const& matched = parent_fi.m_import_table.m_dlls[ith_dll].m_entries[ith_import].m_matched_export;
	if(matched == 0xffff)
	{
		return;
	}
	m_export_view.select_item(matched);
}

int main_window::get_import_type_column_max_width()
{
	if(g_import_type_column_max_width != 0)
	{
		return g_import_type_column_max_width;
	}

	HDC const dc = GetDC(m_import_list);
	assert(dc != NULL);
	smart_dc sdc(m_import_list, dc);

	int maximum = 0;
	SIZE size;

	BOOL const got1 = GetTextExtentPointW(dc, s_import_type_true, static_cast<int>(std::size(s_import_type_true)) - 1, &size);
	assert(got1 != 0);
	maximum = (std::max)(maximum, static_cast<int>(size.cx));

	BOOL const got2 = GetTextExtentPointW(dc, s_import_type_false, static_cast<int>(std::size(s_import_type_false)) - 1, &size);
	assert(got2 != 0);
	maximum = (std::max)(maximum, static_cast<int>(size.cx));

	g_import_type_column_max_width = maximum;
	return maximum;
}

int main_window::get_ordinal_column_max_width()
{
	static constexpr std::uint16_t const s_twobytes[] =
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

	if(g_twobyte_column_max_width != 0)
	{
		return g_twobyte_column_max_width;
	}

	HDC const dc = GetDC(m_import_list);
	assert(dc != NULL);
	smart_dc sdc(m_import_list, dc);
	auto const orig_font = SelectObject(dc, reinterpret_cast<HFONT>(SendMessageW(m_import_list, WM_GETFONT, 0, 0)));
	auto const fn_revert = mk::make_scope_exit([&](){ SelectObject(dc, orig_font); });

	int maximum = 0;
	for(auto const& twobyte : s_twobytes)
	{
		static_assert(sizeof(std::uint16_t) == sizeof(unsigned short int), "");
		std::array<wchar_t, 15> buff;
		auto const n = static_cast<unsigned short int>(twobyte);
		int const printed = std::swprintf(buff.data(), buff.size(), L"%hu (0x%04hx)", n, n);
		assert(printed >= 0);

		SIZE size;
		BOOL const got = GetTextExtentPointW(dc, buff.data(), printed, &size);
		assert(got != 0);

		maximum = (std::max)(maximum, static_cast<int>(size.cx));
	}

	static constexpr int const s_trailing_label_padding = 12;
	g_twobyte_column_max_width = maximum + s_trailing_label_padding;
	return g_twobyte_column_max_width;
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
	if(argc != 2)
	{
		return;
	}
	open_file(argv[1]);
}

void dbg_task_callback_1(get_symbols_from_addresses_task_t* const task)
{
	assert(task);
	LRESULT const sent = SendMessageW(reinterpret_cast<HWND>(task->m_hwnd), wm_main_window_take_finished_dbg_task, 0, reinterpret_cast<LPARAM>(task));
}

void dbg_task_callback_2(get_symbols_from_addresses_task_t* const task)
{
	assert(task);
	std::unique_ptr<get_symbols_from_addresses_task_t> sp_task(task);
}

void main_window::request_symbol_traslation(file_info& fi)
{
	auto const fn_is_unnamed = [](pe_export_address_entry const& e){ return e.m_is_rva && !e.m_name; };
	int const n = static_cast<int>(std::count_if(fi.m_export_table.m_export_address_table.cbegin(), fi.m_export_table.m_export_address_table.cend(), fn_is_unnamed));
	if(n == 0)
	{
		return;
	}
	auto task = std::make_unique<get_symbols_from_addresses_task_t>();
	task->m_canceled.store(false);
	task->m_module_path.assign(fi.m_file_path->m_str, fi.m_file_path->m_len);
	task->m_addresses.resize(n);
	task->m_export_entries.resize(n);
	task->m_symbol_names.resize(n);
	int i = 0;
	for(auto& e : fi.m_export_table.m_export_address_table)
	{
		if(!fn_is_unnamed(e))
		{
			continue;
		}
		task->m_addresses[i] = e.rva_or_forwarder.m_rva;
		task->m_export_entries[i] = &e;
		++i;
	}
	task->m_callback_function.store(&dbg_task_callback_1);
	task->m_hwnd = reinterpret_cast<void*>(m_hwnd);
	get_symbols_from_addresses_task_t* const task_ptr = task.release();
	m_symbol_tasks.push_back(task_ptr);
	dbg_get_symbols_from_addresses(task_ptr);
}

void main_window::request_cancellation_of_all_dbg_tasks()
{
	if(m_symbol_tasks.empty())
	{
		return;
	}
	std::for_each(m_symbol_tasks.begin(), m_symbol_tasks.end(), [](auto const& e){ e->m_canceled.store(true); });
	for(auto it = m_symbol_tasks.begin(); it != m_symbol_tasks.end(); )
	{
		get_symbols_from_addresses_task_t::callback_function_t expected = &dbg_task_callback_1;
		bool const b = (*it)->m_callback_function.compare_exchange_strong(expected, &dbg_task_callback_2);
		if(b)
		{
			it = m_symbol_tasks.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void main_window::process_finished_dbg_task(get_symbols_from_addresses_task_t* const task)
{
	assert(task);
	assert(!m_symbol_tasks.empty());
	assert(task == m_symbol_tasks.front());
	std::unique_ptr<get_symbols_from_addresses_task_t> sp_task(task);
	auto const fn_destroy_task = mk::make_scope_exit([&](){ m_symbol_tasks.pop_front(); });
	if(task->m_canceled.load() == true)
	{
		return;
	}
	assert(task->m_export_entries.size() == task->m_symbol_names.size());
	int const n = static_cast<int>(task->m_export_entries.size());
	for(int i = 0; i != n; ++i)
	{
		if(!task->m_symbol_names[i].empty())
		{
			task->m_export_entries[i]->m_debug_name = m_mo.m_mm.m_wstrs.add_string(task->m_symbol_names[i].c_str(), static_cast<int>(task->m_symbol_names[i].size()), m_mo.m_mm.m_alc);
		}
		else
		{
			task->m_export_entries[i]->m_debug_name = &s_export_name_debug_na;
		}
	}
	BOOL const import_invalidated = InvalidateRect(m_import_list, nullptr, TRUE);
	assert(import_invalidated != 0);
	m_export_view.repaint();
}

ATOM main_window::g_class = 0;
HACCEL main_window::g_accel = nullptr;
