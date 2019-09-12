#pragma once


#include "smart_library.h"

#include <windows.h>
#define DBGHELP_TRANSLATE_TCHAR
#include <dbghelp.h>


class dbghelp
{
public:
	dbghelp();
	~dbghelp();
	bool init();
	void load_dlls();
public:
	smart_library m_dbghelp_dll;
	decltype(&SymGetOptions) m_fn_SymGetOptions;
	decltype(&SymSetOptions) m_fn_SymSetOptions;
	decltype(&SymInitializeW) m_fn_SymInitializeW;
	decltype(&SymCleanup) m_fn_SymCleanup;
	decltype(&SymLoadModuleExW) m_fn_SymLoadModuleExW;
	decltype(&SymUnloadModule64) m_fn_SymUnloadModule64;
	decltype(&SymFromAddrW) m_fn_SymFromAddrW;
	decltype(&UnDecorateSymbolNameW) m_fn_UnDecorateSymbolNameW;
	smart_library m_symsrv_dll;
};
