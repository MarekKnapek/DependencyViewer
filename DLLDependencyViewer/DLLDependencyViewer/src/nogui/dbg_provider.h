#pragma once


#include "dbghelp.h"
#include "thread_worker.h"


class dbg_provider;


typedef void* dbg_provider_param_t;
typedef void(dbg_provider::* dbg_provider_task_t)(dbg_provider_param_t const param);


class dbg_provider
{
public:
	dbg_provider();
	~dbg_provider();
private:
	void add_task(dbg_provider_task_t const task, dbg_provider_param_t const param);
	void init_task(dbg_provider_param_t const param);
	void deinit_task(dbg_provider_param_t const param);
private:
	bool m_sym_inited;
	dbghelp m_dbghelp;
	thread_worker m_thread_worker;
};
