#include "thread_worker.h"

#include "cassert_my.h"


thread_worker::thread_worker():
	m_thread(),
	m_queue_1_mutex(),
	m_condition_variable(),
	m_queue_1(),
	m_queue_2(),
	m_thread_stop_requested(false)
{
	thread_worker* const self = this;
	m_thread = std::thread([self](){ self->thread_func(); });
}

thread_worker::~thread_worker()
{
	static constexpr auto const request_stop_fn = [](thread_worker_param_t const param)
	{
		assert(param);
		thread_worker& self = *reinterpret_cast<thread_worker*>(param);
		self.m_thread_stop_requested = true;
	};
	thread_worker_function_t const rqst_stp_fn = request_stop_fn;
	thread_worker_param_t const rqst_stp_param = this;

	add_task(rqst_stp_fn, rqst_stp_param);
	m_thread.join();
}

void thread_worker::add_task(thread_worker_function_t const func, thread_worker_param_t const param)
{
	{
		std::lock_guard<std::mutex> lck(m_queue_1_mutex);
		m_queue_1.push_back(thread_worker_task{func, param});
	}
	m_condition_variable.notify_one();
}

void thread_worker::thread_func()
{
	while(!m_thread_stop_requested)
	{
		{
			std::unique_lock<std::mutex> lck(m_queue_1_mutex);
			thread_worker* const self = this;
			m_condition_variable.wait(lck, [self](){ return !self->m_queue_1.empty(); });
			assert(!m_queue_1.empty());
			assert(m_queue_2.empty());
			using std::swap;
			swap(m_queue_1, m_queue_2);
		}
		for(auto const& task : m_queue_2)
		{
			task.m_func(task.m_param);
		}
		m_queue_2.clear();
	}
	assert(m_queue_1.empty());
	assert(m_queue_2.empty());
}
