#include "main.h"

#include "main_window.h"
#include "splitter_window.h"
#include "test.h"

#include "../nogui/com.h"
#include "../nogui/dbg_provider.h"
#include "../nogui/default_act_ctx.h"
#include "../nogui/default_manifests.h"
#include "../nogui/file_name_provider.h"
#include "../nogui/known_dlls.h"
#include "../nogui/ole.h"
#include "../nogui/scope_exit.h"

#include <cassert>

#include <commctrl.h>


static HINSTANCE g_instance;


int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE /*hPrevInstance*/, _In_ PWSTR /*pCmdLine*/, _In_ int nCmdShow)
{
	com c;
	ole o;
	file_name_provider::init();
	auto const file_name_deinit = mk::make_scope_exit([](){ file_name_provider::deinit(); });
	init_known_dlls();
	auto const fn_deinit_known_dlls = mk::make_scope_exit([](){ deinit_known_dlls(); });
	test();
	dbg_provider::init();
	auto const dbg_provider_deinit = mk::make_scope_exit([](){ dbg_provider::deinit(); });
	auto const fn_clean_manifests = mk::make_scope_exit([](){ default_manifests::deinit(); });
	auto const fn_clean_actctx = mk::make_scope_exit([](){ default_act_ctx::deinit(); });
	g_instance = hInstance;
	InitCommonControls();
	splitter_window_hor::register_class();
	splitter_window_ver::register_class();
	main_window::register_class();
	main_window::create_accel_table();
	auto const fn_destroy_main_accel_table = mk::make_scope_exit([](){ main_window::destroy_accel_table(); });
	main_window mw;
	BOOL const shown = ShowWindow(mw.get_hwnd(), nCmdShow);
	BOOL const updated = UpdateWindow(mw.get_hwnd());
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
				BOOL const translated = TranslateMessage(&msg);
				LRESULT const dispatched = DispatchMessageW(&msg);
			}
		}
		LRESULT const sent = SendMessageW(mw.get_hwnd(), wm_main_window_process_on_idle, 0, 0);
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
			BOOL const translated = TranslateMessage(&msg);
			LRESULT const dispatched = DispatchMessageW(&msg);
		}
	}
	message_loop_end:;
	return ret;
}

HINSTANCE get_instance()
{
	return g_instance;
}
