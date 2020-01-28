#pragma once


#include "smart_library.h"

#include "my_windows.h"

#include <dbghelp.h>


class dbghelp
{
public:
	dbghelp();
	~dbghelp();
	bool init();
	void deinit();
private:
	void set_env();
	bool load_dll();
	bool load_funcs();
private:
	bool win10_sdk_hardcoded();
	bool win10_sdk_program_files();
	bool win10_sdk_registry();
	bool system_default();
private:
	smart_library m_dbghelp_dll;
public:
	decltype(&SymGetOptions       	) m_fn_SymGetOptions       	;
	decltype(&SymSetOptions       	) m_fn_SymSetOptions       	;
	decltype(&SymInitializeW      	) m_fn_SymInitializeW      	;
	decltype(&SymCleanup          	) m_fn_SymCleanup          	;
	decltype(&SymLoadModuleExW    	) m_fn_SymLoadModuleExW    	;
	decltype(&SymUnloadModule64   	) m_fn_SymUnloadModule64   	;
	decltype(&SymFromAddr         	) m_fn_SymFromAddr         	;
	decltype(&UnDecorateSymbolName	) m_fn_UnDecorateSymbolName	;
};
