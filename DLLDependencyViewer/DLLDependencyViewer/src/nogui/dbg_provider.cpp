#include "dbg_provider.h"

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

void dbg_provider::init_task(dbg_provider_param_t const param)
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

void dbg_provider::deinit_task(dbg_provider_param_t const param)
{
	if(!m_sym_inited)
	{
		return;
	}
	BOOL const cleaned = m_dbghelp.m_fn_SymCleanup(GetCurrentProcess());
	assert(cleaned != FALSE);
	m_dbghelp.m_symsrv_dll.reset();
	m_dbghelp.m_dbghelp_dll.reset();
}
