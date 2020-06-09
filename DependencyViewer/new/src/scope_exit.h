#pragma once
#ifndef mk_scope_exit_h_included
#define mk_scope_exit_h_included


#include <cassert> // assert
#include <memory> // std::addressof
#include <type_traits> // std::remove_reference_t
#include <utility> // std::forward std::move std::swap


namespace mk
{


	template<typename T>
	class scope_exit
	{
	public:
		scope_exit() noexcept :
			m_func(),
			m_empty(true)
		{
		}
		scope_exit(T const& func) :
			m_func(func),
			m_empty(false)
		{
		}
		scope_exit(T&& func) :
			m_func(std::move(func)),
			m_empty(false)
		{
		}
		scope_exit(scope_exit const&) = delete;
		scope_exit(scope_exit&& other) noexcept :
			scope_exit()
		{
			*this = std::move(other);
		}
		scope_exit& operator=(scope_exit const&) = delete;
		scope_exit& operator=(scope_exit&& other) noexcept
		{
			assert(this != std::addressof(other));
			swap(other);
			return *this;
		}
		~scope_exit()
		{
			if(!m_empty)
			{
				m_func();
			}
		}
		void swap(scope_exit& other) noexcept
		{
			using std::swap;
			swap(this->m_func, other.m_func);
			swap(this->m_empty, other.m_empty);
		}
		void execute()
		{
			assert(!m_empty);
			m_empty = true;
			m_func();
		}
		void reset()
		{
			m_empty = true;
		}
		std::remove_reference_t<T> release()
		{
			m_empty = true;
			return m_func;
		}
	private:
		std::remove_reference_t<T> m_func;
		bool m_empty;
	};

	template<typename T>
	void swap(scope_exit<T>& a, scope_exit<T>& b) noexcept { a.swap(b); }

	template<typename T>
	scope_exit<T> make_scope_exit(T&& func)
	{
		return scope_exit<T>(std::forward<T>(func));
	}


}


#endif
