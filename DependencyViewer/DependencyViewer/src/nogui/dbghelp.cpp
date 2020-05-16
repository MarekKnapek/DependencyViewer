#include "dbghelp.h"

#include "cassert_my.h"
#include "format_string.h"
#include "scope_exit.h"
#include "smart_reg_key.h"
#include "wow.h"

#include <array>
#include <filesystem>
#include <string>

#include "my_windows.h"

#include <objbase.h>
#include <shlobj.h>


dbghelp::dbghelp():
	m_dbghelp_dll(),
	m_fn_SymGetOptions(),
	m_fn_SymSetOptions(),
	m_fn_SymInitializeW(),
	m_fn_SymCleanup(),
	m_fn_SymLoadModuleExW(),
	m_fn_SymUnloadModule64(),
	m_fn_SymFromAddr(),
	m_fn_UnDecorateSymbolName()
{
}

dbghelp::~dbghelp()
{
}

bool dbghelp::init()
{
	set_env();
	if(!load_dll()) { return false; }
	if(!load_funcs()) { return false; }
	return true;
}

void dbghelp::deinit()
{
	m_fn_SymGetOptions       	= nullptr;
	m_fn_SymSetOptions       	= nullptr;
	m_fn_SymInitializeW      	= nullptr;
	m_fn_SymCleanup          	= nullptr;
	m_fn_SymLoadModuleExW    	= nullptr;
	m_fn_SymUnloadModule64   	= nullptr;
	m_fn_SymFromAddr         	= nullptr;
	m_fn_UnDecorateSymbolName	= nullptr;
	m_dbghelp_dll.reset();
}

void dbghelp::set_env()
{
	std::array<wchar_t, 32 * 1024> buff;
	DWORD const got_evn = GetEnvironmentVariableW(L"_NT_SYMBOL_PATH", buff.data(), static_cast<int>(buff.size()));
	if(!(got_evn == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND)) { return; }
	DWORD const got_tmp = GetTempPathW(static_cast<int>(buff.size()), buff.data());
	if(got_tmp == 0 || got_tmp >= static_cast<int>(buff.size())) { return; }
	std::filesystem::path p(buff.data(), buff.data() + got_tmp);
	p.append(L"SymbolCache");
	std::wstring const nt_symbol_path = format_string(LR"---(srv*%s*https://msdl.microsoft.com/download/symbols)---", p.wstring().c_str());
	BOOL const set = SetEnvironmentVariableW(L"_NT_SYMBOL_PATH", nt_symbol_path.c_str());
	if(set == 0) { return; }
}

bool dbghelp::load_dll()
{
	if(win10_sdk_hardcoded    	()) { return true; }
	if(win10_sdk_program_files	()) { return true; }
	if(win10_sdk_registry     	()) { return true; }
	if(system_default         	()) { return true; }
	return false;
}

bool dbghelp::load_funcs()
{
	assert(m_dbghelp_dll);
	m_fn_SymGetOptions       	= reinterpret_cast<decltype(&SymGetOptions       	)>(get_function_address(m_dbghelp_dll, "SymGetOptions"       	)); if(!m_fn_SymGetOptions       	) { return false; }
	m_fn_SymSetOptions       	= reinterpret_cast<decltype(&SymSetOptions       	)>(get_function_address(m_dbghelp_dll, "SymSetOptions"       	)); if(!m_fn_SymSetOptions       	) { return false; }
	m_fn_SymInitializeW      	= reinterpret_cast<decltype(&SymInitializeW      	)>(get_function_address(m_dbghelp_dll, "SymInitializeW"      	)); if(!m_fn_SymInitializeW      	) { return false; }
	m_fn_SymCleanup          	= reinterpret_cast<decltype(&SymCleanup          	)>(get_function_address(m_dbghelp_dll, "SymCleanup"          	)); if(!m_fn_SymCleanup          	) { return false; }
	m_fn_SymLoadModuleExW    	= reinterpret_cast<decltype(&SymLoadModuleExW    	)>(get_function_address(m_dbghelp_dll, "SymLoadModuleExW"    	)); if(!m_fn_SymLoadModuleExW    	) { return false; }
	m_fn_SymUnloadModule64   	= reinterpret_cast<decltype(&SymUnloadModule64   	)>(get_function_address(m_dbghelp_dll, "SymUnloadModule64"   	)); if(!m_fn_SymUnloadModule64   	) { return false; }
	m_fn_SymFromAddr         	= reinterpret_cast<decltype(&SymFromAddr         	)>(get_function_address(m_dbghelp_dll, "SymFromAddr"         	)); if(!m_fn_SymFromAddr         	) { return false; }
	m_fn_UnDecorateSymbolName	= reinterpret_cast<decltype(&UnDecorateSymbolName	)>(get_function_address(m_dbghelp_dll, "UnDecorateSymbolName"	)); if(!m_fn_UnDecorateSymbolName	) { return false; }
	return true;
}

bool dbghelp::win10_sdk_hardcoded()
{
	assert(!m_dbghelp_dll);
	#if defined _M_IX86
	static constexpr wchar_t const s_full_path_32os[] = LR"---(C:\Program Files\Windows Kits\10\Debuggers\x86\dbghelp.dll)---";
	static constexpr wchar_t const s_full_path_64os[] = LR"---(C:\Program Files (x86)\Windows Kits\10\Debuggers\x86\dbghelp.dll)---";
	wchar_t const* const full_path = is_wow64() ? s_full_path_64os : s_full_path_32os;
	#elif defined _M_X64
	static constexpr wchar_t const s_full_path[] = LR"---(C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\dbghelp.dll)---";
	wchar_t const* const full_path = s_full_path;
	#endif
	m_dbghelp_dll = load_library(full_path);
	return m_dbghelp_dll != nullptr;
}

bool dbghelp::win10_sdk_program_files()
{
	assert(!m_dbghelp_dll);
	#if defined _M_IX86
	static constexpr wchar_t const s_partial_path[] = LR"---(Windows Kits\10\Debuggers\x86\dbghelp.dll)---";
	static constexpr auto const s_folder_id = GUID{0x905E63B6, 0xC1BF, 0x494E, {0xB2, 0x9C, 0x65, 0xB7, 0x32, 0xD3, 0xD2, 0x1A}}; // FOLDERID_ProgramFiles
	#elif defined _M_X64
	static constexpr wchar_t const s_partial_path[] = LR"---(Windows Kits\10\Debuggers\x64\dbghelp.dll)---";
	static constexpr auto const s_folder_id = GUID{0x7C5A40EF, 0xA0FB, 0x4BFC, {0x87, 0x4A, 0xC0, 0xF2, 0xE0, 0xB9, 0xFA, 0x8E}}; // FOLDERID_ProgramFilesX86
	#endif
	wchar_t* program_files;
	HRESULT const hr = SHGetKnownFolderPath(s_folder_id, KF_FLAG_DEFAULT, nullptr, &program_files);
	assert(hr == S_OK);
	auto const free_program_files = mk::make_scope_exit([&](){ CoTaskMemFree(program_files); });
	std::filesystem::path p(program_files);
	p.append(s_partial_path);
	m_dbghelp_dll = load_library(p.c_str());
	return m_dbghelp_dll != nullptr;
}

bool dbghelp::win10_sdk_registry()
{
	assert(!m_dbghelp_dll);
	#if defined _M_IX86
	static constexpr wchar_t const s_partial_path[] = LR"---(Debuggers\x86\dbghelp.dll)---";
	#elif defined _M_X64
	static constexpr wchar_t const s_partial_path[] = LR"---(Debuggers\x64\dbghelp.dll)---";
	#endif
	static constexpr wchar_t const s_windows_kits_installed_roots[] = LR"---(SOFTWARE\Microsoft\Windows Kits\Installed Roots)---";
	static constexpr wchar_t const s_windows_10_sdk[] = L"KitsRoot10";
	HKEY installed_roots;
	LSTATUS const opened = RegOpenKeyExW(HKEY_LOCAL_MACHINE, s_windows_kits_installed_roots, 0, KEY_QUERY_VALUE, &installed_roots);
	if(opened != ERROR_SUCCESS){ return false; }
	auto const installed_roots_sp = make_smart_reg_key(installed_roots);
	DWORD type;
	std::array<wchar_t, 32 * 1024> buff;
	DWORD size = static_cast<int>(buff.size() * sizeof(wchar_t));
	LSTATUS const read = RegGetValueW(installed_roots, L"", s_windows_10_sdk, RRF_RT_REG_SZ, &type, buff.data(), &size);
	if(read != ERROR_SUCCESS){ return false; }
	assert(size <= buff.size() * sizeof(wchar_t));
	assert(size >= 2 * sizeof(wchar_t));
	assert(size % sizeof(wchar_t) == 0);
	assert(buff[(size / sizeof(wchar_t)) - 1] == L'\0');
	std::filesystem::path p(buff.data(), buff.data() + (size / sizeof(wchar_t)) - 1);
	p.append(s_partial_path);
	m_dbghelp_dll = load_library(p.c_str());
	return m_dbghelp_dll != nullptr;
}

bool dbghelp::system_default()
{
	assert(!m_dbghelp_dll);
	m_dbghelp_dll = load_library(L"dbghelp.dll");
	return m_dbghelp_dll != nullptr;
}
