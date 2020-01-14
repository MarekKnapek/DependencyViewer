#include "my_actctx.h"

#include <cassert>

#include "my_windows.h"


static HANDLE g_my_actctx_h = nullptr;
static ULONG_PTR g_my_actctx_cookie = 0;


void my_actctx::activate()
{
	assert(g_my_actctx_cookie == 0);
	if(g_my_actctx_h == nullptr)
	{
		ACTIVATION_CONTEXT_BASIC_INFORMATION actctx_basic_info;
		BOOL const queried = QueryActCtxW(QUERY_ACTCTX_FLAG_ACTCTX_IS_HMODULE | QUERY_ACTCTX_FLAG_NO_ADDREF, GetModuleHandleW(nullptr), nullptr, ActivationContextBasicInformation, &actctx_basic_info, sizeof(actctx_basic_info), nullptr);
		assert(queried != FALSE);
		g_my_actctx_h = actctx_basic_info.hActCtx;
	}
	assert(g_my_actctx_h != nullptr);
	BOOL const activated = ActivateActCtx(g_my_actctx_h, &g_my_actctx_cookie);
	assert(activated != FALSE);
}

void my_actctx::deactivate()
{
	assert(g_my_actctx_cookie != 0);
	BOOL const deactivated = DeactivateActCtx(0, g_my_actctx_cookie);
	assert(deactivated != FALSE);
	g_my_actctx_cookie = 0;
}
