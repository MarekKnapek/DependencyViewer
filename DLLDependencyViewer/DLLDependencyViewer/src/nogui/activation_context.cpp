#include "activation_context.h"

#include "file_system.h"

#include <cstdint>
#include <array>
#include <vector>

#include <intrin.h>

#pragma warning(push)
#pragma warning(disable:4201)
#include <phnt_windows.h>
#include <phnt.h>
#pragma warning(pop)

#include <sxstypes.h>


static manifests_t* g_system_default_manifests = nullptr;


const manifests_t& activation_context::get_system_default_manifests()
{
	if(g_system_default_manifests)
	{
		return *g_system_default_manifests;
	}
	g_system_default_manifests = new manifests_t;

	ACTIVATION_CONTEXT_DATA const* const activation_context_data = reinterpret_cast<ACTIVATION_CONTEXT_DATA const*>(get_system_default_activation_context_data());
	ACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER const* const assembly_header = reinterpret_cast<ACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER const*>(reinterpret_cast<char const*>(activation_context_data) + activation_context_data->AssemblyRosterOffset);
	ACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_ENTRY const* const assembly_entries = reinterpret_cast<ACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_ENTRY const*>(reinterpret_cast<char const*>(activation_context_data) + assembly_header->FirstEntryOffset);
	void const* const assembly_information_section = reinterpret_cast<char const*>(activation_context_data) + assembly_header->AssemblyInformationSectionOffset;
	g_system_default_manifests->reserve(assembly_header->EntryCount - 1);
	for(ULONG i = 1; i != assembly_header->EntryCount; ++i)
	{
		if((assembly_entries[i].Flags & ACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_ENTRY_INVALID) != 0)
		{
			continue;
		}
		ACTIVATION_CONTEXT_DATA_ASSEMBLY_INFORMATION const* const assembly_info = reinterpret_cast<ACTIVATION_CONTEXT_DATA_ASSEMBLY_INFORMATION const*>(reinterpret_cast<char const*>(activation_context_data) + assembly_entries[i].AssemblyInformationOffset);
		if(assembly_info->ManifestPathOffset == 0)
		{
			continue;
		}
		wchar_t const* const path = reinterpret_cast<wchar_t const*>(reinterpret_cast<char const*>(assembly_information_section) + assembly_info->ManifestPathOffset);
		if(!file_exists(path))
		{
			continue;
		}
		g_system_default_manifests->emplace_back(path);
	}

	return *g_system_default_manifests;
}

void activation_context::free_system_default_manifests()
{
	if(g_system_default_manifests)
	{
		delete g_system_default_manifests;
		g_system_default_manifests = nullptr;
	}
}

void const* activation_context::get_system_default_activation_context_data()
{
#if defined _M_IX86
	static_assert(sizeof(decltype(__readfsdword(0))) == sizeof(void*), "");
	auto const tib_auto = __readfsdword(static_cast<unsigned long>(reinterpret_cast<std::uintptr_t>(&static_cast<NT_TIB*>(nullptr)->Self)));
#elif defined _M_X64
	static_assert(sizeof(decltype(__readgsqword(0))) == sizeof(void*), "");
	auto const tib_auto = __readgsqword(static_cast<unsigned long>(reinterpret_cast<std::uintptr_t>(&static_cast<NT_TIB*>(nullptr)->Self)));
#else
#error Unknown architecture.
#endif
	NT_TIB const* const tib = reinterpret_cast<NT_TIB const*>(tib_auto);
	TEB const* const teb = reinterpret_cast<TEB const*>(tib);
	PEB const* const peb = teb->ProcessEnvironmentBlock;
	return peb->SystemDefaultActivationContextData;
}
