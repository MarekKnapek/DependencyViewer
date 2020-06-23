// ========== ==========
// library begin
// ========== ==========


#include <algorithm> // min
#include <array> // array
#include <cassert> // assert
#include <cstdint> // uint64_t, uint32_t, uint16_t, uint8_t, uintptr_t
#include <cstdlib> // abort
#include <cstring> // memcpy, memcmp
#include <utility> // swap, pair

#include <windows.h>










#define MK_CHECK_RET(A, B) do{ if(!(A)) [[unlikely]] { mk::MK_CHECK_RET_failed(); return B; } }while(false)
#define MK_CHECK_RET_V(A) do{ if(!(A)) [[unlikely]] { mk::MK_CHECK_RET_failed(); return; } }while(false)










namespace mk
{

	void MK_CHECK_RET_failed();

}










void mk::MK_CHECK_RET_failed()
{
	if(IsDebuggerPresent())
	{
		DebugBreak();
	}
}










namespace mk
{

	bool is_ascii(char const* const& b, char const* const& e);
	bool is_ascii(char const* const& b, int const& len);

}










bool mk::is_ascii(char const* const& b, char const* const& e)
{
	unsigned char const* it = reinterpret_cast<unsigned char const*>(b);
	unsigned char const* const ite = reinterpret_cast<unsigned char const*>(e);
	for(; it != ite; ++it) if(!(*it >= 32 && *it <= 126)) return false;
	return true;
}

bool mk::is_ascii(char const* const& b, int const& len)
{
	return is_ascii(b, b + len);
}










namespace mk
{

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

}










mk::read_only_file_t::read_only_file_t() noexcept :
	m_handle(INVALID_HANDLE_VALUE)
{
}

mk::read_only_file_t mk::read_only_file_t::make(wchar_t const* const& file_path)
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

mk::read_only_file_t::read_only_file_t(read_only_file_t&& other) noexcept :
	read_only_file_t()
{
	swap(other);
}

mk::read_only_file_t& mk::read_only_file_t::operator=(read_only_file_t&& other) noexcept
{
	swap(other);
	return *this;
}

mk::read_only_file_t::~read_only_file_t() noexcept
{
	if(m_handle == INVALID_HANDLE_VALUE) return;
	BOOL const closed = CloseHandle(m_handle);
	assert(closed != 0);
}

void mk::read_only_file_t::swap(read_only_file_t& other) noexcept
{
	using std::swap;
	swap(m_handle, other.m_handle);
}

mk::read_only_file_t::operator bool() const
{
	return m_handle != INVALID_HANDLE_VALUE;
}

void mk::read_only_file_t::reset()
{
	*this = read_only_file_t{};
}

HANDLE const& mk::read_only_file_t::get_handle() const
{
	return m_handle;
}

mk::read_only_file_t::read_only_file_t(HANDLE const& h) :
	read_only_file_t()
{
	m_handle = h;
}










namespace mk
{

	std::uint64_t get_file_size(read_only_file_t const& file);

}










std::uint64_t mk::get_file_size(mk::read_only_file_t const& file)
{
	assert(file);
	LARGE_INTEGER size;
	BOOL const got = GetFileSizeEx(file.get_handle(), &size);
	MK_CHECK_RET(got != 0, 0);
	MK_CHECK_RET(size.QuadPart >= 0, 0);
	return static_cast<std::uint64_t>(size.QuadPart);
}










namespace mk
{

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

}










mk::read_only_file_mapping_t::read_only_file_mapping_t() noexcept :
	m_handle()
{
}

mk::read_only_file_mapping_t mk::read_only_file_mapping_t::make(read_only_file_t const& file)
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

mk::read_only_file_mapping_t::read_only_file_mapping_t(read_only_file_mapping_t&& other) noexcept :
	read_only_file_mapping_t()
{
	swap(other);
}

mk::read_only_file_mapping_t& mk::read_only_file_mapping_t::operator=(read_only_file_mapping_t&& other) noexcept
{
	swap(other);
	return *this;
}

mk::read_only_file_mapping_t::~read_only_file_mapping_t() noexcept
{
	if(m_handle == HANDLE{}) return;
	BOOL const closed = CloseHandle(m_handle);
	assert(closed != 0);
}

void mk::read_only_file_mapping_t::swap(read_only_file_mapping_t& other) noexcept
{
	using std::swap;
	swap(m_handle, other.m_handle);
}

mk::read_only_file_mapping_t::operator bool() const
{
	return m_handle != HANDLE{};
}

void mk::read_only_file_mapping_t::reset()
{
	*this = read_only_file_mapping_t{};
}

HANDLE const& mk::read_only_file_mapping_t::get_handle() const
{
	return m_handle;
}

mk::read_only_file_mapping_t::read_only_file_mapping_t(HANDLE const& h) :
	read_only_file_mapping_t()
{
	m_handle = h;
}










namespace mk
{

	std::pair<int, int> get_allocation_granularity();

}










std::pair<int, int> mk::get_allocation_granularity()
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	if(si.dwPageSize != 4 * 1024) std::abort();
	if(si.dwAllocationGranularity != 64 * 1024) std::abort();
	int const page_size = static_cast<int>(si.dwPageSize);
	int const allocation_granularity = static_cast<int>(si.dwAllocationGranularity);
	return {page_size, allocation_granularity};
}










namespace mk
{

	static std::pair<int, int> const s_allocation_granularity = get_allocation_granularity();

}










namespace mk
{

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

}










mk::read_only_map_view_of_file_t::read_only_map_view_of_file_t() noexcept :
	m_view()
{
}

mk::read_only_map_view_of_file_t mk::read_only_map_view_of_file_t::make(read_only_file_mapping_t const& mapping, unsigned const& offset, unsigned const& length)
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

mk::read_only_map_view_of_file_t::read_only_map_view_of_file_t(read_only_map_view_of_file_t&& other) noexcept :
	read_only_map_view_of_file_t()
{
	swap(other);
}

mk::read_only_map_view_of_file_t& mk::read_only_map_view_of_file_t::operator=(read_only_map_view_of_file_t&& other) noexcept
{
	swap(other);
	return *this;
}

mk::read_only_map_view_of_file_t::~read_only_map_view_of_file_t() noexcept
{
	if(m_view == nullptr) return;
	void const* const ptr = reinterpret_cast<void const*>(reinterpret_cast<std::uintptr_t>(m_view) &~ static_cast<std::uintptr_t>(0xFFFFull));
	BOOL const unmapped = UnmapViewOfFile(ptr);
	assert(unmapped != 0);
}

void mk::read_only_map_view_of_file_t::swap(read_only_map_view_of_file_t& other) noexcept
{
	using std::swap;
	swap(m_view, other.m_view);
}

mk::read_only_map_view_of_file_t::operator bool() const
{
	return m_view != nullptr;
}

void mk::read_only_map_view_of_file_t::reset()
{
	*this = read_only_map_view_of_file_t{};
}

void const* const& mk::read_only_map_view_of_file_t::get_view() const
{
	return m_view;
}

mk::read_only_map_view_of_file_t::read_only_map_view_of_file_t(void const* const& v) :
	read_only_map_view_of_file_t()
{
	m_view = v;
}










namespace mk
{

	template<typename t>
	t read_binary(void const* const& ptr);
	template<typename t>
	t read_binary(void const* const& ptr, unsigned const& offset);

}










template<typename t>
t mk::read_binary(void const* const& ptr)
{
	return read_binary<t>(ptr, 0);
}

template<typename t>
t mk::read_binary(void const* const& ptr, unsigned const& offset)
{
	t obj;
	std::memcpy(&obj, static_cast<unsigned char const*>(ptr) + offset, sizeof(obj));
	return obj;
}










namespace mk
{

	struct coff_file_header_t
	{
		std::uint16_t m_machine;
		std::uint16_t m_number_of_sections;
		std::uint32_t m_time_date_stamp;
		std::uint32_t m_pointer_to_symbol_table;
		std::uint32_t m_number_of_symbols;
		std::uint16_t m_size_of_optional_header;
		std::uint16_t m_characteristics;
	};
	static_assert(sizeof(coff_file_header_t) == 20);
	static_assert(sizeof(coff_file_header_t) == 0x14);

	struct coff_optional_header_pe32_standard_t
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
	static_assert(sizeof(coff_optional_header_pe32_standard_t) == 28);
	static_assert(sizeof(coff_optional_header_pe32_standard_t) == 0x1c);

	struct coff_optional_header_pe32plus_standard_t
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
	static_assert(sizeof(coff_optional_header_pe32plus_standard_t) == 24);
	static_assert(sizeof(coff_optional_header_pe32plus_standard_t) == 0x18);

	struct coff_optional_header_pe32_windows_t
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
	static_assert(sizeof(coff_optional_header_pe32_windows_t) == 68, "");
	static_assert(sizeof(coff_optional_header_pe32_windows_t) == 0x44, "");

	struct coff_optional_header_pe32plus_windows_t
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
	static_assert(sizeof(coff_optional_header_pe32plus_windows_t) == 88, "");
	static_assert(sizeof(coff_optional_header_pe32plus_windows_t) == 0x58, "");

	struct image_data_directory_t
	{
		std::uint32_t m_rva;
		std::uint32_t m_size;
	};
	static_assert(sizeof(image_data_directory_t) == 8, "");
	static_assert(sizeof(image_data_directory_t) == 0x8, "");
	static constexpr int const s_image_data_directories_count_max = 16;
	typedef std::array<image_data_directory_t, s_image_data_directories_count_max> image_data_directories_t;
	enum class image_data_directory_e
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
	struct section_header_t
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
	static_assert(sizeof(section_header_t) == 40, "");
	static_assert(sizeof(section_header_t) == 0x28, "");
	static constexpr int const s_section_header_count_max = 96;
	typedef std::array<section_header_t, s_section_header_count_max> section_headers_t;
	enum class section_header_characteristics_e : std::uint32_t
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

}










namespace mk
{

	bool is_pe(void const* const& ptr, int const& len);
	bool parse_pe(void const* const& ptr, int const& len, image_data_directories_t* const& image_data_directories_out, int* const& image_data_directories_count_out, section_headers_t* const& section_headers_out, int* const section_headers_count_out, bool* const& is_32_bit_out);

}










bool mk::is_pe(void const* const& ptr, int const& len)
{
	unsigned char const* const data = static_cast<unsigned char const*>(ptr);
	std::uint32_t const file_size = static_cast<std::uint32_t>(len);

	static constexpr std::uint32_t const s_mz_signature_offset = 0;
	if(!(file_size >= s_mz_signature_offset + sizeof(std::uint16_t))) return false;
	std::uint16_t const mz_signature = read_binary<std::uint16_t>(data, s_mz_signature_offset);
	static constexpr std::uint16_t const s_mz_signature = 0x5A4D; // MZ
	if(!(mz_signature == s_mz_signature)) return false;
	
	static constexpr std::uint32_t const s_pe_offset_offset = 0x3c;
	if(!(file_size >= s_pe_offset_offset + sizeof(std::uint16_t))) return false;
	std::uint16_t const pe_offset = read_binary<std::uint16_t>(data, s_pe_offset_offset);

	if(!(file_size >= pe_offset + sizeof(std::uint32_t))) return false;
	std::uint32_t const pe_signature = read_binary<std::uint32_t>(data, pe_offset);
	static constexpr std::uint32_t const s_pe_signature = 0x00004550; // PE\0\0
	if(!(pe_signature == s_pe_signature)) return false;

	return true;
}

bool mk::parse_pe(void const* const& ptr, int const& len, image_data_directories_t* const& image_data_directories_out, int* const& image_data_directories_count_out, section_headers_t* const& section_headers_out, int* const section_headers_count_out, bool* const& is_32_bit_out)
{
	unsigned char const* const data = static_cast<unsigned char const*>(ptr);
	std::uint32_t const file_size = static_cast<std::uint32_t>(len);

	static constexpr std::uint32_t const s_mz_signature_offset = 0;
	MK_CHECK_RET(file_size >= s_mz_signature_offset + sizeof(std::uint16_t), false);
	std::uint16_t const mz_signature = read_binary<std::uint16_t>(data, s_mz_signature_offset);
	static constexpr std::uint16_t const s_mz_signature = 0x5A4D; // MZ
	MK_CHECK_RET(mz_signature == s_mz_signature, false);
	
	static constexpr std::uint32_t const s_pe_offset_offset = 0x3c;
	MK_CHECK_RET(file_size >= s_pe_offset_offset + sizeof(std::uint16_t), false);
	std::uint16_t const pe_offset = read_binary<std::uint16_t>(data, s_pe_offset_offset);

	MK_CHECK_RET(file_size >= pe_offset + sizeof(std::uint32_t), false);
	std::uint32_t const pe_signature = read_binary<std::uint32_t>(data, pe_offset);
	static constexpr std::uint32_t const s_pe_signature = 0x00004550; // PE\0\0
	MK_CHECK_RET(pe_signature == s_pe_signature, false);

	std::uint32_t const coff_file_header_offset = pe_offset + sizeof(pe_signature);
	MK_CHECK_RET(file_size >= coff_file_header_offset + sizeof(coff_file_header_t), false);
	coff_file_header_t const coff_file_header = read_binary<coff_file_header_t>(data, coff_file_header_offset);

	std::uint32_t const optional_header_signature_offset = coff_file_header_offset + sizeof(coff_file_header);
	MK_CHECK_RET(file_size >= optional_header_signature_offset + sizeof(std::uint16_t), false);
	std::uint16_t const optional_header_signature = read_binary<std::uint16_t>(data, optional_header_signature_offset);
	static constexpr std::uint16_t const s_optional_header_signature_pe32 = 0x010b;
	static constexpr std::uint16_t const s_optional_header_signature_pe32plus = 0x020b;
	MK_CHECK_RET(optional_header_signature == s_optional_header_signature_pe32 || optional_header_signature == s_optional_header_signature_pe32plus, false);

	std::uint32_t number_of_rva_snd_sizes;
	std::uint32_t image_data_directory_offset;
	if(optional_header_signature == s_optional_header_signature_pe32)
	{
		std::uint32_t const optional_header_standard_offset = optional_header_signature_offset;
		MK_CHECK_RET(file_size >= optional_header_standard_offset + sizeof(coff_optional_header_pe32_standard_t), false);
		coff_optional_header_pe32_standard_t const optional_header_standard = read_binary<coff_optional_header_pe32_standard_t>(data, optional_header_standard_offset);

		std::uint32_t const optional_header_windows_offset = optional_header_standard_offset + sizeof(optional_header_standard);
		MK_CHECK_RET(file_size >= optional_header_windows_offset + sizeof(coff_optional_header_pe32_windows_t), false);
		coff_optional_header_pe32_windows_t const optional_header_windows = read_binary<coff_optional_header_pe32_windows_t>(data, optional_header_windows_offset);

		MK_CHECK_RET(optional_header_windows.m_number_of_rva_snd_sizes <= s_image_data_directories_count_max, false);
		number_of_rva_snd_sizes = optional_header_windows.m_number_of_rva_snd_sizes;
		image_data_directory_offset = optional_header_windows_offset + sizeof(optional_header_windows);
	}
	else
	{
		std::uint32_t const optional_header_standard_offset = optional_header_signature_offset;
		MK_CHECK_RET(file_size >= optional_header_standard_offset + sizeof(coff_optional_header_pe32plus_standard_t), false);
		coff_optional_header_pe32plus_standard_t const optional_header_standard = read_binary<coff_optional_header_pe32plus_standard_t>(data, optional_header_standard_offset);

		std::uint32_t const optional_header_windows_offset = optional_header_standard_offset + sizeof(optional_header_standard);
		MK_CHECK_RET(file_size >= optional_header_windows_offset + sizeof(coff_optional_header_pe32plus_windows_t), false);
		coff_optional_header_pe32plus_windows_t const optional_header_windows = read_binary<coff_optional_header_pe32plus_windows_t>(data, optional_header_windows_offset);

		MK_CHECK_RET(optional_header_windows.m_number_of_rva_snd_sizes <= s_image_data_directories_count_max, false);
		number_of_rva_snd_sizes = optional_header_windows.m_number_of_rva_snd_sizes;
		image_data_directory_offset = optional_header_windows_offset + sizeof(optional_header_windows);
	}
	MK_CHECK_RET(file_size >= image_data_directory_offset + number_of_rva_snd_sizes * sizeof(image_data_directory_t), false);

	MK_CHECK_RET(coff_file_header.m_number_of_sections <= s_section_header_count_max, false);
	std::uint32_t const section_headers_offset = image_data_directory_offset + number_of_rva_snd_sizes * sizeof(image_data_directory_t);
	MK_CHECK_RET(coff_file_header.m_number_of_sections <= s_section_header_count_max, false);
	MK_CHECK_RET(file_size >= section_headers_offset + coff_file_header.m_number_of_sections * sizeof(image_data_directory_t), false);

	for(std::uint32_t i = 0; i != number_of_rva_snd_sizes; ++i) image_data_directories_out->operator[](i) = read_binary<image_data_directory_t>(data, image_data_directory_offset + i * sizeof(image_data_directory_t));
	*image_data_directories_count_out = static_cast<int>(number_of_rva_snd_sizes);
	for(std::uint16_t i = 0; i != coff_file_header.m_number_of_sections; ++i) section_headers_out->operator[](i) = read_binary<section_header_t>(data, section_headers_offset + i * sizeof(section_header_t));
	*section_headers_count_out = static_cast<int>(coff_file_header.m_number_of_sections);
	*is_32_bit_out = optional_header_signature == s_optional_header_signature_pe32;
	return true;
}










namespace mk
{

	typedef std::array<read_only_map_view_of_file_t, s_section_header_count_max> mapped_sections_t;

}










namespace mk
{

	bool map_sections(std::uint32_t const& file_size, read_only_file_mapping_t const& mapping, section_headers_t const& section_headers, int const& count, mapped_sections_t* const& mapped_sections_out);
	void const* find_in_sections(section_headers_t const& section_headers, int const& count, mapped_sections_t const& mapped_sections, std::uint32_t const& rva, std::uint32_t const& size, int* const section_idx_out);
	bool find_string(section_headers_t const& section_headers, int const& section_headers_count, mapped_sections_t const& mapped_sections, std::uint32_t const& string_rva, char const** const& string_out, std::uint32_t* const& string_len_out);

}










bool mk::map_sections(std::uint32_t const& file_size, read_only_file_mapping_t const& mapping, section_headers_t const& section_headers, int const& count, mapped_sections_t* const& mapped_sections_out)
{
	static constexpr auto const is_interesting_section = [](section_header_t const& section_header) -> bool
	{
		bool const read = (section_header.m_characteristics & static_cast<std::uint32_t>(section_header_characteristics_e::read)) != 0;
		bool const code = (section_header.m_characteristics & static_cast<std::uint32_t>(section_header_characteristics_e::code)) != 0;
		bool const initialized_data = (section_header.m_characteristics & static_cast<std::uint32_t>(section_header_characteristics_e::initialized_data)) != 0;
		bool const uninitialized_data = (section_header.m_characteristics & static_cast<std::uint32_t>(section_header_characteristics_e::uninitialized_data)) != 0;
		bool const ret = read && (code || initialized_data) && !uninitialized_data;
		return ret;
	};

	assert(mapped_sections_out);
	mapped_sections_t& mapped_sections = *mapped_sections_out;
	for(int i = 0; i != count; ++i)
	{
		section_header_t const& section_header = section_headers[i];
		if(is_interesting_section(section_header))
		{
			MK_CHECK_RET(file_size >= section_header.m_pointer_to_raw_data, false);
			MK_CHECK_RET(file_size >= section_header.m_pointer_to_raw_data + section_header.m_size_of_raw_data, false); // TODO: Overflow?
			mapped_sections[i] = read_only_map_view_of_file_t::make(mapping, section_header.m_pointer_to_raw_data, section_header.m_size_of_raw_data);
			MK_CHECK_RET(mapped_sections[i], false);
		}
	}
	return true;
}

void const* mk::find_in_sections(section_headers_t const& section_headers, int const& count, mapped_sections_t const& mapped_sections, std::uint32_t const& rva, std::uint32_t const& size, int* const section_idx_out)
{
	MK_CHECK_RET(rva != 0, nullptr);
	for(int i = 0; i != count; ++i)
	{
		read_only_map_view_of_file_t const& view = mapped_sections[i];
		if(!view) continue;
		section_header_t const& section_header = section_headers[i];
		if(rva >= section_header.m_virtual_address && rva + size <= section_header.m_virtual_address + section_header.m_size_of_raw_data)
		{
			void const* const v = view.get_view();
			std::uint32_t const offset = rva - section_header.m_virtual_address;
			void const* const address = static_cast<unsigned char const*>(v) + offset;
			*section_idx_out = i;
			return address;
		}
	}
	MK_CHECK_RET(false, nullptr);
}

bool mk::find_string(section_headers_t const& section_headers, int const& section_headers_count, mapped_sections_t const& mapped_sections, std::uint32_t const& string_rva, char const** const& string_out, std::uint32_t* const& string_len_out)
{
	int section_idx;
	void const* const string_ = find_in_sections(section_headers, section_headers_count, mapped_sections, string_rva, 2, &section_idx);
	MK_CHECK_RET(string_, false);
	char const* const string = static_cast<char const*>(string_);
	MK_CHECK_RET(string[0] != '\0', false);
	char const* const string_max = static_cast<char const*>(mapped_sections[section_idx].get_view()) + section_headers[section_idx].m_size_of_raw_data;
	char const* string_end = string + 1;
	while(string_end != string_max && string_end[0] != '\0') ++string_end;
	std::uint32_t const string_len = static_cast<std::uint32_t>(string_end - string);
	MK_CHECK_RET(is_ascii(string, string_len), false);
	*string_out = string;
	*string_len_out = string_len;
	return true;
}










namespace mk
{

	struct export_directory_entry_t
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
	static_assert(sizeof(export_directory_entry_t) == 40);
	static_assert(sizeof(export_directory_entry_t) == 0x28);

	struct export_address_entry_t
	{
		std::uint32_t m_export_or_forwarder_rva;
	};
	static_assert(sizeof(export_address_entry_t) == 4);
	static_assert(sizeof(export_address_entry_t) == 0x4);

	struct export_name_pointer_entry_t
	{
		std::uint32_t m_rva;
	};
	static_assert(sizeof(export_name_pointer_entry_t) == 4);
	static_assert(sizeof(export_name_pointer_entry_t) == 0x4);

	struct export_ordinal_entry_t
	{
		std::uint16_t m_idx;
	};
	static_assert(sizeof(export_ordinal_entry_t) == 2);
	static_assert(sizeof(export_ordinal_entry_t) == 0x2);

	struct export_name_entry_t
	{
	};

}










namespace mk
{

	bool process_export_directory_table(image_data_directories_t const& image_data_directories, int const& image_data_directories_count, section_headers_t const& section_headers, int const& section_headers_count, mapped_sections_t const& mapped_sections, bool const& is_32_bit);

}










bool mk::process_export_directory_table(image_data_directories_t const& image_data_directories, int const& image_data_directories_count, section_headers_t const& section_headers, int const& section_headers_count, mapped_sections_t const& mapped_sections, bool const& is_32_bit)
{
	if(!(image_data_directories_count >= static_cast<int>(image_data_directory_e::export_table) + 1)) return true;
	std::uint32_t const& exports_rva = image_data_directories[static_cast<int>(image_data_directory_e::export_table)].m_rva;
	std::uint32_t const& exports_size = image_data_directories[static_cast<int>(image_data_directory_e::export_table)].m_size;
	if(!(exports_rva != 0 && exports_size != 0)) return true;
	MK_CHECK_RET(exports_size >= sizeof(export_directory_entry_t), false);

	int export_directory_table_section_idx;
	void const* const export_directory_table = find_in_sections(section_headers, section_headers_count, mapped_sections, exports_rva, exports_size, &export_directory_table_section_idx);
	MK_CHECK_RET(export_directory_table, false);
	export_directory_entry_t const export_directory_entry = read_binary<export_directory_entry_t>(export_directory_table);

	MK_CHECK_RET(export_directory_entry.m_export_address_table_rva != 0 && export_directory_entry.m_address_table_entries != 0, false);
	int export_address_table_section_idx;
	void const* const export_address_table = find_in_sections(section_headers, section_headers_count, mapped_sections, export_directory_entry.m_export_address_table_rva, export_directory_entry.m_address_table_entries * sizeof(export_address_entry_t), &export_address_table_section_idx);
	MK_CHECK_RET(export_address_table, false);
	MK_CHECK_RET(export_directory_entry.m_ordinal_base <= 0xFFFF, false);
	MK_CHECK_RET(export_directory_entry.m_address_table_entries <= 0xFFFF - export_directory_entry.m_ordinal_base, false);
	std::uint16_t const ordinal_base = static_cast<std::uint16_t>(export_directory_entry.m_ordinal_base);
	for(std::uint32_t i = 0; i != export_directory_entry.m_address_table_entries; ++i)
	{
		std::uint16_t const ordinal = ordinal_base + static_cast<std::uint16_t>(i);
		export_address_entry_t const export_address_entry = read_binary<export_address_entry_t>(export_address_table, i * sizeof(export_address_entry_t));
		int export_address_entry_section_idx;
		void const* const export_address_entry_ptr = find_in_sections(section_headers, section_headers_count, mapped_sections, export_address_entry.m_export_or_forwarder_rva, sizeof(std::uint32_t), &export_address_entry_section_idx);
		MK_CHECK_RET(export_address_entry_ptr, false);
		if(export_address_entry.m_export_or_forwarder_rva >= exports_rva && export_address_entry.m_export_or_forwarder_rva < exports_rva + exports_size - static_cast<std::uint32_t>(sizeof(std::uint32_t)))
		{
			std::uint32_t const forwarder_rva = export_address_entry.m_export_or_forwarder_rva;
		}
		else
		{
			std::uint32_t const export_rva = export_address_entry.m_export_or_forwarder_rva;
		}
	}

	return true;
}


// ========== ==========
// library end
// ========== ==========










// ========== ==========
// application begin
// ========== ==========


#if 0
#include <algorithm> // sort, transform
#include <cstdio> // printf
#include <cstdlib> // EXIT_SUCCESS, EXIT_FAILURE
#include <cwchar> // wcscmp
#include <string> // string
#include <variant> // variant
#include <vector> // vector


namespace mk
{

	bool imports(int const& argc, wchar_t const*const* const& argv);

}










bool mk::imports(int const& argc, wchar_t const*const* const& argv)
{
	struct callback_data_t
	{
		struct dll_t
		{
			std::string m_name;
			std::vector<std::variant<std::uint16_t, std::string>> m_imports;
		};
		std::vector<dll_t> m_import_dlls;
	};
	static constexpr import_callback_fn_dll_t const callback_fn_dll = [](import_callback_ctx_t const& ctx, char const* const& dll_name, std::uint32_t const& dll_name_len) -> void
	{
		callback_data_t& data = *static_cast<callback_data_t*>(ctx);
		data.m_import_dlls.push_back({});
		data.m_import_dlls.back().m_name.assign(dll_name, dll_name + dll_name_len);
	};
	static constexpr import_callback_fn_import_ordinal_t const callback_fn_ordinal = [](import_callback_ctx_t const& ctx, std::uint16_t const& ordinal) -> void
	{
		callback_data_t& data = *static_cast<callback_data_t*>(ctx);
		data.m_import_dlls.back().m_imports.push_back({});
		data.m_import_dlls.back().m_imports.back().emplace<0>(ordinal);
	};
	static constexpr import_callback_fn_import_named_t const callback_fn_named = [](import_callback_ctx_t const& ctx, char const* const& import_name, std::uint32_t const& import_name_len) -> void
	{
		callback_data_t& data = *static_cast<callback_data_t*>(ctx);
		data.m_import_dlls.back().m_imports.push_back({});
		data.m_import_dlls.back().m_imports.back().emplace<1>(import_name, import_name + import_name_len);
	};


	MK_CHECK_RET(argc == 2 || argc == 3, false);

	bool sort = false;
	int file_name_idx = 1;
	static constexpr wchar_t const s_param_sort[] = L"/sort";
	if(argc == 3 && std::wcscmp(argv[1], s_param_sort) == 0)
	{
		sort = true;
		file_name_idx = 2;
	}


	read_only_file_t const file = read_only_file_t::make(argv[file_name_idx]);
	MK_CHECK_RET(file, false);
	std::uint64_t const file_size_ = get_file_size(file);
	static constexpr std::uint64_t const s_big_num_2g = 2ull * 1024ull * 1024ull * 1024ull - 64ull * 1024ull;
	MK_CHECK_RET(file_size_ != 0 && file_size_ <= s_big_num_2g, false);
	std::uint32_t const file_size = static_cast<std::uint32_t>(file_size_);
	read_only_file_mapping_t const mapping = read_only_file_mapping_t::make(file);
	MK_CHECK_RET(mapping, false);

	std::uint32_t const view_size = (std::min)(file_size, static_cast<std::uint32_t>(2 * 64 * 1024));
	read_only_map_view_of_file_t view = read_only_map_view_of_file_t::make(mapping, 0, view_size);
	MK_CHECK_RET(view, false);
	bool is_32_bit;
	image_data_directories_t image_data_directories;
	int image_data_directories_count;
	section_headers_t section_headers;
	int section_headers_count;
	bool const pe_parsed = parse_pe(view.get_view(), view_size, &image_data_directories, &image_data_directories_count, &section_headers, &section_headers_count, &is_32_bit);
	MK_CHECK_RET(pe_parsed, false);
	view.reset();

	mapped_sections_t mapped_sections;
	bool const mapped = map_sections(file_size, mapping, section_headers, section_headers_count, &mapped_sections);
	MK_CHECK_RET(mapped, false);

	callback_data_t callback_data;
	import_callback_t const callback = {&callback_data, callback_fn_dll, callback_fn_ordinal, callback_fn_named};
	bool const imports_processed = process_import_directory_table(callback, image_data_directories, image_data_directories_count, section_headers, section_headers_count, mapped_sections, is_32_bit);
	MK_CHECK_RET(imports_processed, false);


	if(sort)
	{
		static constexpr auto const to_lowercase = [](char const& ch) -> char { if(ch >= 'A' && ch <= 'Z') { return 'a' + (ch - 'A'); } else { return ch; } };
		for(auto& e : callback_data.m_import_dlls)
		{
			std::transform(e.m_name.begin(), e.m_name.end(), e.m_name.begin(), to_lowercase);
			std::sort(e.m_imports.begin(), e.m_imports.end(), [](auto const& a, auto const& b){ if(a.index() != b.index()){ return a.index() < b.index(); } else { if(a.index() == 0){ return std::get<0>(a) < std::get<0>(b); } else { return std::get<1>(a) < std::get<1>(b); } } });
		}
		std::sort(callback_data.m_import_dlls.begin(), callback_data.m_import_dlls.end(), [](auto const& a, auto const& b){ return a.m_name < b.m_name; });
	}
	for(auto const& dll : callback_data.m_import_dlls)
	{
		std::printf("%s\n", dll.m_name.c_str());
		for(auto const& import : dll.m_imports)
		{
			if(import.index() == 0)
			{
				std::printf("\t%d (0x%04x)\n", static_cast<int>(std::get<0>(import)), static_cast<unsigned>(std::get<0>(import)));
			}
			else
			{
				std::printf("\t%s\n", std::get<1>(import).c_str());
			}
		}
	}

	return true;
}










int wmain(int const argc, wchar_t const* argv[])
{
	bool const imports = mk::imports(argc, argv);
	MK_CHECK_RET(imports, EXIT_FAILURE);
	return EXIT_SUCCESS;
}


// ========== ==========
// application end
// ========== ==========
#endif
