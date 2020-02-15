#include "coff_optional_windows.h"

#include "coff.h"
#include "coff_optional_standard.h"
#include "mz.h"

#include "../assert_my.h"

#include <algorithm>
#include <iterator>


static constexpr std::uint16_t s_image_subsystem_unknown                 	= 0; // IMAGE_SUBSYSTEM_UNKNOWN                  	An unknown subsystem
static constexpr std::uint16_t s_image_subsystem_native                  	= 1; // IMAGE_SUBSYSTEM_NATIVE                   	Device drivers and native Windows processes
static constexpr std::uint16_t s_image_subsystem_windows_gui             	= 2; // IMAGE_SUBSYSTEM_WINDOWS_GUI              	The Windows graphical user interface (GUI) subsystem
static constexpr std::uint16_t s_image_subsystem_windows_cui             	= 3; // IMAGE_SUBSYSTEM_WINDOWS_CUI              	The Windows character subsystem
static constexpr std::uint16_t s_image_subsystem_os2_cui                 	= 5; // IMAGE_SUBSYSTEM_OS2_CUI                  	The OS/2 character subsystem
static constexpr std::uint16_t s_image_subsystem_posix_cui               	= 7; // IMAGE_SUBSYSTEM_POSIX_CUI                	The Posix character subsystem
static constexpr std::uint16_t s_image_subsystem_native_windows          	= 8; // IMAGE_SUBSYSTEM_NATIVE_WINDOWS           	Native Win9x driver
static constexpr std::uint16_t s_image_subsystem_windows_ce_gui          	= 9; // IMAGE_SUBSYSTEM_WINDOWS_CE_GUI           	Windows CE
static constexpr std::uint16_t s_image_subsystem_efi_application         	= 10; // IMAGE_SUBSYSTEM_EFI_APPLICATION         	An Extensible Firmware Interface (EFI) application
static constexpr std::uint16_t s_image_subsystem_efi_boot_service_driver 	= 11; // IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER 	An EFI driver with boot services
static constexpr std::uint16_t s_image_subsystem_efi_runtime_driver      	= 12; // IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER      	An EFI driver with run-time services
static constexpr std::uint16_t s_image_subsystem_efi_rom                 	= 13; // IMAGE_SUBSYSTEM_EFI_ROM                 	An EFI ROM image
static constexpr std::uint16_t s_image_subsystem_xbox                    	= 14; // IMAGE_SUBSYSTEM_XBOX                    	XBOX
static constexpr std::uint16_t s_image_subsystem_windows_boot_application	= 16; // IMAGE_SUBSYSTEM_WINDOWS_BOOT_APPLICATION	Windows boot application.

static constexpr std::uint16_t s_image_subsystems[] =
{
	s_image_subsystem_unknown,
	s_image_subsystem_native,
	s_image_subsystem_windows_gui,
	s_image_subsystem_windows_cui,
	s_image_subsystem_os2_cui,
	s_image_subsystem_posix_cui,
	s_image_subsystem_native_windows,
	s_image_subsystem_windows_ce_gui,
	s_image_subsystem_efi_application,
	s_image_subsystem_efi_boot_service_driver,
	s_image_subsystem_efi_runtime_driver,
	s_image_subsystem_efi_rom,
	s_image_subsystem_xbox,
	s_image_subsystem_windows_boot_application,
};

static constexpr std::uint16_t s_image_dllcharacteristics_xxx_1                	= 0x0001; // IMAGE_DLLCHARACTERISTICS_XXX_1                	Reserved, must be zero.
static constexpr std::uint16_t s_image_dllcharacteristics_xxx_2                	= 0x0002; // IMAGE_DLLCHARACTERISTICS_XXX_2                	Reserved, must be zero.
static constexpr std::uint16_t s_image_dllcharacteristics_xxx_3                	= 0x0004; // IMAGE_DLLCHARACTERISTICS_XXX_3                	Reserved, must be zero.
static constexpr std::uint16_t s_image_dllcharacteristics_xxx_4                	= 0x0008; // IMAGE_DLLCHARACTERISTICS_XXX_4                	Reserved, must be zero.
static constexpr std::uint16_t s_image_dllcharacteristics_xxx_5                	= 0x0010; // IMAGE_DLLCHARACTERISTICS_XXX_5                	xxx
static constexpr std::uint16_t s_image_dllcharacteristics_high_entropy_va      	= 0x0020; // IMAGE_DLLCHARACTERISTICS_HIGH_ENTROPY_VA      	Image can handle a high entropy 64-bit virtual address space.
static constexpr std::uint16_t s_image_dllcharacteristics_dynamic_base         	= 0x0040; // IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE         	DLL can be relocated at load time.
static constexpr std::uint16_t s_image_dllcharacteristics_force_integrity      	= 0x0080; // IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY      	Code Integrity checks are enforced.
static constexpr std::uint16_t s_image_dllcharacteristics_nx_compat            	= 0x0100; // IMAGE_DLLCHARACTERISTICS_NX_COMPAT            	Image is NX compatible.
static constexpr std::uint16_t s_image_dllcharacteristics_no_isolation         	= 0x0200; // IMAGE_DLLCHARACTERISTICS_NO_ISOLATION         	Isolation aware, but do not isolate the image.
static constexpr std::uint16_t s_image_dllcharacteristics_no_seh               	= 0x0400; // IMAGE_DLLCHARACTERISTICS_NO_SEH               	Does not use structured exception (SE) handling. No SE handler may be called in this image.
static constexpr std::uint16_t s_image_dllcharacteristics_no_bind              	= 0x0800; // IMAGE_DLLCHARACTERISTICS_NO_BIND              	Do not bind the image.
static constexpr std::uint16_t s_image_dllcharacteristics_appcontainer         	= 0x1000; // IMAGE_DLLCHARACTERISTICS_APPCONTAINER         	Image must execute in an AppContainer.
static constexpr std::uint16_t s_image_dllcharacteristics_wdm_driver           	= 0x2000; // IMAGE_DLLCHARACTERISTICS_WDM_DRIVER           	A WDM driver.
static constexpr std::uint16_t s_image_dllcharacteristics_guard_cf             	= 0x4000; // IMAGE_DLLCHARACTERISTICS_GUARD_CF             	Image supports Control Flow Guard.
static constexpr std::uint16_t s_image_dllcharacteristics_terminal_server_aware	= 0x8000; // IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE	Terminal Server aware.

static constexpr std::uint16_t s_image_dllcharacteristics[] =
{
	s_image_dllcharacteristics_xxx_1,
	s_image_dllcharacteristics_xxx_2,
	s_image_dllcharacteristics_xxx_3,
	s_image_dllcharacteristics_xxx_4,
	s_image_dllcharacteristics_xxx_5,
};


bool is_power_of_two(std::uint32_t const& n);


pe_e_parse_coff_optional_header_windows_32_64 pe_parse_coff_optional_header_windows_32_64(std::byte const* const file_data, int const file_size, pe_coff_optional_header_windows_32_64 const** const header_out)
{
	pe_dos_header const& dosheader = *reinterpret_cast<pe_dos_header const*>(file_data + 0);
	pe_coff_header const& coff_hdr = *reinterpret_cast<pe_coff_header const*>(file_data + dosheader.m_pe_offset);
	pe_coff_optional_header_standard_32_64 const& coff_opt_std = *reinterpret_cast<pe_coff_optional_header_standard_32_64 const*>(file_data + dosheader.m_pe_offset + sizeof(pe_coff_header));
	bool const is_32 = pe_is_32_bit(coff_opt_std.m_32);
	WARN_M_R(coff_hdr.m_optional_header_size >= sizeof(pe_coff_optional_header_standard_32_64) + sizeof(pe_coff_optional_header_windows_32_64), L"COFF header contains too small size of coff_optional_header_windows_32_64.", pe_e_parse_coff_optional_header_windows_32_64::coff_has_wrong_optional);
	WARN_M_R(file_size >= static_cast<int>(dosheader.m_pe_offset + sizeof(pe_coff_header) + sizeof(pe_coff_optional_header_standard_32_64) + sizeof(pe_coff_optional_header_windows_32_64)), L"File is too small to contain coff_optional_header_windows_32_64.", pe_e_parse_coff_optional_header_windows_32_64::file_too_small);
	pe_coff_optional_header_windows_32_64 const& header = *reinterpret_cast<pe_coff_optional_header_windows_32_64 const*>(file_data + dosheader.m_pe_offset + sizeof(pe_coff_header) + (is_32 ? sizeof(pe_coff_optional_header_standard_32) : sizeof(pe_coff_optional_header_standard_64)));
	WARN_M(((is_32 ? header.m_32.m_image_base : header.m_64.m_image_base) & (64 * 1024 - 1)) == 0, L"ImageBase must a multiple of 64kB.");
	WARN_M((is_32 ? header.m_32.m_section_alignment : header.m_64.m_section_alignment) >= (is_32 ? header.m_32.m_file_alignment : header.m_64.m_file_alignment), L"SectionAlignment must be greater or equal to FileAlignment.");
	WARN_M(is_power_of_two(is_32 ? header.m_32.m_file_alignment : header.m_64.m_file_alignment), L"FileAlignment should be a power of 2 between 512 and 64 K, inclusive.");
	WARN_M((is_32 ? header.m_32.m_file_alignment : header.m_64.m_file_alignment) >= 512, L"FileAlignment should be a power of 2 between 512 and 64 K, inclusive.");
	WARN_M((is_32 ? header.m_32.m_file_alignment : header.m_64.m_file_alignment) <= (64 * 1024), L"FileAlignment should be a power of 2 between 512 and 64 K, inclusive.");
	WARN_M((is_32 ? header.m_32.m_win32version : header.m_64.m_win32version) == 0, L"Win32VersionValue is reserved, must be zero.");
	WARN_M((is_32 ? header.m_32.m_section_alignment : header.m_64.m_section_alignment) == 0 || (is_32 ? header.m_32.m_image_size : header.m_64.m_image_size) % (is_32 ? header.m_32.m_section_alignment : header.m_64.m_section_alignment) == 0, L"SizeOfImage must be a multiple of SectionAlignment.");
	WARN_M(std::find(std::cbegin(s_image_subsystems), std::cend(s_image_subsystems), is_32 ? header.m_32.m_subsystem : header.m_64.m_subsystem) != std::end(s_image_subsystems), L"Unknown subsystem.");
	WARN_M(std::find(std::cbegin(s_image_dllcharacteristics), std::cend(s_image_dllcharacteristics), is_32 ? header.m_32.m_dll_characteristics : header.m_64.m_dll_characteristics) == std::end(s_image_dllcharacteristics), L"Unknown DLL characteristics.");
	WARN_M((is_32 ? header.m_32.m_loader_flags : header.m_64.m_loader_flags) == 0, L"LoaderFlags is reserved, must be zero.");
	*header_out = &header;
	return pe_e_parse_coff_optional_header_windows_32_64::ok;
}


bool is_power_of_two(std::uint32_t const& n)
{
	return n == 0 || (n & (n - 1)) == 0;
}
