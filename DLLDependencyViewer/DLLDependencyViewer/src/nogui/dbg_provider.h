#pragma once


#include "dbghelp.h"
#include "pe.h"
#include "thread_worker.h"
#include "unique_strings.h"

#include <atomic>
#include <string>
#include <vector>


class dbg_provider;


typedef void* dbg_provider_param_t;
typedef void(dbg_provider::* dbg_provider_task_t)(dbg_provider_param_t const param);


struct get_symbols_from_addresses_task_t
{
	typedef void(* callback_function_t)(get_symbols_from_addresses_task_t* const);
	std::atomic<bool> m_canceled;
	wstring const* m_module_path = nullptr;
	std::vector<std::uint32_t> m_addresses;
	std::vector<pe_export_address_entry*> m_export_entries;
	std::vector<std::wstring> m_symbol_names;
	std::atomic<callback_function_t> m_callback_function;
	void* m_hwnd;
};


class dbg_provider
{
public:
	dbg_provider();
	~dbg_provider();
	void get_symbols_from_addresses(get_symbols_from_addresses_task_t* const param);
private:
	void add_task(dbg_provider_task_t const task, dbg_provider_param_t const param);
	void init_task(dbg_provider_param_t const param);
	void deinit_task(dbg_provider_param_t const param);
	void get_symbols_from_addresses_task(dbg_provider_param_t const param);
private:
	bool m_sym_inited;
	dbghelp m_dbghelp;
	thread_worker m_thread_worker;
};
