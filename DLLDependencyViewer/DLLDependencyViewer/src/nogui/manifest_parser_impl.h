#pragma once


#include "manifest_parser.h"
#include "xml_parser.h"

#include <cstddef>


class manifest_parser2_impl
{
public:
	manifest_parser2_impl(std::byte const* const data, int const data_len);
	~manifest_parser2_impl();
	bool ok() const;
public:
	bool parse_files(files_t& out_files);
private:
	bool parse_files_1(files_t& out_files);
private:
	bool find_assembly();
	bool find_file();
private:
	xml_parser m_xml_parser;
};


bool parse_files_impl(std::byte const* const data, int const data_len, files_t& out_files);
