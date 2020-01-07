#include "default_act_ctx.h"

#include <cassert>


static act_ctx_t* g_system_default_act_ctx = nullptr;


void default_act_ctx::init()
{
	if(g_system_default_act_ctx)
	{
		return;
	}
	act_ctx_t act_ctx;
	g_system_default_act_ctx = new act_ctx_t(std::move(act_ctx));
}

void default_act_ctx::deinit()
{
	if(g_system_default_act_ctx)
	{
		delete g_system_default_act_ctx;
		g_system_default_act_ctx = nullptr;
	}
}

act_ctx_t const& default_act_ctx::get()
{
	init();
	assert(g_system_default_act_ctx);
	return *g_system_default_act_ctx;
}
