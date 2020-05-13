#include "act_ctx.h"

#include "assert_my.h"
#include "cassert_my.h"
#include "utils.h"

#include <array>
#include <cstring>

#include "my_windows.h"


#define s_very_big_int (2'147'483'647)


bool create_actctx2(wstring_handle const exe, wstring_handle const dll, std::uint32_t const manifest_id, actctx_state_t* const actctx_state_out);


bool create_actctx(wstring_handle const exe, wstring_handle const dll, std::uint32_t const manifest_id, actctx_state_t* const actctx_state_out)
{
	if(manifest_id == 0)
	{
		actctx_state_out->m_actctx_handle = nullptr;
		actctx_state_out->m_cookie = 0;
		return true;
	}
	else
	{
		return create_actctx2(exe, dll, manifest_id, actctx_state_out);
	}
}

void destroy_actctx(actctx_state_t const& actctx_state)
{
	if(actctx_state.m_actctx_handle == nullptr && actctx_state.m_cookie == 0)
	{
		return;
	}
	else
	{
		BOOL const deactivated = DeactivateActCtx(0, actctx_state.m_cookie);
		assert(deactivated != FALSE);
		ReleaseActCtx(actctx_state.m_actctx_handle);
		return;
	}
}


bool create_actctx2(wstring_handle const exe, wstring_handle const dll, std::uint32_t const manifest_id, actctx_state_t* const actctx_state_out)
{
	assert(actctx_state_out);
	auto const b = begin(exe);
	auto const e = end(exe);
	auto const it = rfind(b, e, L'\\');
	WARN_M_R(it != e, L"Could not find directory.", false);
	auto const diff_ = it - b + 1;
	assert(diff_ <= s_very_big_int);
	int const diff = static_cast<int>(diff_);
	std::array<wchar_t, 32 * 1024> buff;
	assert(diff < static_cast<int>(buff.size()));
	std::memcpy(buff.data(), b, diff * sizeof(wchar_t));
	buff[diff] = L'\0';
	ACTCTXW actctx_request{};
	actctx_request.cbSize = sizeof(actctx_request);
	actctx_request.dwFlags = ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID | ACTCTX_FLAG_RESOURCE_NAME_VALID | ACTCTX_FLAG_APPLICATION_NAME_VALID;
	actctx_request.lpSource = dll.m_string->m_str;
	actctx_request.wProcessorArchitecture = 0;
	actctx_request.wLangId = 0;
	actctx_request.lpAssemblyDirectory = buff.data();
	actctx_request.lpResourceName = MAKEINTRESOURCEW(manifest_id);
	actctx_request.lpApplicationName = exe.m_string->m_str;
	actctx_request.hModule = nullptr;
	HANDLE const actctx_handle = CreateActCtxW(&actctx_request);
	if(actctx_handle == INVALID_HANDLE_VALUE)
	{
		DWORD const gle = GetLastError();
		if(gle == ERROR_SXS_CANT_GEN_ACTCTX)
		{
			actctx_state_out->m_actctx_handle = nullptr;
			actctx_state_out->m_cookie = 0;
			return true;
		}
	}
	WARN_M_R(actctx_handle != INVALID_HANDLE_VALUE, L"Failed to CreateActCtxW.", false);
	ULONG_PTR cookie;
	BOOL const activated = ActivateActCtx(actctx_handle, &cookie);
	WARN_M_R(activated != FALSE, L"Failed to ActivateActCtx.", false); // Leak.
	actctx_state_out->m_actctx_handle = actctx_handle;
	actctx_state_out->m_cookie = cookie;
	return true;
}
