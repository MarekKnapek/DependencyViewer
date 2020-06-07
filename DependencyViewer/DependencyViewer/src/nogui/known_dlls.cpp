#include "known_dlls.h"

#include "assert_my.h"
#include "cassert_my.h"
#include "scope_exit.h"
#include "smart_handle.h"
#include "unicode.h"
#include "wow.h"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iterator>

#include "my_windows.h"

#pragma warning(push)
#pragma warning(disable:4201) // warning C4201: nonstandard extension used: nameless struct/union
#pragma warning(disable:4324) // warning C4324: 'xxx': structure was padded due to alignment specifier
#pragma warning(disable:4471) // warning C4471: 'xxx': a forward declaration of an unscoped enumeration must have an underlying type
#include <phnt.h>
#pragma warning(pop)


#define STATUS_NO_MORE_ENTRIES ((NTSTATUS)0x8000001AL)


static std::wstring* g_known_dll_path = nullptr;
static std::vector<std::wstring>* g_known_dll_names = nullptr;
static std::vector<std::string>* g_known_dll_names_sorted_lowercase_ascii = nullptr;


struct buffer_t
{
	union union_t
	{
		OBJECT_DIRECTORY_INFORMATION m_odi;
		std::byte m_buff[1 * 1024];
	};
	union_t m_union;
};


bool is_section(OBJECT_DIRECTORY_INFORMATION const& odi);
bool is_symlink(OBJECT_DIRECTORY_INFORMATION const& odi);
bool is_known_dll_path(OBJECT_DIRECTORY_INFORMATION const& odi);


void known_dlls::init()
{
	if(g_known_dll_path)
	{
		assert(g_known_dll_path);
		assert(g_known_dll_names);
		assert(g_known_dll_names_sorted_lowercase_ascii);
		return;
	}
	std::wstring known_dll_path;
	std::vector<std::wstring> known_dll_names;
	std::vector<std::string> known_dll_names_sorted_lowercase_ascii;
	auto const finish = mk::make_scope_exit([&]()
	{
		assert(!g_known_dll_path);
		assert(!g_known_dll_names);
		assert(!g_known_dll_names_sorted_lowercase_ascii);
		g_known_dll_path = new std::wstring{std::move(known_dll_path)};
		g_known_dll_names = new std::vector<std::wstring>{std::move(known_dll_names)};
		g_known_dll_names_sorted_lowercase_ascii = new std::vector<std::string>{std::move(known_dll_names_sorted_lowercase_ascii)};
	});

	HMODULE const ntdll = GetModuleHandleW(L"ntdll.dll");
	WARN_M_RV(ntdll != nullptr, L"Library ntdll.dll is not loaded.");

	auto const NtOpenDirectoryObject_proc = GetProcAddress(ntdll, "NtOpenDirectoryObject");
	WARN_M_RV(NtOpenDirectoryObject_proc != 0, L"NtOpenDirectoryObject not found inside ntdll.dll.");
	auto const NtOpenDirectoryObject_fn = reinterpret_cast<decltype(&NtOpenDirectoryObject)>(NtOpenDirectoryObject_proc);

	auto const NtQueryDirectoryObject_proc = GetProcAddress(ntdll, "NtQueryDirectoryObject");
	WARN_M_RV(NtQueryDirectoryObject_proc != 0, L"NtQueryDirectoryObject not found inside ntdll.dll.");
	auto const NtQueryDirectoryObject_fn = reinterpret_cast<decltype(&NtQueryDirectoryObject)>(NtQueryDirectoryObject_proc);

	auto const NtOpenSymbolicLinkObject_proc = GetProcAddress(ntdll, "NtOpenSymbolicLinkObject");
	WARN_M_RV(NtOpenSymbolicLinkObject_proc != 0, L"NtOpenSymbolicLinkObject not found inside ntdll.dll.");
	auto const NtOpenSymbolicLinkObject_fn = reinterpret_cast<decltype(&NtOpenSymbolicLinkObject)>(NtOpenSymbolicLinkObject_proc);

	auto const NtQuerySymbolicLinkObject_proc = GetProcAddress(ntdll, "NtQuerySymbolicLinkObject");
	WARN_M_RV(NtQuerySymbolicLinkObject_proc != 0, L"NtQuerySymbolicLinkObject not found inside ntdll.dll.");
	auto const NtQuerySymbolicLinkObject_fn = reinterpret_cast<decltype(&NtQuerySymbolicLinkObject)>(NtQuerySymbolicLinkObject_proc);

	static constexpr wchar_t const s_known_dlls_folder_name[] = LR"---(\KnownDlls)---";
	#if defined _M_IX86
	static constexpr wchar_t const s_known_dlls_folder_name_wow[] = LR"---(\KnownDlls32)---";
	bool const iswow64 = is_wow64();
	auto const& known_dlls_folder_name = iswow64 ? s_known_dlls_folder_name_wow : s_known_dlls_folder_name;
	int const known_dlls_folder_name_len = static_cast<int>(iswow64 ? std::size(s_known_dlls_folder_name_wow) : std::size(s_known_dlls_folder_name)) - 1;
	#elif defined _M_X64
	auto const& known_dlls_folder_name = s_known_dlls_folder_name;
	int const known_dlls_folder_name_len = static_cast<int>(std::size(s_known_dlls_folder_name)) - 1;
	#else
	#error Unknown architecture.
	#endif

	HANDLE knonw_dll_dir;
	UNICODE_STRING name;
	name.Length = static_cast<USHORT>(known_dlls_folder_name_len * sizeof(wchar_t));
	name.MaximumLength = name.Length;
	name.Buffer = const_cast<wchar_t*>(known_dlls_folder_name);
	OBJECT_ATTRIBUTES oa_1{};
	oa_1.Length = sizeof(oa_1);
	oa_1.RootDirectory = nullptr;
	oa_1.ObjectName = &name;
	oa_1.Attributes = 0;
	oa_1.SecurityDescriptor = nullptr;
	oa_1.SecurityQualityOfService = nullptr;
	NTSTATUS const opened_1 = NtOpenDirectoryObject_fn(&knonw_dll_dir, DIRECTORY_QUERY, &oa_1);
	WARN_M_RV(opened_1 == 0, L"NtOpenDirectoryObject failed.");
	smart_handle const s_knonw_dll_dir(knonw_dll_dir);

	known_dll_names.reserve(32);
	BOOLEAN restart = TRUE;
	ULONG context = 0;
	for(;;)
	{
		buffer_t buff_1;
		ULONG ret_len;
		NTSTATUS const queried_1 = NtQueryDirectoryObject_fn(knonw_dll_dir, &buff_1, sizeof(buff_1), TRUE, restart, &context, &ret_len);
		restart = FALSE;
		WARN_M_RV(queried_1 == 0 || queried_1 == STATUS_NO_MORE_ENTRIES, L"NtQueryDirectoryObject failed.");
		if(queried_1 == STATUS_NO_MORE_ENTRIES)
		{
			break;
		}
		if(is_section(buff_1.m_union.m_odi))
		{
			WARN_M_RV(buff_1.m_union.m_odi.Name.Length % 2 == 0, L"Bad string length.");
			known_dll_names.emplace_back(buff_1.m_union.m_odi.Name.Buffer, buff_1.m_union.m_odi.Name.Length / 2);
		}
		else if(known_dll_path.empty() && is_symlink(buff_1.m_union.m_odi) && is_known_dll_path(buff_1.m_union.m_odi))
		{
			HANDLE symlink;
			OBJECT_ATTRIBUTES oa_2{};
			oa_2.Length = sizeof(oa_2);
			oa_2.RootDirectory = knonw_dll_dir;
			oa_2.ObjectName = &buff_1.m_union.m_odi.Name;
			oa_2.Attributes = 0;
			oa_2.SecurityDescriptor = nullptr;
			oa_2.SecurityQualityOfService = nullptr;
			NTSTATUS const opened_2 = NtOpenSymbolicLinkObject_fn(&symlink, GENERIC_READ, &oa_2);
			WARN_M_RV(opened_2 == 0, L"NtOpenSymbolicLinkObject failed.");
			smart_handle const s_symlink(symlink);
			wchar_t buff_2[1 * 1024];
			UNICODE_STRING target;
			target.Length = 0;
			target.MaximumLength = static_cast<USHORT>((std::size(buff_2) - 1) * sizeof(wchar_t));
			target.Buffer = buff_2;
			ULONG len;
			NTSTATUS const queried_2 = NtQuerySymbolicLinkObject_fn(symlink, &target, &len);
			WARN_M_RV(queried_2 == 0, L"NtQuerySymbolicLinkObject failed.");
			WARN_M_RV(target.Length % 2 == 0, L"Bad string length.");
			known_dll_path.assign(target.Buffer, target.Length / 2);
		}
	}

	WARN_M_RV(std::all_of(known_dll_names.begin(), known_dll_names.end(), [](auto const& e) -> bool { return is_ascii(e.c_str(), static_cast<int>(e.size())); }), L"Known DLL name must be ASCII.");
	known_dll_names_sorted_lowercase_ascii.resize(known_dll_names.size());
	std::transform(known_dll_names.begin(), known_dll_names.end(), known_dll_names_sorted_lowercase_ascii.begin(), [](auto const& wide) -> std::string
	{
		std::string narrow;
		narrow.resize(wide.size());
		std::transform(wide.begin(), wide.end(), narrow.begin(), [](auto const& ch) -> char { return to_lowercase(static_cast<char>(ch)); });
		return narrow;
	});
	std::sort(known_dll_names_sorted_lowercase_ascii.begin(), known_dll_names_sorted_lowercase_ascii.end());
}

void known_dlls::deinit()
{
	if(g_known_dll_path)
	{
		assert(g_known_dll_path);
		assert(g_known_dll_names);
		assert(g_known_dll_names_sorted_lowercase_ascii);
		delete g_known_dll_path;
		delete g_known_dll_names;
		delete g_known_dll_names_sorted_lowercase_ascii;
		g_known_dll_path = nullptr;
		g_known_dll_names = nullptr;
		g_known_dll_names_sorted_lowercase_ascii = nullptr;
	}
}


std::wstring const& known_dlls::get_path()
{
	init();
	assert(g_known_dll_path);
	return *g_known_dll_path;
}

std::vector<std::wstring> const& known_dlls::get_names()
{
	init();
	assert(g_known_dll_names);
	return *g_known_dll_names;
}

std::vector<std::string> const& known_dlls::get_names_sorted_lowercase_ascii()
{
	init();
	assert(g_known_dll_names_sorted_lowercase_ascii);
	return *g_known_dll_names_sorted_lowercase_ascii;
}


bool is_section(OBJECT_DIRECTORY_INFORMATION const& odi)
{
	static constexpr wchar_t const s_directory_item_type_name_section[] = L"Section";
	if(odi.TypeName.Length != (std::size(s_directory_item_type_name_section) - 1) * sizeof(wchar_t))
	{
		return false;
	}
	if(std::memcmp(odi.TypeName.Buffer, s_directory_item_type_name_section, std::size(s_directory_item_type_name_section) * sizeof(wchar_t)) != 0)
	{
		return false;
	}
	return true;
}

bool is_symlink(OBJECT_DIRECTORY_INFORMATION const& odi)
{
	static constexpr wchar_t const s_directory_item_type_name_symlink[] = L"SymbolicLink";
	if(odi.TypeName.Length != (std::size(s_directory_item_type_name_symlink) - 1) * sizeof(wchar_t))
	{
		return false;
	}
	if(std::memcmp(odi.TypeName.Buffer, s_directory_item_type_name_symlink, std::size(s_directory_item_type_name_symlink) * sizeof(wchar_t)) != 0)
	{
		return false;
	}
	return true;
}

bool is_known_dll_path(OBJECT_DIRECTORY_INFORMATION const& odi)
{
	static constexpr wchar_t const s_known_dlls_symlink_name[] = L"KnownDllPath";
	if(odi.Name.Length != (std::size(s_known_dlls_symlink_name) - 1) * sizeof(wchar_t))
	{
		return false;
	}
	if(std::memcmp(odi.Name.Buffer, s_known_dlls_symlink_name, std::size(s_known_dlls_symlink_name) * sizeof(wchar_t)) != 0)
	{
		return false;
	}
	return true;
}
