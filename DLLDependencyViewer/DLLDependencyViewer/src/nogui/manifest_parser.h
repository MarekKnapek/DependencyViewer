#pragma once


#include "smart_library.h"
#include "unique_strings.h"
#include "memory_manager.h"

#include <windows.h>


enum class manifest_dependency_architecture
{
	x86,
	ia64,
	amd64,
	star
};

struct manifest_dependency
{
	wstring const* m_name;
	wstring const* m_language;
	std::uint64_t m_version;
	char m_token[16];
	manifest_dependency_architecture m_architecture;
};

struct manifest_data
{
	std::vector<manifest_dependency> m_dependencies;
};


class manifest_parser
{
public:
	manifest_parser(memory_manager& mm);
	~manifest_parser();
public:
	manifest_data parse(char const* const& data, int const& len);
private:
	class manifest_parser_impl;
	memory_manager& m_mm;
	smart_library m_xmllite;
	function_ptr_t m_create_xml_reader;
	smart_library m_shlwapi;
	function_ptr_t m_sh_create_mem_stream;
};
