#pragma once


#include "../nogui/my_windows.h"

#include <commdlg.h>


namespace com_dlg
{
	void load();
	void unload();
	BOOL GetOpenFileNameW(LPOPENFILENAMEW ofn);
}
