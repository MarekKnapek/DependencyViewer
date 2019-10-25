#pragma once


#include "memory_manager.h"
#include "smart_library.h"
#include "unique_strings.h"

#include <cstddef>
#include <vector>

#include "my_windows.h"


class manifest_parser_impl;


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
	manifest_data parse(std::byte const* const data, int const len);
private:
	memory_manager& m_mm;
	friend manifest_parser_impl;
};
