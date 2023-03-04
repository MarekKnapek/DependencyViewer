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


int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE /*hPrevInstance*/, _In_ PWSTR /*pCmdLine*/, _In_ int nCmdShow)
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
	BOOL const shown = ShowWindow(mw.get_hwnd(), nCmdShow); ((void)(shown));
	BOOL const updated = UpdateWindow(mw.get_hwnd()); ((void)(updated));
	int ret;
	for(;;)
	{
		MSG msg;
		while(PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE) != 0)
		{
			if(msg.message == WM_QUIT)
			{
				ret = static_cast<int>(msg.wParam);
				goto message_loop_end;
			}
			int const acc_transated = TranslateAcceleratorW(mw.get_hwnd(), mw.get_accell_table(), &msg);
			if(acc_transated == 0)
			{
				BOOL const translated = TranslateMessage(&msg); ((void)(translated));
				LRESULT const dispatched = DispatchMessageW(&msg); ((void)(dispatched));
			}
		}
		LRESULT const sent = SendMessageW(mw.get_hwnd(), wm_main_window_process_on_idle, 0, 0); ((void)(sent));
		BOOL const got_msg = GetMessageW(&msg, nullptr, 0, 0);
		if(got_msg == 0)
		{
			ret = static_cast<int>(msg.wParam);
			break;
		}
		else if(got_msg == -1)
		{
			assert(false);
			ret = 1;
			break;
		}
		int const acc_transated = TranslateAcceleratorW(mw.get_hwnd(), mw.get_accell_table(), &msg);
		if(acc_transated == 0)
		{
			BOOL const translated = TranslateMessage(&msg); ((void)(translated));
			LRESULT const dispatched = DispatchMessageW(&msg); ((void)(dispatched));
		}
	}
	message_loop_end:;
	return ret;
}

HINSTANCE get_instance()
{
	return g_instance;
}
