#pragma once


#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>


typedef void* thread_worker_param_t;
typedef void(*thread_worker_function_t)(thread_worker_param_t const);


struct thread_worker_task
{
	thread_worker_function_t m_func;
	thread_worker_param_t m_param;
};


class thread_worker
{
public:
	thread_worker();
	~thread_worker();
public:
	void add_task(thread_worker_function_t const func, thread_worker_param_t const param);
private:
	void thread_func();
private:
	std::thread m_thread;
	std::mutex m_queue_1_mutex;
	std::condition_variable m_condition_variable;
	std::vector<thread_worker_task> m_queue_1;
	std::vector<thread_worker_task> m_queue_2;
	bool m_thread_stop_requested;
};
