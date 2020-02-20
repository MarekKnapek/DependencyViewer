#pragma once


#include "../nogui/windows_my.h"

#include <commdlg.h>


namespace com_dlg
{
	void load();
	void unload();
	BOOL GetOpenFileNameW(LPOPENFILENAMEW ofn);
}
