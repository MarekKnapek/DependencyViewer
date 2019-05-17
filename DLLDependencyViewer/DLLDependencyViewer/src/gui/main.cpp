#include "main.h"

#include "main_window.h"
#include "splitter_window.h"

#include <cassert>

#include <commctrl.h>


#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif


static HINSTANCE g_instance;


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
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
