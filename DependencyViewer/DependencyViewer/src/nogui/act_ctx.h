#pragma once


#include "my_string_handle.h"

#include "my_windows.h"


struct actctx_state_t
{
	HANDLE m_actctx_handle;
	ULONG_PTR m_cookie;
};


bool create_actctx(wstring_handle const exe, wstring_handle const dll, std::uint32_t const manifest_id, actctx_state_t* const actctx_state_out);
void destroy_actctx(actctx_state_t const& actctx_state);
