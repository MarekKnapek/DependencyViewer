#pragma once


#include <memory>


struct close_handle_deleter
{
public:
	void operator()(void* const ptr) const;
};
typedef std::unique_ptr<void, close_handle_deleter> smart_handle;
