#include "com_ptr.h"

#include "cassert_my.h"

#include <utility>


template<typename T>
com_ptr<T>::com_ptr() noexcept :
	m_ptr()
{
}

template<typename T>
com_ptr<T>::com_ptr(T* const& ptr) :
	m_ptr(ptr)
{
}

template<typename T>
com_ptr<T>::com_ptr(com_ptr<T> const& other) :
	m_ptr(other.m_ptr)
{
	add_ref();
}

template<typename T>
com_ptr<T>::com_ptr(com_ptr<T>&& other) noexcept :
	com_ptr(other.m_ptr)
{
	other.m_ptr = nullptr;
}

template<typename T>
com_ptr<T>& com_ptr<T>::operator=(com_ptr<T> const& other)
{
	assert(this != &other);
	dec_ref();
	m_ptr = other.m_ptr;
	add_ref();
	return *this;
}

template<typename T>
com_ptr<T>& com_ptr<T>::operator=(com_ptr<T>&& other) noexcept
{
	assert(this != &other);
	dec_ref();
	m_ptr = other.m_ptr;
	other.m_ptr = nullptr;
	return *this;
}

template<typename T>
com_ptr<T>::~com_ptr()
{
	dec_ref();
}

template<typename T>
void com_ptr<T>::swap(com_ptr<T>& other) noexcept
{
	using std::swap;
	swap(m_ptr, other.m_ptr);
}

template<typename T>
void com_ptr<T>::reset()
{
	dec_ref();
	m_ptr = nullptr;
}

template<typename T>
void com_ptr<T>::reset(T* const ptr)
{
	dec_ref();
	m_ptr = ptr;
}

template<typename T>
com_ptr<T>::operator bool() const
{
	return m_ptr != nullptr;
}

template<typename T>
bool com_ptr<T>::operator!() const
{
	return m_ptr == nullptr;
}

template<typename T>
T* com_ptr<T>::get() const
{
	return m_ptr;
}

template<typename T>
T* com_ptr<T>::operator->() const
{
	return m_ptr;
}

template<typename T>
com_ptr<T>::operator T*() const
{
	return m_ptr;
}

template<typename T>
com_ptr<IUnknown> com_ptr<T>::to_iunknown() const
{
	return query_cast<IUnknown>(IID_IUnknown);
}

template<typename T>
com_ptr<T>::operator com_ptr<IUnknown>() const
{
	return query_cast<IUnknown>(IID_IUnknown);
}

template<typename T>
template<typename U>
com_ptr<U> com_ptr<T>::query_cast(IID const& iid) const
{
	if(!m_ptr)
	{
		return com_ptr<U>{};
	}
	else
	{
		U* ret = nullptr;
		HRESULT const queried = m_ptr->lpVtbl->QueryInterface(m_ptr, iid, reinterpret_cast<void**>(&ret));
		if(queried == S_OK)
		{
			assert(ret);
			return com_ptr<U>{ret};
		}
		else
		{
			return com_ptr<U>{};
		}
	}
}

template<typename T>
void com_ptr<T>::add_ref()
{
	if(m_ptr)
	{
		ULONG const new_ref_cnt = m_ptr->lpVtbl->AddRef(m_ptr);
		assert(new_ref_cnt >= 2);
	}
}

template<typename T>
void com_ptr<T>::dec_ref()
{
	if(m_ptr)
	{
		ULONG const new_ref_cnt = m_ptr->lpVtbl->Release(m_ptr);
		assert(new_ref_cnt >= 0);
	}
}
