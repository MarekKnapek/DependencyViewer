#include "main.h"

#include "main_window.h"
#include "splitter_window.h"
#include "test.h"

#include "../nogui/activation_context.h"
#include "../nogui/dbg.h"
#include "../nogui/scope_exit.h"

#include <cassert>

#include <commctrl.h>


static HINSTANCE g_instance;


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	test();
	dbg_start();
	auto const fn_dbg_stop = mk::make_scope_exit([](){ dbg_stop(); });
	auto const fn_clean_actctx = mk::make_scope_exit([](){ activation_context::free_system_default_manifests(); });
	g_instance = hInstance;
	InitCommonControls();
	splitter_window_hor::register_class();
	splitter_window_ver::register_class();
	main_window::register_class();
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
			BOOL const translated = TranslateMessage(&msg);
			LRESULT const dispatched = DispatchMessageW(&msg);
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
		BOOL const translated = TranslateMessage(&msg);
		LRESULT const dispatched = DispatchMessageW(&msg);
	}
	message_loop_end:;
	return ret;
}

HINSTANCE get_instance()
{
	return g_instance;
}
