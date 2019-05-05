#include "main.h"

#include "main_window.h"

#include <cassert>


static HINSTANCE g_instance;


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	g_instance = hInstance;
	main_window::register_class();
	main_window mw;
	BOOL const shown = ShowWindow(mw.get_hwnd(), nCmdShow);
	BOOL const updated = UpdateWindow(mw.get_hwnd());
	int ret;
	for(;;)
	{
		MSG msg;
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
	return ret;
}

HINSTANCE get_instance()
{
	return g_instance;
}
