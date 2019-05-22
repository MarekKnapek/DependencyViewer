#include "processor.h"

#include "search.h"

#include "../nogui/memory_mapped_file.h"
#include "../nogui/utils.h"
#include "../nogui/unicode.h"
#include "../nogui/unique_strings.h"

#include <cassert>
#include <algorithm>
#include <queue>
#include <unordered_map>
#include <iterator>


static constexpr wchar_t const s_not_found_path[] = L"* not found *";
static constexpr wstring const s_not_found_wstr = wstring{static_cast<wchar_t const*>(s_not_found_path), static_cast<int>(std::size(s_not_found_path)) - 1};


struct processor
{
	main_type* m_mo;
	std::wstring const* m_main_file_path;
	std::queue<file_info*> m_queue;
	std::unordered_map<string const*, file_info*, string_hash, string_equal> m_map;
};


void process_r(processor& prcsr);
void process_e(processor& prcsr, file_info& fi, string const* const& dll_name);


wstring const* get_not_found_string()
{
	return &s_not_found_wstr;
}


main_type process(std::wstring const& main_file_path)
{
	main_type mo;
	processor prcsr;
	prcsr.m_mo = &mo;
	prcsr.m_main_file_path = &main_file_path;

	file_info& fi = mo.m_fi;
	fi.m_orig_instance = nullptr;
	fi.m_file_path = nullptr;
	fi.m_sub_file_infos.resize(1);
	fi.m_import_table.m_dlls.resize(1);
	pe_import_dll_with_entries& only_dependency = fi.m_import_table.m_dlls[0];
	wchar_t const* const name = find_file_name(main_file_path.c_str(), static_cast<int>(main_file_path.size()));
	int const name_len = static_cast<int>(main_file_path.c_str() + main_file_path.size() - name);
	assert(is_ascii(name, name_len));
	mo.m_tmpn.resize(name_len);
	std::transform(name, name + name_len, begin(mo.m_tmpn), [](wchar_t const& e) -> char { return static_cast<char>(e); });
	only_dependency.m_dll_name = mo.m_mm.m_strs.add_string(mo.m_tmpn.c_str(), name_len, mo.m_mm.m_alc);
	prcsr.m_queue.push(&fi);
	process_r(prcsr);
	return mo;
}

void process_r(processor& prcsr)
{
	while(!prcsr.m_queue.empty())
	{
		file_info& fi = *prcsr.m_queue.front();
		prcsr.m_queue.pop();
		for(int i = 0; i != static_cast<int>(fi.m_import_table.m_dlls.size()); ++i)
		{
			file_info& sub_fi = fi.m_sub_file_infos[i];
			string const* const& sub_name = fi.m_import_table.m_dlls[i].m_dll_name;
			auto const it = prcsr.m_map.find(sub_name);
			if(it != prcsr.m_map.cend())
			{
				sub_fi.m_orig_instance = it->second;
			}
			else
			{
				process_e(prcsr, sub_fi, sub_name);
				prcsr.m_map[sub_name] = &sub_fi;
				sub_fi.m_sub_file_infos.resize(sub_fi.m_import_table.m_dlls.size());
				prcsr.m_queue.push(&sub_fi);
			}
		}
	}
}

void process_e(processor& prcsr, file_info& fi, string const* const& dll_name)
{
	searcher sch;
	sch.m_mo = prcsr.m_mo;
	sch.m_main_file_path = prcsr.m_main_file_path;
	wstring const* const file_path = search(sch, dll_name);
	if(!file_path)
	{
		fi.m_file_path = &s_not_found_wstr;
		return;
	}
	fi.m_file_path = file_path;

	memory_mapped_file const mmf = memory_mapped_file(fi.m_file_path->m_str);
	pe_header_info const hi = pe_process_header(mmf.begin(), mmf.size());
	fi.m_is_32_bit = hi.m_is_pe32;
	fi.m_import_table = pe_process_import_table(mmf.begin(), mmf.size(), hi, prcsr.m_mo->m_mm);
	fi.m_export_table = pe_process_export_table(mmf.begin(), mmf.size(), hi, prcsr.m_mo->m_mm);
}
