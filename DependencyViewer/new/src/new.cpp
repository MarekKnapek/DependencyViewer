#include <array> // array
#include <cstddef> // size_t

#include <windows.h>


#define CHECK_RET_MSG(A, B, F, ...) do{ if(!(A)){ check_ret_failed(__LINE__, L ## #A, F, __VA_ARGS__); return B; } }while(false)
#define CHECK_RET(A, B) do{ if(!(A)){ check_ret_failed(__LINE__, L ## #A); return B; } }while(false)
#define CHECK_RET_V(A) do{ if(!(A)){ check_ret_failed(__LINE__, L ## #A); return; } }while(false)


#define USE_THREADS 0


void stop_here_dummy()
{
}

template<std::size_t N, typename... ts_t>
void check_ret_failed(int const& line, wchar_t const(&expression)[N], wchar_t const* const& fmt, ts_t const&... ts)
{
	std::array<wchar_t, 1024> check_ret_failed_buff_1;
	std::array<wchar_t, 1024> check_ret_failed_buff_2;

	int const printed_1 = std::swprintf(check_ret_failed_buff_1.data(), check_ret_failed_buff_1.size(), fmt, ts...);
	if(!(printed_1 >= 0 && printed_1 < static_cast<int>(check_ret_failed_buff_1.size()))) std::abort();
	int const printed_2 = std::swprintf(check_ret_failed_buff_2.data(), check_ret_failed_buff_2.size(), L"check_ret_failed: Line: %d, expression: `%s`, message: `%s`.\x0D\x0A", line, expression, check_ret_failed_buff_1.data());
	if(!(printed_2 >= 0 && printed_2 < static_cast<int>(check_ret_failed_buff_2.size()))) std::abort();
	OutputDebugStringW(check_ret_failed_buff_2.data());
	stop_here_dummy();
}

template<std::size_t N>
void check_ret_failed(int const& line, wchar_t const(&expression)[N])
{
	std::array<wchar_t, 1024> check_ret_failed_buff_2;

	int const printed_2 = std::swprintf(check_ret_failed_buff_2.data(), check_ret_failed_buff_2.size(), L"check_ret_failed: Line: %d, expression: `%s`.\x0D\x0A", line, expression);
	if(!(printed_2 >= 0 && printed_2 < static_cast<int>(check_ret_failed_buff_2.size()))) std::abort();
	OutputDebugStringW(check_ret_failed_buff_2.data());
	stop_here_dummy();
}










#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <cassert>


struct work_item_t
{
	void(*m_func)(void* data);
	void* m_data;
};


class work_pool_t
{
public:
	work_pool_t();
	~work_pool_t();
	void submit(void(*const& func)(void*), void* const& data);
	void submit(work_item_t const& wi);
private:
	static void thread_func(void* const data);
	void thread_func();
private:
	std::mutex m_mutex;
	std::condition_variable m_cv;
	std::vector<work_item_t> m_work_items;
	std::vector<std::thread> m_threads;
	bool m_shall_run;
};


work_pool_t::work_pool_t() :
	m_mutex(),
	m_cv(),
	m_work_items(),
	m_threads(),
	m_shall_run(true)
{
	unsigned const n = std::thread::hardware_concurrency();
	m_threads.resize(n);
	for(unsigned i = 0; i != n; ++i)
	{
		void(*const func)(void*) = &work_pool_t::thread_func;
		void* const data = this;
		m_threads[i] = std::thread{func, data};
	}
}

work_pool_t::~work_pool_t()
{
	{
		std::lock_guard<std::mutex> const lck{m_mutex};
		m_shall_run = false;
	}
	m_cv.notify_all();
	for(auto& t : m_threads)
	{
		t.join();
	}
}

void work_pool_t::submit(void(*const& func)(void*), void* const& data)
{
	submit(work_item_t{func, data});
}

void work_pool_t::submit(work_item_t const& wi)
{
	{
		std::lock_guard<std::mutex> const lck{m_mutex};
		m_work_items.push_back(wi);
	}
	m_cv.notify_one();
}

void work_pool_t::thread_func(void* const data)
{
	static_cast<work_pool_t*>(data)->thread_func();
}

void work_pool_t::thread_func()
{
	for(;;)
	{
		work_item_t wi;
		{
			std::unique_lock<std::mutex> lck{m_mutex};
			run:;
			if(m_work_items.empty())
			{
				if(!m_shall_run)
				{
					return;
				}
				else
				{
					m_cv.wait(lck);
					goto run;
				}
			}
			else
			{
				wi = m_work_items.back();
				m_work_items.pop_back();
			}
		}
		wi.m_func(wi.m_data);
	}
}










#include <Windows.h>


class read_only_file_t
{
public:
	read_only_file_t() noexcept;
	static read_only_file_t make(wchar_t const* const& file_path);
	read_only_file_t(read_only_file_t const&) = delete;
	read_only_file_t(read_only_file_t&& other) noexcept;
	read_only_file_t& operator=(read_only_file_t const&) = delete;
	read_only_file_t& operator=(read_only_file_t&& other) noexcept;
	~read_only_file_t() noexcept;
	void swap(read_only_file_t& other) noexcept;
	explicit operator bool() const;
	void reset();
public:
	HANDLE const& get_handle() const;
private:
	explicit read_only_file_t(HANDLE const& h);
private:
	HANDLE m_handle;
};

inline void swap(read_only_file_t& a, read_only_file_t& b) noexcept { a.swap(b); }










#include <cassert>
#include <utility> // swap


read_only_file_t::read_only_file_t() noexcept :
	m_handle(INVALID_HANDLE_VALUE)
{
}

read_only_file_t read_only_file_t::make(wchar_t const* const& file_path)
{
	LPCWSTR const lpFileName = file_path;
	DWORD const dwDesiredAccess = GENERIC_READ;
	DWORD const dwShareMode = FILE_SHARE_READ | FILE_SHARE_DELETE;
	LPSECURITY_ATTRIBUTES const lpSecurityAttributes = nullptr;
	DWORD const dwCreationDisposition = OPEN_EXISTING;
	DWORD const dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;
	HANDLE const hTemplateFile = HANDLE{};
	HANDLE const h = CreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
	return read_only_file_t{h};
}

read_only_file_t::read_only_file_t(read_only_file_t&& other) noexcept :
	read_only_file_t()
{
	swap(other);
}

read_only_file_t& read_only_file_t::operator=(read_only_file_t&& other) noexcept
{
	swap(other);
	return *this;
}

read_only_file_t::~read_only_file_t() noexcept
{
	if(m_handle == INVALID_HANDLE_VALUE) return;
	BOOL const closed = CloseHandle(m_handle);
	assert(closed != 0);
}

void read_only_file_t::swap(read_only_file_t& other) noexcept
{
	using std::swap;
	swap(m_handle, other.m_handle);
}

read_only_file_t::operator bool() const
{
	return m_handle != INVALID_HANDLE_VALUE;
}

void read_only_file_t::reset()
{
	*this = read_only_file_t{};
}

HANDLE const& read_only_file_t::get_handle() const
{
	return m_handle;
}

read_only_file_t::read_only_file_t(HANDLE const& h) :
	read_only_file_t()
{
	m_handle = h;
}










#include <cstdint>


std::uint64_t get_file_size(read_only_file_t const& file);










#include <cassert>


std::uint64_t get_file_size(read_only_file_t const& file)
{
	assert(file);
	LARGE_INTEGER size;
	BOOL const got = GetFileSizeEx(file.get_handle(), &size);
	CHECK_RET(got != 0, 0);
	CHECK_RET(size.QuadPart >= 0, 0);
	return static_cast<std::uint64_t>(size.QuadPart);
}










class read_only_file_mapping_t
{
public:
	read_only_file_mapping_t() noexcept;
	static read_only_file_mapping_t make(read_only_file_t const& file);
	read_only_file_mapping_t(read_only_file_mapping_t const&) = delete;
	read_only_file_mapping_t(read_only_file_mapping_t&& other) noexcept;
	read_only_file_mapping_t& operator=(read_only_file_mapping_t const&) = delete;
	read_only_file_mapping_t& operator=(read_only_file_mapping_t&& other) noexcept;
	~read_only_file_mapping_t() noexcept;
	void swap(read_only_file_mapping_t& other) noexcept;
	explicit operator bool() const;
	void reset();
public:
	HANDLE const& get_handle() const;
private:
	explicit read_only_file_mapping_t(HANDLE const& h);
private:
	HANDLE m_handle;
};

inline void swap(read_only_file_mapping_t& a, read_only_file_mapping_t& b) noexcept { a.swap(b); }










#include <cassert>


read_only_file_mapping_t::read_only_file_mapping_t() noexcept :
	m_handle()
{
}

read_only_file_mapping_t read_only_file_mapping_t::make(read_only_file_t const& file)
{
	assert(file);
	assert(get_file_size(file) != 0);
	HANDLE const hFile = file.get_handle();
	LPSECURITY_ATTRIBUTES const lpFileMappingAttributes = nullptr;
	DWORD const flProtect = PAGE_READONLY;
	DWORD const dwMaximumSizeHigh = 0;
	DWORD const dwMaximumSizeLow = 0;
	LPCWSTR const lpName = nullptr;
	HANDLE const h = CreateFileMappingW(hFile, lpFileMappingAttributes, flProtect, dwMaximumSizeHigh, dwMaximumSizeLow, lpName);
	return read_only_file_mapping_t{h};
}

read_only_file_mapping_t::read_only_file_mapping_t(read_only_file_mapping_t&& other) noexcept :
	read_only_file_mapping_t()
{
	swap(other);
}

read_only_file_mapping_t& read_only_file_mapping_t::operator=(read_only_file_mapping_t&& other) noexcept
{
	swap(other);
	return *this;
}

read_only_file_mapping_t::~read_only_file_mapping_t() noexcept
{
	if(m_handle == HANDLE{}) return;
	BOOL const closed = CloseHandle(m_handle);
	assert(closed != 0);
}

void read_only_file_mapping_t::swap(read_only_file_mapping_t& other) noexcept
{
	using std::swap;
	swap(m_handle, other.m_handle);
}

read_only_file_mapping_t::operator bool() const
{
	return m_handle != HANDLE{};
}

void read_only_file_mapping_t::reset()
{
	*this = read_only_file_mapping_t{};
}

HANDLE const& read_only_file_mapping_t::get_handle() const
{
	return m_handle;
}

read_only_file_mapping_t::read_only_file_mapping_t(HANDLE const& h) :
	read_only_file_mapping_t()
{
	m_handle = h;
}










std::pair<int, int> get_allocation_granularity();


static std::pair<int, int> const s_allocation_granularity = get_allocation_granularity();


std::pair<int, int> get_allocation_granularity()
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	int const page_size = static_cast<int>(si.dwPageSize);
	int const allocation_granularity = static_cast<int>(si.dwAllocationGranularity);
	if(page_size != 4 * 1024) std::abort();
	if(allocation_granularity != 64 * 1024) std::abort();
	return {page_size, allocation_granularity};
}










class read_only_map_view_of_file_t
{
public:
	read_only_map_view_of_file_t() noexcept;
	static read_only_map_view_of_file_t make(read_only_file_mapping_t const& mapping, unsigned const& offset, unsigned const& length);
	read_only_map_view_of_file_t(read_only_map_view_of_file_t const&) = delete;
	read_only_map_view_of_file_t(read_only_map_view_of_file_t&& other) noexcept;
	read_only_map_view_of_file_t& operator=(read_only_map_view_of_file_t const&) = delete;
	read_only_map_view_of_file_t& operator=(read_only_map_view_of_file_t&& other) noexcept;
	~read_only_map_view_of_file_t() noexcept;
	void swap(read_only_map_view_of_file_t& other) noexcept;
	explicit operator bool() const;
	void reset();
public:
	void const* const& get_view() const;
private:
	explicit read_only_map_view_of_file_t(void const* const& v);
private:
	void const* m_view;
};

inline void swap(read_only_map_view_of_file_t& a, read_only_map_view_of_file_t& b) noexcept { a.swap(b); }










#include <cassert>


read_only_map_view_of_file_t::read_only_map_view_of_file_t() noexcept :
	m_view()
{
}

read_only_map_view_of_file_t read_only_map_view_of_file_t::make(read_only_file_mapping_t const& mapping, unsigned const& offset, unsigned const& length)
{
	assert(mapping);

	std::uint32_t const offset_aditional = static_cast<std::uint32_t>(offset) & static_cast<std::uint32_t>(0xFFFFull);
	std::uint32_t const offset_rounded = static_cast<std::uint32_t>(offset) &~ static_cast<std::uint32_t>(0xFFFFull);

	HANDLE const hFileMappingObject = mapping.get_handle();
	DWORD const dwDesiredAccess = FILE_MAP_READ;
	DWORD const dwFileOffsetHigh = 0;
	DWORD const dwFileOffsetLow = offset_rounded;
	SIZE_T const dwNumberOfBytesToMap = length + offset_aditional;
	LPVOID const v = MapViewOfFile(hFileMappingObject, dwDesiredAccess, dwFileOffsetHigh, dwFileOffsetLow, dwNumberOfBytesToMap);
	if(v == nullptr) [[unlikely]] return read_only_map_view_of_file_t{};

	void const* const ptr = static_cast<void const*>(static_cast<unsigned char const*>(v) + offset_aditional);
	return read_only_map_view_of_file_t{ptr};
}

read_only_map_view_of_file_t::read_only_map_view_of_file_t(read_only_map_view_of_file_t&& other) noexcept :
	read_only_map_view_of_file_t()
{
	swap(other);
}

read_only_map_view_of_file_t& read_only_map_view_of_file_t::operator=(read_only_map_view_of_file_t&& other) noexcept
{
	swap(other);
	return *this;
}

read_only_map_view_of_file_t::~read_only_map_view_of_file_t() noexcept
{
	if(m_view == nullptr) return;
	void const* const ptr = reinterpret_cast<void const*>(reinterpret_cast<std::uintptr_t>(m_view) &~ static_cast<std::uintptr_t>(0xFFFFull));
	BOOL const unmapped = UnmapViewOfFile(ptr);
	assert(unmapped != 0);
}

void read_only_map_view_of_file_t::swap(read_only_map_view_of_file_t& other) noexcept
{
	using std::swap;
	swap(m_view, other.m_view);
}

read_only_map_view_of_file_t::operator bool() const
{
	return m_view != nullptr;
}

void read_only_map_view_of_file_t::reset()
{
	*this = read_only_map_view_of_file_t{};
}

void const* const& read_only_map_view_of_file_t::get_view() const
{
	return m_view;
}

read_only_map_view_of_file_t::read_only_map_view_of_file_t(void const* const& v) :
	read_only_map_view_of_file_t()
{
	m_view = v;
}










void crawl_all_files(std::array<wchar_t, 32 * 1024>& path, void(* const& pfn)(wchar_t const* const& path, int const& len, void* const data), void* const data);


#include "scope_exit.h"
#include <array>


bool skip_dots(HANDLE const& fff, WIN32_FIND_DATAW* const& output);
void crawl_all_files_r(std::array<wchar_t, 32 * 1024>& path, int const& len, void(* const& pfn)(wchar_t const* const& path, int const& len, void* const data), void* const data, WIN32_FIND_DATAW& find_data);


bool skip_dots(HANDLE const& fff, WIN32_FIND_DATAW* const& output)
{
	for(;;)
	{
		bool const do_skip =
			(output->cFileName[0] == L'.' && output->cFileName[1] == L'\0') ||
			(output->cFileName[0] == L'.' && output->cFileName[1] == L'.' && output->cFileName[2] == L'\0');
		if(!do_skip)
		{
			break;
		}
		BOOL const found = FindNextFileW(fff, output);
		if(found == 0)
		{
			return false;
		}
	}
	return true;
}

void crawl_all_files_r(std::array<wchar_t, 32 * 1024>& path, int const& len, void(* const& pfn)(wchar_t const* const& path, int const& len, void* const data), void* const data, WIN32_FIND_DATAW& find_data)
{
	assert(path[len - 1] != L'\\');
	path[len + 0] = L'\\';
	path[len + 1] = L'*';
	path[len + 2] = L'\0';

	LPCWSTR const lpFileName = path.data();
	FINDEX_INFO_LEVELS const fInfoLevelId = FindExInfoBasic;
	LPVOID const lpFindFileData = &find_data;
	FINDEX_SEARCH_OPS const fSearchOp = FindExSearchNameMatch;
	LPVOID const lpSearchFilter = nullptr;
	DWORD const dwAdditionalFlags = FIND_FIRST_EX_LARGE_FETCH;
	HANDLE const fff = FindFirstFileExW(lpFileName, fInfoLevelId, lpFindFileData, fSearchOp, lpSearchFilter, dwAdditionalFlags);
	if(fff == INVALID_HANDLE_VALUE) return;
	auto const close_find = mk::make_scope_exit([&](){ BOOL const closed = FindClose(fff); assert(closed != 0); });

	bool const keep_going = skip_dots(fff, &find_data);
	if(!keep_going)
	{
		return;
	}

	do
	{
		int const len_2 = static_cast<int>(std::wcslen(find_data.cFileName));
		path[len] = L'\\';
		std::memcpy(path.data() + len + 1, find_data.cFileName, (len_2 + 1) * sizeof(wchar_t));
		bool const is_dir = (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
		if(is_dir)
		{
			crawl_all_files_r(path, len + len_2 + 1, pfn, data, find_data);
		}
		else
		{
			pfn(path.data(), len + len_2 + 1, data);
		}
	}while(FindNextFileW(fff, &find_data) != 0);
	assert(GetLastError() == ERROR_NO_MORE_FILES);
}

void crawl_all_files(std::array<wchar_t, 32 * 1024>& path, void(* const& pfn)(wchar_t const* const& path, int const& len, void* const data), void* const data)
{
	WIN32_FIND_DATAW find_data;
	int const len = static_cast<int>(std::wcslen(path.data()));
	assert(path[len - 1] != L'\\');
	crawl_all_files_r(path, len, pfn, data, find_data);
}










#include <cstdint>


struct mk_coff_file_header_t
{
	std::uint16_t m_machine;
	std::uint16_t m_number_of_sections;
	std::uint32_t m_time_date_stamp;
	std::uint32_t m_pointer_to_symbol_table;
	std::uint32_t m_number_of_symbols;
	std::uint16_t m_size_of_optional_header;
	std::uint16_t m_characteristics;
};
static_assert(sizeof(mk_coff_file_header_t) == 20);
static_assert(sizeof(mk_coff_file_header_t) == 0x14);

struct mk_coff_optional_header_pe32_standard_t
{
	std::uint16_t m_magic;
	std::uint8_t m_major_linker_version;
	std::uint8_t m_minor_linker_version;
	std::uint32_t m_size_of_code;
	std::uint32_t m_size_of_initialized_data;
	std::uint32_t m_size_of_uninitialized_data;
	std::uint32_t m_address_of_entry_point;
	std::uint32_t m_base_of_code;
	std::uint32_t m_base_of_data;
};
static_assert(sizeof(mk_coff_optional_header_pe32_standard_t) == 28);
static_assert(sizeof(mk_coff_optional_header_pe32_standard_t) == 0x1c);

struct mk_coff_optional_header_pe32plus_standard_t
{
	std::uint16_t m_magic;
	std::uint8_t m_major_linker_version;
	std::uint8_t m_minor_linker_version;
	std::uint32_t m_size_of_code;
	std::uint32_t m_size_of_initialized_data;
	std::uint32_t m_size_of_uninitialized_data;
	std::uint32_t m_address_of_entry_point;
	std::uint32_t m_base_of_code;
};
static_assert(sizeof(mk_coff_optional_header_pe32plus_standard_t) == 24);
static_assert(sizeof(mk_coff_optional_header_pe32plus_standard_t) == 0x18);

struct mk_coff_optional_header_pe32_windows_t
{
	std::uint32_t m_image_base;
	std::uint32_t m_section_alignment;
	std::uint32_t m_file_alignment;
	std::uint16_t m_major_operating_system_version;
	std::uint16_t m_minor_operating_system_version;
	std::uint16_t m_major_image_version;
	std::uint16_t m_minor_image_version;
	std::uint16_t m_major_subsystem_version;
	std::uint16_t m_minor_subsystem_version;
	std::uint32_t m_reserved;
	std::uint32_t m_size_of_image;
	std::uint32_t m_size_of_headers;
	std::uint32_t m_check_sum;
	std::uint16_t m_subsystem;
	std::uint16_t m_dll_characteristics;
	std::uint32_t m_size_of_stack_reserve;
	std::uint32_t m_size_of_stack_commit;
	std::uint32_t m_size_of_heap_reserve;
	std::uint32_t m_size_of_heap_commit;
	std::uint32_t m_loader_flags;
	std::uint32_t m_number_of_rva_snd_sizes;
};
static_assert(sizeof(mk_coff_optional_header_pe32_windows_t) == 68, "");
static_assert(sizeof(mk_coff_optional_header_pe32_windows_t) == 0x44, "");

struct mk_coff_optional_header_pe32plus_windows_t
{
	std::uint64_t m_image_base;
	std::uint32_t m_section_alignment;
	std::uint32_t m_file_alignment;
	std::uint16_t m_major_operating_system_version;
	std::uint16_t m_minor_operating_system_version;
	std::uint16_t m_major_image_version;
	std::uint16_t m_minor_image_version;
	std::uint16_t m_major_subsystem_version;
	std::uint16_t m_minor_subsystem_version;
	std::uint32_t m_reserved;
	std::uint32_t m_size_of_image;
	std::uint32_t m_size_of_headers;
	std::uint32_t m_check_sum;
	std::uint16_t m_subsystem;
	std::uint16_t m_dll_characteristics;
	std::uint64_t m_size_of_stack_reserve;
	std::uint64_t m_size_of_stack_commit;
	std::uint64_t m_size_of_heap_reserve;
	std::uint64_t m_size_of_heap_commit;
	std::uint32_t m_loader_flags;
	std::uint32_t m_number_of_rva_snd_sizes;
};
static_assert(sizeof(mk_coff_optional_header_pe32plus_windows_t) == 88, "");
static_assert(sizeof(mk_coff_optional_header_pe32plus_windows_t) == 0x58, "");

struct mk_image_data_directory_t
{
	std::uint32_t m_rva;
	std::uint32_t m_size;
};
static_assert(sizeof(mk_image_data_directory_t) == 8, "");
static_assert(sizeof(mk_image_data_directory_t) == 0x8, "");
typedef std::array<mk_image_data_directory_t, 16> mk_image_data_directories_t;
enum class mk_image_data_directory_e
{
	export_table,
	import_table,
	resource_table,
	exception_table,
	certificate_table,
	base_relocation_table,
	debug,
	architecture,
	global_ptr,
	tls_table,
	load_config_table,
	bound_import,
	iat,
	delay_import_descriptor,
	clr_runtime_header,
	reserved,
};

// aka section_table_entry
struct mk_section_header_t
{
	char m_name[8];
	std::uint32_t m_virtual_size;
	std::uint32_t m_virtual_address;
	std::uint32_t m_size_of_raw_data;
	std::uint32_t m_pointer_to_raw_data;
	std::uint32_t m_pointer_to_relocations;
	std::uint32_t m_pointer_to_linenumbers;
	std::uint16_t m_number_of_relocations;
	std::uint16_t m_number_of_linenumbers;
	std::uint32_t m_characteristics;
};
static_assert(sizeof(mk_section_header_t) == 40, "");
static_assert(sizeof(mk_section_header_t) == 0x28, "");
typedef std::array<mk_section_header_t, 96> mk_section_headers_t;
enum class mk_section_header_characteristics_e : std::uint32_t
{
	code = 0x00000020ull,
	initialized_data = 0x00000040ull,
	uninitialized_data = 0x00000080ull,
	discardable = 0x02000000ull,
	not_cached = 0x04000000ull,
	not_paged = 0x08000000ull,
	shared = 0x10000000ull,
	execute = 0x20000000ull,
	read = 0x40000000ull,
	write = 0x80000000ull,
};


bool is_pe(void const* const& ptr, int const& len)
{
	unsigned char const* const data = static_cast<unsigned char const*>(ptr);
	std::uint32_t const file_size = static_cast<std::uint32_t>(len);

	std::uint16_t mz_signature;
	static constexpr std::uint32_t const s_mz_signature_offset = 0;
	if(!(file_size >= s_mz_signature_offset + sizeof(mz_signature))) return false;
	std::memcpy(&mz_signature, data + s_mz_signature_offset, sizeof(mz_signature));
	static constexpr std::uint16_t const s_mz_signature = 0x5A4D; // MZ
	if(!(mz_signature == s_mz_signature)) return false;
	
	std::uint16_t pe_offset;
	static constexpr std::uint32_t const s_pe_offset_offset = 0x3c;
	if(!(file_size >= s_pe_offset_offset + sizeof(pe_offset))) return false;
	std::memcpy(&pe_offset, data + s_pe_offset_offset, sizeof(pe_offset));

	std::uint32_t pe_signature;
	if(!(file_size >= pe_offset + sizeof(pe_signature))) return false;
	std::memcpy(&pe_signature, data + pe_offset, sizeof(pe_signature));
	static constexpr std::uint32_t const s_pe_signature = 0x00004550; // PE\0\0
	if(!(pe_signature == s_pe_signature)) return false;

	return true;
}

bool parse_pe(void const* const& ptr, int const& len, mk_image_data_directories_t* const& image_data_directories_out, int* const& image_data_directories_count_out, mk_section_headers_t* const& section_headers_out, int* const section_headers_count_out, bool* const& is_32_bit_out)
{
	unsigned char const* const data = static_cast<unsigned char const*>(ptr);
	std::uint32_t const file_size = static_cast<std::uint32_t>(len);

	std::uint16_t mz_signature;
	static constexpr std::uint32_t const s_mz_signature_offset = 0;
	CHECK_RET(file_size >= s_mz_signature_offset + sizeof(mz_signature), false);
	std::memcpy(&mz_signature, data + s_mz_signature_offset, sizeof(mz_signature));
	static constexpr std::uint16_t const s_mz_signature = 0x5A4D; // MZ
	CHECK_RET(mz_signature == s_mz_signature, false);
	
	std::uint16_t pe_offset;
	static constexpr std::uint32_t const s_pe_offset_offset = 0x3c;
	CHECK_RET(file_size >= s_pe_offset_offset + sizeof(pe_offset), false);
	std::memcpy(&pe_offset, data + s_pe_offset_offset, sizeof(pe_offset));

	std::uint32_t pe_signature;
	CHECK_RET(file_size >= pe_offset + sizeof(pe_signature), false);
	std::memcpy(&pe_signature, data + pe_offset, sizeof(pe_signature));
	static constexpr std::uint32_t const s_pe_signature = 0x00004550; // PE\0\0
	CHECK_RET(pe_signature == s_pe_signature, false);

	mk_coff_file_header_t coff_file_header;
	std::uint32_t const coff_file_header_offset = pe_offset + sizeof(pe_signature);
	CHECK_RET(file_size >= coff_file_header_offset + sizeof(coff_file_header), false);
	std::memcpy(&coff_file_header, data + coff_file_header_offset, sizeof(coff_file_header));

	std::uint16_t optional_header_signature;
	std::uint32_t const optional_header_signature_offset = coff_file_header_offset + sizeof(coff_file_header);
	CHECK_RET(file_size >= optional_header_signature_offset + sizeof(optional_header_signature), false);
	std::memcpy(&optional_header_signature, data + optional_header_signature_offset, sizeof(optional_header_signature));
	static constexpr std::uint16_t const s_optional_header_signature_pe32 = 0x010b;
	static constexpr std::uint16_t const s_optional_header_signature_pe32plus = 0x020b;
	CHECK_RET(optional_header_signature == s_optional_header_signature_pe32 || optional_header_signature == s_optional_header_signature_pe32plus, false);
	assert(is_32_bit_out);
	*is_32_bit_out = optional_header_signature == s_optional_header_signature_pe32;

	std::uint32_t number_of_rva_snd_sizes;
	std::uint32_t image_data_directory_offset;
	if(optional_header_signature == s_optional_header_signature_pe32)
	{
		mk_coff_optional_header_pe32_standard_t optional_header_standard;
		std::uint32_t const optional_header_standard_offset = optional_header_signature_offset;
		CHECK_RET(file_size >= optional_header_standard_offset + sizeof(optional_header_standard), false);
		std::memcpy(&optional_header_standard, data + optional_header_standard_offset, sizeof(optional_header_standard));

		mk_coff_optional_header_pe32_windows_t optional_header_windows;
		std::uint32_t const optional_header_windows_offset = optional_header_standard_offset + sizeof(optional_header_standard);
		CHECK_RET(file_size >= optional_header_windows_offset + sizeof(optional_header_windows), false);
		std::memcpy(&optional_header_windows, data + optional_header_windows_offset, sizeof(optional_header_windows));

		CHECK_RET(optional_header_windows.m_number_of_rva_snd_sizes <= image_data_directories_out->size(), false);
		number_of_rva_snd_sizes = optional_header_windows.m_number_of_rva_snd_sizes;
		image_data_directory_offset = optional_header_windows_offset + sizeof(optional_header_windows);
	}
	else
	{
		mk_coff_optional_header_pe32plus_standard_t optional_header_standard;
		std::uint32_t const optional_header_standard_offset = optional_header_signature_offset;
		CHECK_RET(file_size >= optional_header_standard_offset + sizeof(optional_header_standard), false);
		std::memcpy(&optional_header_standard, data + optional_header_standard_offset, sizeof(optional_header_standard));

		mk_coff_optional_header_pe32plus_windows_t optional_header_windows;
		std::uint32_t const optional_header_windows_offset = optional_header_standard_offset + sizeof(optional_header_standard);
		CHECK_RET(file_size >= optional_header_windows_offset + sizeof(optional_header_windows), false);
		std::memcpy(&optional_header_windows, data + optional_header_windows_offset, sizeof(optional_header_windows));

		CHECK_RET(optional_header_windows.m_number_of_rva_snd_sizes <= image_data_directories_out->size(), false);
		number_of_rva_snd_sizes = optional_header_windows.m_number_of_rva_snd_sizes;
		image_data_directory_offset = optional_header_windows_offset + sizeof(optional_header_windows);
	}

	mk_image_data_directories_t& image_data_directories = *image_data_directories_out;
	CHECK_RET(file_size >= image_data_directory_offset + number_of_rva_snd_sizes * sizeof(mk_image_data_directory_t), false);
	for(std::uint32_t i = 0; i != number_of_rva_snd_sizes; ++i)
	{
		std::memcpy(&image_data_directories[i].m_rva, data + image_data_directory_offset + i * sizeof(mk_image_data_directory_t) + 0, sizeof(image_data_directories[i].m_rva));
		std::memcpy(&image_data_directories[i].m_size, data + image_data_directory_offset + i * sizeof(mk_image_data_directory_t) + sizeof(image_data_directories[i].m_rva), sizeof(image_data_directories[i].m_rva));
	}
	int& image_data_directories_count = *image_data_directories_count_out;
	image_data_directories_count = static_cast<int>(number_of_rva_snd_sizes);

	CHECK_RET(coff_file_header.m_number_of_sections <= section_headers_out->size(), false);
	std::uint32_t const section_headers_offset = image_data_directory_offset + number_of_rva_snd_sizes * sizeof(mk_image_data_directory_t);
	CHECK_RET(file_size >= section_headers_offset + coff_file_header.m_number_of_sections * sizeof(mk_image_data_directory_t), false);
	mk_section_headers_t& section_headers = *section_headers_out;
	for(std::uint16_t i = 0; i != coff_file_header.m_number_of_sections; ++i)
	{
		std::memcpy(&section_headers[i], data + section_headers_offset + i * sizeof(mk_section_header_t), sizeof(mk_section_header_t));
	}
	int& section_headers_count = *section_headers_count_out;
	section_headers_count = static_cast<int>(coff_file_header.m_number_of_sections);

	return true;
}

struct mk_export_directory_entry_t
{
	std::uint32_t m_export_flags;
	std::uint32_t m_time_date_stamp;
	std::uint16_t m_major_version;
	std::uint16_t m_minor_version;
	std::uint32_t m_name_rva;
	std::uint32_t m_ordinal_base;
	std::uint32_t m_address_table_entries;
	std::uint32_t m_number_of_name_pointers;
	std::uint32_t m_export_address_table_rva;
	std::uint32_t m_name_pointer_rva;
	std::uint32_t m_ordinal_table_rva;
};
static_assert(sizeof(mk_export_directory_entry_t) == 40, "");
static_assert(sizeof(mk_export_directory_entry_t) == 0x28, "");

bool parse_export_section(void const* const& ptr, int const& len)
{
	unsigned char const* const data = static_cast<unsigned char const*>(ptr);
	std::uint32_t const section_size = static_cast<std::uint32_t>(len);

	static constexpr std::uint32_t const s_export_directory_table_offset = 0;
	mk_export_directory_entry_t export_directory_table;
	CHECK_RET(section_size >= s_export_directory_table_offset + sizeof(export_directory_table), false);
	std::memcpy(&export_directory_table, data + s_export_directory_table_offset, sizeof(export_directory_table));

	return true;
}










#include <array>


typedef std::array<read_only_map_view_of_file_t, 96> mapped_sections_t;


bool map_sections(std::uint32_t const& file_size, read_only_file_mapping_t const& mapping, mk_section_headers_t const& section_headers, int const& count, mapped_sections_t* const& mapped_sections_out);










#include <cassert>


bool map_sections(std::uint32_t const& file_size, read_only_file_mapping_t const& mapping, mk_section_headers_t const& section_headers, int const& count, mapped_sections_t* const& mapped_sections_out)
{
	static constexpr auto const is_interesting_section = [](mk_section_header_t const& section_header)
	{
		bool const read = (section_header.m_characteristics & static_cast<std::uint32_t>(mk_section_header_characteristics_e::read)) != 0;
		bool const code = (section_header.m_characteristics & static_cast<std::uint32_t>(mk_section_header_characteristics_e::code)) != 0;
		bool const initialized_data = (section_header.m_characteristics & static_cast<std::uint32_t>(mk_section_header_characteristics_e::initialized_data)) != 0;
		bool const uninitialized_data = (section_header.m_characteristics & static_cast<std::uint32_t>(mk_section_header_characteristics_e::uninitialized_data)) != 0;
		bool const ret = read && (code || initialized_data) && !uninitialized_data;
		return ret;
	};

	assert(mapped_sections_out);
	mapped_sections_t& mapped_sections = *mapped_sections_out;
	for(int i = 0; i != count; ++i)
	{
		mk_section_header_t const& section_header = section_headers[i];
		if(is_interesting_section(section_header))
		{
			CHECK_RET(file_size >= section_header.m_pointer_to_raw_data, false);
			CHECK_RET(file_size >= section_header.m_pointer_to_raw_data + section_header.m_size_of_raw_data, false); // TODO: Overflow?
			mapped_sections[i] = read_only_map_view_of_file_t::make(mapping, section_header.m_pointer_to_raw_data, section_header.m_size_of_raw_data);
			CHECK_RET(mapped_sections[i], false);
		}
	}
	return true;
}

void const* find_in_sections(mk_section_headers_t const& section_headers, int const& count, mapped_sections_t const& mapped_sections, std::uint32_t const& rva, std::uint32_t const& size, int* const section_idx_out = nullptr)
{
	CHECK_RET(rva != 0, nullptr);
	for(int i = 0; i != count; ++i)
	{
		read_only_map_view_of_file_t const& view = mapped_sections[i];
		if(!view) continue;
		mk_section_header_t const& section_header = section_headers[i];
		if(rva >= section_header.m_virtual_address && rva + size <= section_header.m_virtual_address + section_header.m_size_of_raw_data)
		{
			void const* const v = view.get_view();
			std::uint32_t const offset = rva - section_header.m_virtual_address;
			void const* const address = static_cast<unsigned char const*>(v) + offset;
			if(section_idx_out) *section_idx_out = i;
			return address;
		}
	}
	CHECK_RET(false, nullptr);
}










struct mk_import_directory_entry_t
{
	std::uint32_t m_import_lookup_table_rva;
	std::uint32_t m_time_date_stamp;
	std::uint32_t m_fowarder_chain;
	std::uint32_t m_name;
	std::uint32_t m_import_address_table_rva;
};
static_assert(sizeof(mk_import_directory_entry_t) == 20);
static_assert(sizeof(mk_import_directory_entry_t) == 0x14);

bool operator==(mk_import_directory_entry_t const& a, mk_import_directory_entry_t const& b) { return std::memcmp(&a, &b, sizeof(a)) == 0; }

struct mk_import_hint_name_entry_t
{
	std::uint16_t m_hint;
	char m_name[2];
};
static_assert(sizeof(mk_import_hint_name_entry_t) == 4);
static_assert(sizeof(mk_import_hint_name_entry_t) == 0x4);










bool mk_is_ascii(char const* const& b, char const* const& e)
{
	unsigned char const* it = reinterpret_cast<unsigned char const*>(b);
	unsigned char const* ite = reinterpret_cast<unsigned char const*>(e);
	for(; it != ite; ++it) if(!(*it >= 32 && *it <= 126)) return false;
	return true;
}

bool mk_is_ascii(char const* const& b, unsigned const& len)
{
	return mk_is_ascii(b, b + len);
}

bool find_string(mk_section_headers_t const& section_headers, int const& section_headers_count, mapped_sections_t const& mapped_sections, std::uint32_t const& string_rva, char const** const& string_out, std::uint32_t* const& string_len_out)
{
	int string_section_idx;
	void const* const string_ = find_in_sections(section_headers, section_headers_count, mapped_sections, string_rva, 2, &string_section_idx);
	CHECK_RET(string_, false);
	char const* const string = static_cast<char const*>(string_);
	CHECK_RET(string[0] != '\0', false);
	char const* const string_max = static_cast<char const*>(mapped_sections[string_section_idx].get_view()) + section_headers[string_section_idx].m_size_of_raw_data;
	char const* string_end = string + 1;
	while(string_end != string_max && string_end[0] != '\0') ++string_end;
	std::uint32_t const string_len = static_cast<std::uint32_t>(string_end - string);
	assert(string_len == std::strlen(string)); // TODO: Remove in release.
	CHECK_RET(mk_is_ascii(string, string_len), false);
	*string_out = string;
	*string_len_out = string_len;
	return true;
}










bool find_dll(char const* const& dll_name, std::uint32_t const& dll_name_len, wchar_t const** const& dll_path, int* const& dll_path_len)
{
	return true;
}










template<typename t>
t mk_read(void const* const& ptr, int const offset_bytes = 0)
{
	t e;
	std::memcpy(&e, static_cast<unsigned char const*>(ptr) + offset_bytes, sizeof(e));
	return e;
}

bool process_hint_name_entry(mk_section_headers_t const& section_headers, int const& section_headers_count, mapped_sections_t const& mapped_sections, std::uint32_t const& hint_name_entry_rva, std::uint16_t* const& hint_out, char const** const& name_out, std::uint32_t* const& name_len_out)
{
	int section_idx;
	void const* const hint_name_entry_ = find_in_sections(section_headers, section_headers_count, mapped_sections, hint_name_entry_rva, sizeof(mk_import_hint_name_entry_t), &section_idx);
	CHECK_RET(hint_name_entry_, false);
	std::uint16_t const hint = mk_read<std::uint16_t>(hint_name_entry_);

	char const* const string_begin = static_cast<char const*>(hint_name_entry_) + sizeof(std::uint16_t);
	CHECK_RET(string_begin[0] != '\0', false);
	char const* const string_max = static_cast<char const*>(mapped_sections[section_idx].get_view()) + section_headers[section_idx].m_size_of_raw_data;
	char const* string_end = string_begin + 1;
	while(string_end != string_max && string_end[0] != '\0') ++string_end;
	std::uint32_t const string_len = static_cast<std::uint32_t>(string_end - string_begin);
	assert(string_len == std::strlen(string_begin)); // TODO: Remove in release.
	CHECK_RET(mk_is_ascii(string_begin, string_len), false);

	*hint_out = hint;
	*name_out = string_begin;
	*name_len_out = string_len;
	return true;
}

template<typename uint_t>
bool process_import_address_table(mk_section_headers_t const& section_headers, int const& section_headers_count, mapped_sections_t const& mapped_sections, std::uint32_t const& import_address_table_rva)
{
	int import_address_table_section_idx;
	void const* const import_address_table_ = find_in_sections(section_headers, section_headers_count, mapped_sections, import_address_table_rva, sizeof(uint_t), &import_address_table_section_idx);
	CHECK_RET(import_address_table_, false);
	unsigned char const* const import_address_table = static_cast<unsigned char const*>(import_address_table_);

	unsigned char const* const section_max = static_cast<unsigned char const*>(mapped_sections[import_address_table_section_idx].get_view()) + section_headers[import_address_table_section_idx].m_size_of_raw_data;
	std::uint32_t const size_left = static_cast<std::uint32_t>(section_max - import_address_table);
	std::uint32_t const elems_end = size_left / sizeof(uint_t) + 1;
	std::uint32_t import_address_table_size = 0;
	while(import_address_table_size != elems_end && mk_read<uint_t>(import_address_table, import_address_table_size * sizeof(uint_t)) != uint_t{}) ++import_address_table_size;
	CHECK_RET(import_address_table_size != elems_end && import_address_table_size <= 0xFFFF / 2, false);

	for(std::uint32_t i = 0; i != import_address_table_size; ++i)
	{
		static constexpr uint_t const s_mask = static_cast<uint_t>(1u) << (sizeof(uint_t) * CHAR_BIT - 1);
		uint_t const value = mk_read<uint_t>(import_address_table, i * sizeof(uint_t));
		bool const flag = (value & s_mask) != 0;
		if(flag)
		{
			uint_t const ordinal_ = value &~ s_mask;
			CHECK_RET(ordinal_ <= 0xFFFFull, false);
			std::uint16_t const ordinal = static_cast<std::uint16_t>(ordinal_);
		}
		else
		{
			uint_t const hint_name_table_rva_ = value;
			CHECK_RET(hint_name_table_rva_ <= 0xFFFFFFFFull, false);
			std::uint32_t const hint_name_table_rva = static_cast<std::uint32_t>(hint_name_table_rva_);
			std::uint16_t hint;
			char const* name;
			std::uint32_t name_len;
			bool const hint_name_processed = process_hint_name_entry(section_headers, section_headers_count, mapped_sections, hint_name_table_rva, &hint, &name, &name_len);
			CHECK_RET(hint_name_processed, false);
		}
	}

	return true;
}

bool process_import_address_table(mk_section_headers_t const& section_headers, int const& section_headers_count, mapped_sections_t const& mapped_sections, bool const& is_32_bit, std::uint32_t const& import_address_table_rva)
{
	if(is_32_bit)
	{
		return process_import_address_table<std::uint32_t>(section_headers, section_headers_count, mapped_sections, import_address_table_rva);
	}
	else
	{
		return process_import_address_table<std::uint64_t>(section_headers, section_headers_count, mapped_sections, import_address_table_rva);
	}
}

bool process_import_directory_table(mk_image_data_directories_t const& image_data_directories, int const& image_data_directories_count, mk_section_headers_t const& section_headers, int const& section_headers_count, mapped_sections_t const& mapped_sections, bool const& is_32_bit)
{
	if(!(image_data_directories_count >= static_cast<int>(mk_image_data_directory_e::import_table) + 1)) return true;
	std::uint32_t const& imports_rva = image_data_directories[static_cast<int>(mk_image_data_directory_e::import_table)].m_rva;
	std::uint32_t const& imports_size = image_data_directories[static_cast<int>(mk_image_data_directory_e::import_table)].m_size;
	if(!(imports_rva != 0 && imports_size != 0)) return true;
	CHECK_RET(imports_size >= sizeof(mk_import_directory_entry_t), false);

	int section_idx;
	void const* const import_directory_table = find_in_sections(section_headers, section_headers_count, mapped_sections, imports_rva, imports_size, &section_idx);
	CHECK_RET(import_directory_table, false);

	std::uint32_t const remaining_bytes = section_headers[section_idx].m_size_of_raw_data - static_cast<std::uint32_t>(static_cast<char const*>(import_directory_table) - static_cast<char const*>(mapped_sections[section_idx].get_view()));
	std::uint32_t const remaining_elements = remaining_bytes / sizeof(mk_import_directory_entry_t);
	bool found_end = false;
	for(std::uint32_t i = 0; i != remaining_elements; ++i)
	{
		mk_import_directory_entry_t const import_directory_entry = mk_read<mk_import_directory_entry_t>(import_directory_table, i * sizeof(mk_import_directory_entry_t));
		if(import_directory_entry == mk_import_directory_entry_t{})
		{
			found_end = true;
			break;
		}

		char const* dll_name;
		std::uint32_t dll_name_len;
		bool const dll_name_found = find_string(section_headers, section_headers_count, mapped_sections, import_directory_entry.m_name, &dll_name, &dll_name_len);
		CHECK_RET(dll_name_found, false);

		wchar_t const* dll_path;
		int dll_path_len;
		bool const dll_found = find_dll(dll_name, dll_name_len, &dll_path, &dll_path_len);
		CHECK_RET(dll_found, false);

		bool const import_address_table_processed = process_import_address_table(section_headers, section_headers_count, mapped_sections, is_32_bit, import_directory_entry.m_import_lookup_table_rva != 0 ? import_directory_entry.m_import_lookup_table_rva : import_directory_entry.m_import_address_table_rva);
		CHECK_RET(import_address_table_processed, false);
	}
	CHECK_RET(found_end, false);

	return true;
}










bool loader_load(read_only_file_mapping_t const& mapping, std::uint32_t const& file_size, read_only_map_view_of_file_t& view, std::uint32_t const& view_size)
{
	// Gotchas:
	// UNACEV2.DLL (TotalCommander). One section reports weird raw size.
	// ApiMonitor. Hight ordinal offset.
	// Import address table vs import lookup table. Some DLLs first one one some DLLs use second one.
	// c:\Windows\System32\ir41_32original.dll. Imports DLL but zero functions from it.
	bool is_32_bit;
	mk_image_data_directories_t image_data_directories;
	int image_data_directories_count;
	mk_section_headers_t section_headers;
	int section_headers_count;
	bool const pe_parsed = parse_pe(view.get_view(), view_size, &image_data_directories, &image_data_directories_count, &section_headers, &section_headers_count, &is_32_bit);
	CHECK_RET(pe_parsed, false);
	view.reset();

	mapped_sections_t mapped_sections;
	bool const mapped = map_sections(file_size, mapping, section_headers, section_headers_count, &mapped_sections);
	CHECK_RET(mapped, false);

	bool const imports_processed = process_import_directory_table(image_data_directories, image_data_directories_count, section_headers, section_headers_count, mapped_sections, is_32_bit);
	CHECK_RET(imports_processed, false);

	return true;
}

void process_file_impl(wchar_t const* const& path)
{
	read_only_file_t const file = read_only_file_t::make(path);
	if(!file) return;

	std::uint64_t const file_size_ = get_file_size(file);
	if(file_size_ == 0 || file_size_ > 2ull * 1024ull * 1024ull * 1024ull - 64ull * 1024ull) return;
	std::uint32_t const file_size = static_cast<std::uint32_t>(file_size_);

	read_only_file_mapping_t const mapping = read_only_file_mapping_t::make(file);
	CHECK_RET_V(mapping);

	std::uint32_t const view_size = (std::min)(file_size, static_cast<std::uint32_t>(2 * 64 * 1024));
	read_only_map_view_of_file_t view = read_only_map_view_of_file_t::make(mapping, 0, view_size);
	CHECK_RET_V(view);

	if(!is_pe(view.get_view(), view_size)) return;

	bool const loaded = loader_load(mapping, file_size, view, view_size);
	CHECK_RET_V(loaded);
}

void process_file(wchar_t const* const& path, void* const data)
{
	#if defined USE_THREADS && USE_THREADS == 1
	static constexpr auto const fn = [](void* const data)
	{
		wchar_t const* const path = static_cast<wchar_t const*>(data);
		process_file_impl(path);
	};
	static constexpr void(*const fn_)(void*) = fn;

	work_pool_t& wp = *static_cast<work_pool_t*>(data);
	void* const data_ = static_cast<void*>(const_cast<wchar_t*>(path));
	wp.submit(fn_, data_);
	#else
	process_file_impl(path);
	(void)data;
	#endif
}

struct mk_false_t
{
	static constexpr bool const value = false;
};
struct mk_true_t
{
	static constexpr bool const value = true;
};

template<typename t>
struct mk_is_array
{
	typedef mk_false_t type;
};
template<typename t, unsigned n>
struct mk_is_array<t[n]>
{
	typedef mk_true_t type;
};

void process_folder(wchar_t const* const& path, int const& path_len, void* const data)
{
	std::array<wchar_t, 32 * 1024> file_name_buff;
	std::memcpy(file_name_buff.data(), path, path_len * sizeof(wchar_t));
	file_name_buff[path_len] = L'\0';
	crawl_all_files(file_name_buff, [](wchar_t const* const& path, int const&, void* const data)
	{
		process_file(path, data);
	}, data);
}

template<unsigned N>
void process_folder_impl(wchar_t const(&path)[N], void* const data, mk_true_t const&)
{
	process_folder(path, static_cast<int>(N - 1), data);
}

void process_folder_impl(wchar_t const* const& path, void* const data, mk_false_t const&)
{
	int const path_len = static_cast<int>(std::wcslen(path));
	process_folder(path, path_len, data);
}

template<typename tt>
void process_folder(tt const& t, void* const data)
{
	process_folder_impl(t, data, typename mk_is_array<tt>::type{});
}










int wmain(int const argc, wchar_t const* argv[])
{
	#if defined USE_THREADS && USE_THREADS == 1
	work_pool_t wp;
	void* const param = &wp;
	#else
	void* const param = nullptr;
	#endif
	if(argc >= 2)
	{
		for(int i = 1; i != argc; ++i)
		{
			process_folder(argv[i], param);
		}
	}
	return EXIT_SUCCESS;
}
