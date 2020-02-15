#include "dbg_provider.h"

#include "cassert_my.h"
#include "scope_exit.h"
#include "thread_name.h"

#include <array>


static dbg_provider* g_dbg_provider = nullptr;


void dbg_provider::deinit()
{
	delete g_dbg_provider;
	g_dbg_provider = nullptr;
}

dbg_provider* dbg_provider::get()
{
	if(!g_dbg_provider)
	{
		g_dbg_provider = new dbg_provider();
	}
	return g_dbg_provider;
}

dbg_provider::dbg_provider() :
	m_sym_inited(false),
	m_dbghelp(),
	m_thread_worker()
{
	static constexpr auto const init_task = [](thread_worker_param_t const param)
	{
		assert(param);
		dbg_provider& self = *static_cast<dbg_provider*>(param);
		self.init_task();
	};
	thread_worker_function_t const init_tsk = init_task;
	thread_worker_param_t const init_tsk_param = this;

	add_task(init_tsk, init_tsk_param);
}

dbg_provider::~dbg_provider()
{
	static constexpr auto const deinit_task = [](thread_worker_param_t const param)
	{
		assert(param);
		dbg_provider& self = *static_cast<dbg_provider*>(param);
		self.deinit_task();
	};
	thread_worker_function_t const deinit_tsk = deinit_task;
	thread_worker_param_t const deinit_tsk_param = this;

	add_task(deinit_tsk, deinit_tsk_param);
}

void dbg_provider::add_task(thread_worker_function_t const func, thread_worker_param_t const param)
{
	m_thread_worker.add_task(func, param);
}

void dbg_provider::get_symbols_from_addresses_task(symbols_from_addresses_param_t& param)
{
	if(!m_sym_inited)
	{
		return;
	}
	DWORD64 const sym_module = m_dbghelp.m_fn_SymLoadModuleExW(GetCurrentProcess(), nullptr, cbegin(param.m_module_path), nullptr, 0, 0, nullptr, 0);
	if(sym_module == 0)
	{
		return;
	}
	auto const fn_unload_module = mk::make_scope_exit([&]()
	{
		BOOL const unloaded = m_dbghelp.m_fn_SymUnloadModule64(GetCurrentProcess(), sym_module);
		assert(unloaded != FALSE);
	});
	assert(param.m_indexes.size() == param.m_strings.size());
	std::uint16_t const n = static_cast<std::uint16_t>(param.m_indexes.size());
	for(std::uint16_t i = 0; i != n; ++i)
	{
		DWORD64 displacement;
		union symbol_info_t
		{
			SYMBOL_INFO sym_info;
			char buff[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(char)];
		};
		symbol_info_t symbol_info_v{};
		SYMBOL_INFO& symbol_info = symbol_info_v.sym_info;
		symbol_info.SizeOfStruct = sizeof(symbol_info);
		symbol_info.MaxNameLen = MAX_SYM_NAME;
		std::uint32_t const& address = param.m_eti->m_rvas_or_forwarders[param.m_indexes[i]].m_rva;
		DWORD64 const address64 = sym_module + address;
		BOOL const got_sym = m_dbghelp.m_fn_SymFromAddr(GetCurrentProcess(), address64, &displacement, &symbol_info);
		if(got_sym != FALSE)
		{
			param.m_strings[i].assign(symbol_info.Name, symbol_info.Name + symbol_info.NameLen);
		}
	}
}

void dbg_provider::get_undecorated_from_decorated_e_task(undecorated_from_decorated_e_param_t& param)
{
	if(!m_sym_inited)
	{
		return;
	}
	assert(param.m_indexes.size() == param.m_strings.size());
	std::uint16_t const n = static_cast<std::uint16_t>(param.m_indexes.size());
	for(std::uint16_t i = 0; i != n; ++i)
	{
		std::uint16_t const idx = param.m_indexes[i];
		string_handle const& name = param.m_eti->m_names[idx];
		std::array<char, 8 * 1024> buff;
		DWORD const undecorated = m_dbghelp.m_fn_UnDecorateSymbolName(cbegin(name), buff.data(), static_cast<int>(buff.size()), UNDNAME_COMPLETE);
		if(undecorated != 0)
		{
			param.m_strings[i].assign(buff.data(), buff.data() + undecorated);
		}
	}
}

void dbg_provider::get_undecorated_from_decorated_i_task(undecorated_from_decorated_i_param_t& param)
{
	if(!m_sym_inited)
	{
		return;
	}
	assert(param.m_indexes.size() == param.m_strings.size());
	std::uint16_t const n = static_cast<std::uint16_t>(param.m_indexes.size());
	for(std::uint16_t i = 0; i != n; ++i)
	{
		std::uint16_t const idx = param.m_indexes[i];
		string_handle const& name = param.m_iti->m_names[param.m_dll_idx][idx];
		std::array<char, 8 * 1024> buff;
		DWORD const undecorated = m_dbghelp.m_fn_UnDecorateSymbolName(cbegin(name), buff.data(), static_cast<int>(buff.size()), UNDNAME_COMPLETE);
		if(undecorated != 0)
		{
			param.m_strings[i].assign(buff.data(), buff.data() + undecorated);
		}
	}
}

void dbg_provider::init_task()
{
	name_current_thread("dbg_provider", L"dbg_provider");
	bool const dbghelp_inited = m_dbghelp.init();
	if(!dbghelp_inited)
	{
		return;
	}
	DWORD const sym_options = m_dbghelp.m_fn_SymGetOptions();
	DWORD const set = m_dbghelp.m_fn_SymSetOptions((sym_options | SYMOPT_DEFERRED_LOADS | SYMOPT_FAIL_CRITICAL_ERRORS | SYMOPT_PUBLICS_ONLY) &~ SYMOPT_UNDNAME);
	BOOL const inited = m_dbghelp.m_fn_SymInitializeW(GetCurrentProcess(), nullptr, FALSE);
	if(inited == FALSE)
	{
		return;
	}
	m_sym_inited = true;
}

void dbg_provider::deinit_task()
{
	if(m_sym_inited)
	{
		BOOL const cleaned = m_dbghelp.m_fn_SymCleanup(GetCurrentProcess());
		assert(cleaned != FALSE);
	}
	m_dbghelp.deinit();
}
