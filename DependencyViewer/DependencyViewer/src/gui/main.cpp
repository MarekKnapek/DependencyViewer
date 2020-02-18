#include "main.h"

#include "com_dlg.h"
#include "common_controls.h"
#include "main_window.h"
#include "splitter_window.h"
#include "test.h"

#include "../nogui/cassert_my.h"
#include "../nogui/com.h"
#include "../nogui/dbg_provider.h"
#include "../nogui/file_name_provider.h"
#include "../nogui/known_dlls.h"
#include "../nogui/my_actctx.h"
#include "../nogui/ole.h"
#include "../nogui/scope_exit.h"

#include "../nogui/my_windows.h"

#include <commctrl.h>


static HINSTANCE g_instance;


HINSTANCE get_instance()
{
	return g_instance;
}


int message_loop(main_window& mw);
void process_message(main_window& mw, MSG& msg);
void process_on_idle(main_window& mw);


int WINAPI wWinMain(_In_ HINSTANCE hInstance, [[maybe_unused]] _In_opt_ HINSTANCE hPrevInstance, [[maybe_unused]] _In_ PWSTR pCmdLine, _In_ int nCmdShow)
{
	my_actctx::create();
	my_actctx::activate();
	auto const actctx_done = mk::make_scope_exit([](){ my_actctx::deactivate(); my_actctx::destroy(); });
	common_controls::load();
	auto const common_controls_unload = mk::make_scope_exit([](){ common_controls::unload(); });
	com_dlg::load();
	auto const com_dlg_unload = mk::make_scope_exit([](){ com_dlg::unload(); });
	com c;
	ole o;
	file_name_provider::init();
	auto const file_name_deinit = mk::make_scope_exit([](){ file_name_provider::deinit(); });
	auto const fn_clean_known_dlls = mk::make_scope_exit([](){ known_dlls::deinit(); });
	test();
	auto const dbg_provider_deinit = mk::make_scope_exit([](){ dbg_provider::deinit(); });
	g_instance = hInstance;
	common_controls::InitCommonControls();
	splitter_window_hor::register_class();
	splitter_window_ver::register_class();
	main_window::register_class();
	main_window::create_accel_table();
	auto const fn_destroy_main_accel_table = mk::make_scope_exit([](){ main_window::destroy_accel_table(); });
	main_window mw;
	BOOL const shown = ShowWindow(mw.get_hwnd(), nCmdShow);
	BOOL const updated = UpdateWindow(mw.get_hwnd());
	int const exit_code = message_loop(mw);
	return exit_code;
}


int message_loop(main_window& mw)
{
	for(;;)
	{
		MSG msg;
		BOOL const peeked_msg = PeekMessageW(&msg, nullptr, 0, 0, PM_NOREMOVE);
		bool const msg_avail = peeked_msg != 0;
		if(!msg_avail)
		{
			process_on_idle(mw);
		}
		BOOL const got_msg = GetMessageW(&msg, nullptr, 0, 0);
		if(got_msg == 0)
		{
			assert(msg.hwnd == nullptr);
			assert(msg.message == WM_QUIT);
			int const exit_code = static_cast<int>(msg.wParam);
			return exit_code;
		}
		assert(got_msg != -1);
		process_message(mw, msg);
	}
}

void process_message(main_window& mw, MSG& msg)
{
	bool accel_translated;
	BOOL const is_child = IsChild(mw.get_hwnd(), msg.hwnd);
	if(is_child != 0)
	{
		accel_translated = TranslateAcceleratorW(mw.get_hwnd(), mw.get_accell_table(), &msg) != 0;
	}
	else
	{
		accel_translated = false;
	}
	if(!accel_translated)
	{
		BOOL const translated = TranslateMessage(&msg);
		LRESULT const dispatched = DispatchMessageW(&msg);
	}
}

void process_on_idle(main_window& mw)
{
	LRESULT const res = SendMessageW(mw.get_hwnd(), wm_main_window_process_on_idle, 0, 0);
}
