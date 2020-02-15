#include "coff.h"

#include "mz.h"

#include "../assert_my.h"

#include <algorithm>
#include <iterator>


static constexpr std::uint16_t const s_ne_signature = 0x454E; // NE
static constexpr std::uint32_t const s_coff_signature = 0x00004550; // PE\0\0
static constexpr std::uint16_t const s_max_coff_header_sections = 96;

static constexpr std::uint16_t const s_image_file_machine_unknown  	= 0x0000; // IMAGE_FILE_MACHINE_UNKNOWN  	The contents of this field are assumed to be applicable to any machine type
static constexpr std::uint16_t const s_image_file_machine_i386     	= 0x014c; // IMAGE_FILE_MACHINE_I386     	Intel 386 or later processors and compatible processors
static constexpr std::uint16_t const s_image_file_machine_r4000    	= 0x0166; // IMAGE_FILE_MACHINE_R4000    	MIPS little endian
static constexpr std::uint16_t const s_image_file_machine_wcemipsv2	= 0x0169; // IMAGE_FILE_MACHINE_WCEMIPSV2	MIPS little-endian WCE v2
static constexpr std::uint16_t const s_image_file_machine_sh3      	= 0x01a2; // IMAGE_FILE_MACHINE_SH3      	Hitachi SH3
static constexpr std::uint16_t const s_image_file_machine_sh3dsp   	= 0x01a3; // IMAGE_FILE_MACHINE_SH3DSP   	Hitachi SH3 DSP
static constexpr std::uint16_t const s_image_file_machine_sh4      	= 0x01a6; // IMAGE_FILE_MACHINE_SH4      	Hitachi SH4
static constexpr std::uint16_t const s_image_file_machine_sh5      	= 0x01a8; // IMAGE_FILE_MACHINE_SH5      	Hitachi SH5
static constexpr std::uint16_t const s_image_file_machine_arm      	= 0x01c0; // IMAGE_FILE_MACHINE_ARM      	ARM little endian
static constexpr std::uint16_t const s_image_file_machine_thumb    	= 0x01c2; // IMAGE_FILE_MACHINE_THUMB    	Thumb
static constexpr std::uint16_t const s_image_file_machine_armnt    	= 0x01c4; // IMAGE_FILE_MACHINE_ARMNT    	ARM Thumb-2 little endian
static constexpr std::uint16_t const s_image_file_machine_am33     	= 0x01d3; // IMAGE_FILE_MACHINE_AM33     	Matsushita AM33
static constexpr std::uint16_t const s_image_file_machine_powerpc  	= 0x01f0; // IMAGE_FILE_MACHINE_POWERPC  	Power PC little endian
static constexpr std::uint16_t const s_image_file_machine_powerpcfp	= 0x01f1; // IMAGE_FILE_MACHINE_POWERPCFP	Power PC with floating point support
static constexpr std::uint16_t const s_image_file_machine_ia64     	= 0x0200; // IMAGE_FILE_MACHINE_IA64     	Intel Itanium processor family
static constexpr std::uint16_t const s_image_file_machine_mips16   	= 0x0266; // IMAGE_FILE_MACHINE_MIPS16   	MIPS16
static constexpr std::uint16_t const s_image_file_machine_mipsfpu  	= 0x0366; // IMAGE_FILE_MACHINE_MIPSFPU  	MIPS with FPU
static constexpr std::uint16_t const s_image_file_machine_mipsfpu16	= 0x0466; // IMAGE_FILE_MACHINE_MIPSFPU16	MIPS16 with FPU
static constexpr std::uint16_t const s_image_file_machine_ebc      	= 0x0ebc; // IMAGE_FILE_MACHINE_EBC      	EFI byte code
static constexpr std::uint16_t const s_image_file_machine_riscv32  	= 0x5032; // IMAGE_FILE_MACHINE_RISCV32  	RISC-V 32-bit address space
static constexpr std::uint16_t const s_image_file_machine_riscv64  	= 0x5064; // IMAGE_FILE_MACHINE_RISCV64  	RISC-V 64-bit address space
static constexpr std::uint16_t const s_image_file_machine_riscv128 	= 0x5128; // IMAGE_FILE_MACHINE_RISCV128 	RISC-V 128-bit address space
static constexpr std::uint16_t const s_image_file_machine_amd64    	= 0x8664; // IMAGE_FILE_MACHINE_AMD64    	x64
static constexpr std::uint16_t const s_image_file_machine_m32r     	= 0x9041; // IMAGE_FILE_MACHINE_M32R     	Mitsubishi M32R little endian
static constexpr std::uint16_t const s_image_file_machine_arm64    	= 0xaa64; // IMAGE_FILE_MACHINE_ARM64    	ARM64 little endian

static constexpr std::uint16_t const s_image_file_machines[] =
{
	s_image_file_machine_unknown,
	s_image_file_machine_i386,
	s_image_file_machine_r4000,
	s_image_file_machine_wcemipsv2,
	s_image_file_machine_sh3,
	s_image_file_machine_sh3dsp,
	s_image_file_machine_sh4,
	s_image_file_machine_sh5,
	s_image_file_machine_arm,
	s_image_file_machine_thumb,
	s_image_file_machine_armnt,
	s_image_file_machine_am33,
	s_image_file_machine_powerpc,
	s_image_file_machine_powerpcfp,
	s_image_file_machine_ia64,
	s_image_file_machine_mips16,
	s_image_file_machine_mipsfpu,
	s_image_file_machine_mipsfpu16,
	s_image_file_machine_ebc,
	s_image_file_machine_riscv32,
	s_image_file_machine_riscv64,
	s_image_file_machine_riscv128,
	s_image_file_machine_amd64,
	s_image_file_machine_m32r,
	s_image_file_machine_arm64,
};

static constexpr std::uint16_t s_image_file_relocs_stripped        	= 0x0001; // IMAGE_FILE_RELOCS_STRIPPED        	Image only, Windows CE, and Microsoft Windows NT and later. This indicates that the file does not contain base relocations and must therefore be loaded at its preferred base address. If the base address is not available, the loader reports an error. The default behavior of the linker is to strip base relocations from executable (EXE) files.
static constexpr std::uint16_t s_image_file_executable_image       	= 0x0002; // IMAGE_FILE_EXECUTABLE_IMAGE       	Image only. This indicates that the image file is valid and can be run. If this flag is not set, it indicates a linker error.
static constexpr std::uint16_t s_image_file_line_nums_stripped     	= 0x0004; // IMAGE_FILE_LINE_NUMS_STRIPPED     	COFF line numbers have been removed. This flag is deprecated and should be zero.
static constexpr std::uint16_t s_image_file_local_syms_stripped    	= 0x0008; // IMAGE_FILE_LOCAL_SYMS_STRIPPED    	COFF symbol table entries for local symbols have been removed. This flag is deprecated and should be zero.
static constexpr std::uint16_t s_image_file_aggressive_ws_trim     	= 0x0010; // IMAGE_FILE_AGGRESSIVE_WS_TRIM     	Obsolete. Aggressively trim working set. This flag is deprecated for Windows 2000 and later and must be zero.
static constexpr std::uint16_t s_image_file_large_address_aware    	= 0x0020; // IMAGE_FILE_LARGE_ADDRESS_AWARE    	Application can handle > 2-GB addresses.
static constexpr std::uint16_t s_image_file_xxx                    	= 0x0040; // IMAGE_FILE_BYTES_XXX              	This flag is reserved for future use.
static constexpr std::uint16_t s_image_file_bytes_reversed_lo      	= 0x0080; // IMAGE_FILE_BYTES_REVERSED_LO      	Little endian: the least significant bit (LSB) precedes the most significant bit (MSB) in memory. This flag is deprecated and should be zero.
static constexpr std::uint16_t s_image_file_32bit_machine          	= 0x0100; // IMAGE_FILE_32BIT_MACHINE          	Machine is based on a 32-bit-word architecture.
static constexpr std::uint16_t s_image_file_debug_stripped         	= 0x0200; // IMAGE_FILE_DEBUG_STRIPPED         	Debugging information is removed from the image file.
static constexpr std::uint16_t s_image_file_removable_run_from_swap	= 0x0400; // IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP	If the image is on removable media, fully load it and copy it to the swap file.
static constexpr std::uint16_t s_image_file_net_run_from_swap      	= 0x0800; // IMAGE_FILE_NET_RUN_FROM_SWAP      	If the image is on network media, fully load it and copy it to the swap file.
static constexpr std::uint16_t s_image_file_system                 	= 0x1000; // IMAGE_FILE_SYSTEM                 	The image file is a system file, not a user program.
static constexpr std::uint16_t s_image_file_dll                    	= 0x2000; // IMAGE_FILE_DLL                    	The image file is a dynamic-link library (DLL). Such files are considered executable files for almost all purposes, although they cannot be directly run.
static constexpr std::uint16_t s_image_file_up_system_only         	= 0x4000; // IMAGE_FILE_UP_SYSTEM_ONLY         	The file should be run only on a uniprocessor machine.
static constexpr std::uint16_t s_image_file_bytes_reversed_hi      	= 0x8000; // IMAGE_FILE_BYTES_REVERSED_HI      	Big endian: the MSB precedes the LSB in memory. This flag is deprecated and should be zero.


pe_e_parse_coff_header pe_parse_coff_header(std::byte const* const file_data, int const file_size, pe_coff_header const** const header_out)
{
	pe_dos_header const& dosheader = *reinterpret_cast<pe_dos_header const*>(file_data + 0);
	WARN_M_R(file_size >= static_cast<int>(dosheader.m_pe_offset + sizeof(pe_coff_header)), L"File is too small to contain coff_header.", pe_e_parse_coff_header::file_too_small);
	pe_coff_header const& header = *reinterpret_cast<pe_coff_header const*>(file_data + dosheader.m_pe_offset);
	WARN_M_R((header.m_signature & 0xFFFF) != s_ne_signature, L"File is not PE, it is NE.", pe_e_parse_coff_header::file_ne);
	WARN_M_R(header.m_signature == s_coff_signature, L"COFF signature not found.", pe_e_parse_coff_header::file_not_coff);
	WARN_M_R(std::find(std::cbegin(s_image_file_machines), std::cend(s_image_file_machines), header.m_machine) != std::end(s_image_file_machines), L"Unknown machine type.", pe_e_parse_coff_header::unknown_machine_type);
	WARN_M_R(header.m_section_count <= s_max_coff_header_sections, L"Too many sections.", pe_e_parse_coff_header::too_many_sections);
	*header_out = &header;
	return pe_e_parse_coff_header::ok;
}
