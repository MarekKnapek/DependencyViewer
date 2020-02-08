#pragma once


#include "my_windows.h"

#include <guiddef.h>
#include <unknwn.h>


template<typename T>
class com_ptr
{
public:
	com_ptr() noexcept;
	explicit com_ptr(T* const& ptr);
	com_ptr(com_ptr<T> const& other);
	com_ptr(com_ptr<T>&& other) noexcept;
	com_ptr<T>& operator=(com_ptr<T> const& other);
	com_ptr<T>& operator=(com_ptr<T>&& other) noexcept;
	~com_ptr();
	void swap(com_ptr<T>& other) noexcept;
public:
	void reset();
	void reset(T* const ptr);
	explicit operator bool() const;
	bool operator!() const;
	T* get() const;
	T* operator->() const;
	operator T*() const;
	com_ptr<IUnknown> to_iunknown() const;
	operator com_ptr<IUnknown>() const;
	template<typename U> com_ptr<U> query_cast(IID const& iid) const;
private:
	void add_ref();
	void dec_ref();
private:
	T* m_ptr;
};

template<typename T> inline void swap(com_ptr<T>& a, com_ptr<T>& b) noexcept { a.swap(b); }


#include "com_ptr.inl"
