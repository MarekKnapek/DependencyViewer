#include "processor.h"

#include "search.h"

#include "../nogui/memory_mapped_file.h"

#include <algorithm>
#include <queue>
#include <unordered_map>
#include <experimental/filesystem>


namespace fs = std::experimental::filesystem;


struct processor_hash
{
	std::size_t operator()(std::wstring* const& e) const
	{
		return std::hash<std::wstring>()(*e);
	}
};

struct processor_equal
{
	bool operator()(std::wstring* const& a, std::wstring* const& b) const
	{
		return *a == *b;
	}
};


struct processor
{
	std::wstring const* m_main_file_path;
	std::queue<file_info*> m_queue;
	std::unordered_map<std::wstring*, file_info*, processor_hash, processor_equal> m_map;
};


void process_r(processor& prcsr);
void process_e(processor& prcsr, file_info& fi);


file_info process(std::wstring const& main_file_path)
{
	file_info fi;
	processor prcsr;
	prcsr.m_main_file_path = &main_file_path;
	try
	{
		fi.m_file_name = fs::path(main_file_path).filename();
		prcsr.m_queue.push(&fi);
		process_r(prcsr);
	}
	catch(wchar_t const* const)
	{
		return {};
	}
	return fi;
}

void process_r(processor& prcsr)
{
	while(!prcsr.m_queue.empty())
	{
		file_info& fi = *prcsr.m_queue.front();
		prcsr.m_queue.pop();
		std::wstring& dll_name = fi.m_file_name;
		auto const it = prcsr.m_map.find(&dll_name);
		if(it != cend(prcsr.m_map))
		{
			fi.m_orig_instance = it->second;
		}
		else
		{
			process_e(prcsr, fi);
		}
	}
}

void process_e(processor& prcsr, file_info& fi)
{
	searcher sch;
	sch.m_main_file_path = prcsr.m_main_file_path;
	fi.m_file_path = search(sch, fi.m_file_name);
	if(fi.m_file_path.empty())
	{
		fi.m_file_path = L"* not found *";
		return;
	}

	memory_mapped_file const mmf = memory_mapped_file(fi.m_file_path.c_str());
	pe_header_info const hi = pe_process_header(mmf.begin(), mmf.size());
	fi.m_import_table = pe_process_import_table(mmf.begin(), mmf.size(), hi);
	fi.m_export_table = pe_process_export_table(mmf.begin(), mmf.size(), hi);
	fi.m_orig_instance = nullptr;
	fi.m_sub_file_infos.resize(fi.m_import_table.m_dlls.size());
	for(int i = 0; i != static_cast<int>(fi.m_import_table.m_dlls.size()); ++i)
	{
		fi.m_sub_file_infos[i].m_file_name.resize(fi.m_import_table.m_dlls[i].m_dll_name.size());
		std::transform(cbegin(fi.m_import_table.m_dlls[i].m_dll_name), cend(fi.m_import_table.m_dlls[i].m_dll_name), begin(fi.m_sub_file_infos[i].m_file_name), [](char const& e) -> wchar_t { return static_cast<wchar_t>(e); });
	}
	prcsr.m_map[&fi.m_file_name] = &fi;
	std::for_each(begin(fi.m_sub_file_infos), end(fi.m_sub_file_infos), [&](file_info& e){ prcsr.m_queue.push(&e); });
}
