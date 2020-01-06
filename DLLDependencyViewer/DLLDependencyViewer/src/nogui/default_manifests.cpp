#include "default_manifests.h"

#include <cassert>
#include <cstdint>
#include <filesystem>

#include "my_windows.h"

#pragma warning(push)
#pragma warning(disable:4201) // warning C4201: nonstandard extension used: nameless struct/union
#pragma warning(disable:4324) // warning C4324: 'xxx': structure was padded due to alignment specifier
#pragma warning(disable:4471) // warning C4471: 'xxx': a forward declaration of an unscoped enumeration must have an underlying type
#include <phnt.h>
#pragma warning(pop)

#include <sxstypes.h>


static manifests_t* g_system_default_manifests = nullptr;


void default_manifests::init()
{
	if(g_system_default_manifests)
	{
		return;
	}
	manifests_t manifests;
	auto const tib_offset = static_cast<unsigned long>(reinterpret_cast<std::uintptr_t>(&(static_cast<NT_TIB*>(nullptr)->Self)));
	#if defined _M_IX86
	static_assert(sizeof(decltype(__readfsdword(0))) == sizeof(void*), "");
	auto const tib_auto = __readfsdword(tib_offset);
	#elif defined _M_X64
	static_assert(sizeof(decltype(__readgsqword(0))) == sizeof(void*), "");
	auto const tib_auto = __readgsqword(tib_offset);
	#else
	#error Unknown architecture.
	#endif
	NT_TIB const* const tib = reinterpret_cast<NT_TIB const*>(tib_auto);
	TEB const* const teb = reinterpret_cast<TEB const*>(tib);
	PEB const* const peb = teb->ProcessEnvironmentBlock;
	ACTIVATION_CONTEXT_DATA const* const system_default_activation_context_data = reinterpret_cast<ACTIVATION_CONTEXT_DATA const*>(peb->SystemDefaultActivationContextData);
	ACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER const* const assembly_roster = reinterpret_cast<ACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER const*>(reinterpret_cast<char const*>(system_default_activation_context_data) + system_default_activation_context_data->AssemblyRosterOffset);
	ACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_ENTRY const* const assembly_entries = reinterpret_cast<ACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_ENTRY const*>(reinterpret_cast<char const*>(system_default_activation_context_data) + assembly_roster->FirstEntryOffset);
	void const* const assembly_information_section = reinterpret_cast<char const*>(system_default_activation_context_data) + assembly_roster->AssemblyInformationSectionOffset;
	manifests.reserve(assembly_roster->EntryCount - 1);
	for(ULONG i = 1; i != assembly_roster->EntryCount; ++i)
	{
		if((assembly_entries[i].Flags & ACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_ENTRY_INVALID) != 0)
		{
			continue;
		}
		ACTIVATION_CONTEXT_DATA_ASSEMBLY_INFORMATION const* const assembly_info = reinterpret_cast<ACTIVATION_CONTEXT_DATA_ASSEMBLY_INFORMATION const*>(reinterpret_cast<char const*>(system_default_activation_context_data) + assembly_entries[i].AssemblyInformationOffset);
		if(assembly_info->ManifestPathOffset == 0)
		{
			continue;
		}
		wchar_t const* const path = reinterpret_cast<wchar_t const*>(reinterpret_cast<char const*>(assembly_information_section) + assembly_info->ManifestPathOffset);
		assert(assembly_info->ManifestPathLength % 2 == 0);
		int const path_len = assembly_info->ManifestPathLength / sizeof(wchar_t);
		if(!std::filesystem::exists(path))
		{
			continue;
		}
		manifests.emplace_back(path, path + path_len);
	}
	g_system_default_manifests = new manifests_t(std::move(manifests));
}

void default_manifests::deinit()
{
	if(g_system_default_manifests)
	{
		delete g_system_default_manifests;
		g_system_default_manifests = nullptr;
	}
}

manifests_t const& default_manifests::get()
{
	init();
	assert(g_system_default_manifests);
	return *g_system_default_manifests;
}
