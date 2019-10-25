#include "dbg_provider.h"

#include "scope_exit.h"

#include <cassert>


static dbg_provider* g_dbg_provider = nullptr;


void dbg_provider::init()
{
	assert(!g_dbg_provider);
	g_dbg_provider = new dbg_provider();
}

void dbg_provider::deinit()
{
	assert(g_dbg_provider);
	delete g_dbg_provider;
	g_dbg_provider = nullptr;
}

dbg_provider* dbg_provider::get()
{
	assert(g_dbg_provider);;
	return g_dbg_provider;
}

dbg_provider::dbg_provider():
	m_sym_inited(false),
	m_dbghelp(),
	m_thread_worker()
{
	auto const init_task = [](thread_worker_param_t const param)
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
	auto const deinit_task = [](thread_worker_param_t const param)
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

void dbg_provider::get_symbols_from_addresses_task(get_symbols_from_addresses_param_t& param)
{
	if(!m_sym_inited)
	{
		return;
	}
	DWORD64 const sym_module = m_dbghelp.m_fn_SymLoadModuleExW(GetCurrentProcess(), nullptr, param.m_module_path->m_str, nullptr, 0, 0, nullptr, 0);
	if(sym_module == 0)
	{
		return;
	}
	auto const fn_unload_module = mk::make_scope_exit([&]()
	{
		BOOL const unloaded = m_dbghelp.m_fn_SymUnloadModule64(GetCurrentProcess(), sym_module);
		assert(unloaded != FALSE);
	});
	assert(param.m_indexes.size() == param.m_symbol_names.size());
	std::uint16_t const n = static_cast<std::uint16_t>(param.m_indexes.size());
	for(std::uint16_t i = 0; i != n; ++i)
	{
		DWORD64 displacement;
		union symbol_info_t
		{
			SYMBOL_INFOW sym_info;
			char buff[sizeof(SYMBOL_INFOW) + MAX_SYM_NAME * sizeof(wchar_t)];
		};
		symbol_info_t symbol_info_v;
		SYMBOL_INFOW& symbol_info = symbol_info_v.sym_info;
		symbol_info.SizeOfStruct = sizeof(SYMBOL_INFOW);
		symbol_info.MaxNameLen = MAX_SYM_NAME;
		std::uint32_t const address = param.m_eti->m_rvas_or_forwarders[param.m_indexes[i]].m_rva;
		DWORD64 const address64 = sym_module + address;
		BOOL const got_sym = m_dbghelp.m_fn_SymFromAddrW(GetCurrentProcess(), address64, &displacement, &symbol_info);
		if(got_sym != FALSE)
		{
			param.m_symbol_names[i].assign(symbol_info.Name, symbol_info.Name + symbol_info.NameLen);
		}
		else
		{
			param.m_symbol_names[i].clear();
		}
	}
}

void dbg_provider::init_task()
{
	bool const dbghelp_inited = m_dbghelp.init();
	if(!dbghelp_inited)
	{
		return;
	}
	DWORD const sym_options = m_dbghelp.m_fn_SymGetOptions();
	DWORD const set = m_dbghelp.m_fn_SymSetOptions((sym_options | SYMOPT_DEFERRED_LOADS | SYMOPT_FAIL_CRITICAL_ERRORS) &~ SYMOPT_UNDNAME);
	BOOL const inited = m_dbghelp.m_fn_SymInitializeW(GetCurrentProcess(), nullptr, FALSE);
	if(inited == FALSE)
	{
		return;
	}
	m_sym_inited = true;
}

void dbg_provider::deinit_task()
{
	if(!m_sym_inited)
	{
		return;
	}
	BOOL const cleaned = m_dbghelp.m_fn_SymCleanup(GetCurrentProcess());
	assert(cleaned != FALSE);
	m_dbghelp.m_dbghelp_dll.reset();
}
