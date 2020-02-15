#include "my_actctx.h"

#include "cassert_my.h"

#include "../res/resources.h"

#include "my_windows.h"


static HANDLE g_my_actctx_h = nullptr;
static ULONG_PTR g_my_actctx_cookie = 0;


void my_actctx::create()
{
	assert(g_my_actctx_h == nullptr);
	ACTCTXW actctx_request{};
	actctx_request.cbSize = sizeof(actctx_request);
	actctx_request.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID | ACTCTX_FLAG_HMODULE_VALID;
	actctx_request.lpSource = nullptr;
	actctx_request.wProcessorArchitecture = 0;
	actctx_request.wLangId = 0;
	actctx_request.lpAssemblyDirectory = nullptr;
	actctx_request.lpResourceName = MAKEINTRESOURCEW(s_res_dependency_manifest);
	actctx_request.lpApplicationName = nullptr;
	actctx_request.hModule = GetModuleHandleW(nullptr);
	HANDLE const actctx_handle = CreateActCtxW(&actctx_request);
	assert(actctx_handle != INVALID_HANDLE_VALUE);
	g_my_actctx_h = actctx_handle;
}

void my_actctx::destroy()
{
	assert(g_my_actctx_h != nullptr);
	ReleaseActCtx(g_my_actctx_h);
	g_my_actctx_h = nullptr;
}

void my_actctx::activate()
{
	assert(g_my_actctx_cookie == 0);
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
