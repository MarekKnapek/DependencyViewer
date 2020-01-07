#pragma once


#include <string>
#include <vector>


typedef std::vector<std::vector<std::string>> act_ctx_t;


namespace default_act_ctx
{
	void init();
	void deinit();
	act_ctx_t const& get();
};
