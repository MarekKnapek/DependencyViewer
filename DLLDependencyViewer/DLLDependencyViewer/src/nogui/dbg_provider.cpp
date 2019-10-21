#include "dbg_provider.h"

#include "scope_exit.h"

#include <cassert>
#include <memory>


struct dbg_provider_data_marshaller
{
	dbg_provider* m_self;
	dbg_provider_task_t m_task;
	dbg_provider_param_t m_param;
};


dbg_provider::dbg_provider():
	m_sym_inited(false),
	m_dbghelp(),
	m_thread_worker()
{
	add_task(&dbg_provider::init_task, nullptr);
}

dbg_provider::~dbg_provider()
{
	add_task(&dbg_provider::deinit_task, nullptr);
}

void dbg_provider::get_symbols_from_addresses(get_symbols_from_addresses_task_t* const param)
{
	add_task(&dbg_provider::get_symbols_from_addresses_task, param);
}

void dbg_provider::add_task(dbg_provider_task_t const task, dbg_provider_param_t const param)
{
	dbg_provider_data_marshaller* const m = new dbg_provider_data_marshaller;
	m->m_self = this;
	m->m_task = task;
	m->m_param = param;
	thread_worker_param_t const thread_worker_param = m;
	m_thread_worker.add_task([](thread_worker_param_t const param)
	{
		assert(param);
		dbg_provider_data_marshaller* const m = reinterpret_cast<dbg_provider_data_marshaller*>(param);
		std::unique_ptr<dbg_provider_data_marshaller> const sp_m(m);
		(m->m_self->*m->m_task)(m->m_param);
	}, thread_worker_param);
}

void dbg_provider::init_task(dbg_provider_param_t const /*param*/)
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

void dbg_provider::deinit_task(dbg_provider_param_t const /*param*/)
{
	if(!m_sym_inited)
	{
		return;
	}
	BOOL const cleaned = m_dbghelp.m_fn_SymCleanup(GetCurrentProcess());
	assert(cleaned != FALSE);
	m_dbghelp.m_dbghelp_dll.reset();
}

void dbg_provider::get_symbols_from_addresses_task(dbg_provider_param_t const param)
{
	assert(param);
	get_symbols_from_addresses_task_t* const task = reinterpret_cast<get_symbols_from_addresses_task_t*>(param);
	auto const fn_call_callback = mk::make_scope_exit([&]()
	{
		auto const callback = task->m_callback_function.load();
		(*callback)(task);
	});
	if(task->m_canceled.load() == true)
	{
		return;
	}
	if(!m_sym_inited)
	{
		return;
	}
	DWORD64 const sym_module = m_dbghelp.m_fn_SymLoadModuleExW(GetCurrentProcess(), nullptr, task->m_module_path.c_str(), nullptr, 0, 0, nullptr, 0);
	if(sym_module == 0)
	{
		return;
	}
	auto const fn_unload_module = mk::make_scope_exit([&]()
	{
		BOOL const unloaded = m_dbghelp.m_fn_SymUnloadModule64(GetCurrentProcess(), sym_module);
		assert(unloaded != FALSE);
	});
	assert(task->m_indexes.size() == task->m_symbol_names.size());
	std::uint16_t const n = static_cast<std::uint16_t>(task->m_indexes.size());
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
		std::uint32_t const address = task->m_eti->m_rvas_or_forwarders[task->m_indexes[i]].m_rva;
		DWORD64 const address64 = sym_module + address;
		BOOL const got_sym = m_dbghelp.m_fn_SymFromAddrW(GetCurrentProcess(), address64, &displacement, &symbol_info);
		if(got_sym != FALSE)
		{
			task->m_symbol_names[i].assign(symbol_info.Name, symbol_info.Name + symbol_info.NameLen);
		}
		else
		{
			task->m_symbol_names[i].clear();
		}
	}
}
