#pragma once


#include <condition_variable>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>


typedef void* thread_worker_param_t;
typedef void(* thread_worker_task_t)(thread_worker_param_t const param);


class thread_worker
{
public:
	thread_worker();
	~thread_worker();
public:
	void add_task(thread_worker_task_t const task, thread_worker_param_t const param);
private:
	void thread_func();
private:
	std::thread m_thread;
	std::mutex m_queue_1_mutex;
	std::condition_variable m_condition_variable;
	std::vector<std::pair<thread_worker_task_t, thread_worker_param_t>> m_queue_1;
	std::vector<std::pair<thread_worker_task_t, thread_worker_param_t>> m_queue_2;
	bool m_thread_stop_requested;
};
