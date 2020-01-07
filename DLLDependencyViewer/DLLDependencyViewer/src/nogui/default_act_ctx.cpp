#include "default_act_ctx.h"

#include "assert.h"
#include "default_manifests.h"
#include "manifest_parser2.h"
#include "memory_mapped_file.h"
#include "unicode.h"

#include <algorithm>
#include <cassert>


static act_ctx_t* g_system_default_act_ctx = nullptr;


void default_act_ctx::init()
{
	if(g_system_default_act_ctx)
	{
		return;
	}
	act_ctx_t act_ctx;
	auto const& manifests = default_manifests::get();
	int const n = static_cast<int>(manifests.size());
	act_ctx.resize(n);
	files_t files;
	for(int i = 0; i != n; ++i)
	{
		auto const& manifest = manifests[i];
		memory_mapped_file const mmf{manifest.c_str()};
		WARN_M_RV(mmf.begin() != nullptr, L"Failed to memory map file.");
		files.clear();
		bool const parsed = parse_files(mmf.begin(), mmf.size(), files);
		WARN_M_RV(parsed, L"Failed to parse_files.");
		WARN_M_RV(std::all_of(files.begin(), files.end(), [](auto const& e){ return is_ascii(e.c_str(), static_cast<int>(e.size())); }), L"DLLs shall have ASCII names.");
		auto& target_vec = act_ctx[i];
		target_vec.resize(files.size());
		std::transform(files.begin(), files.end(), target_vec.begin(), [](auto const& e)
		{
			std::string ret;
			ret.resize(e.size());
			std::transform(e.begin(), e.end(), ret.begin(), [](auto const& ch){ return to_lowercase(static_cast<char>(ch)); });
			return ret;
		});
		std::sort(target_vec.begin(), target_vec.end());
	}
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
