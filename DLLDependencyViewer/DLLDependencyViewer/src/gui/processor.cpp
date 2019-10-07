#include "processor.h"

#include "processor_impl.h"


static constexpr wchar_t const s_not_found_path[] = L"* not found *";
static constexpr wstring const s_not_found_wstr = wstring{static_cast<wchar_t const*>(s_not_found_path), static_cast<int>(std::size(s_not_found_path)) - 1};


void main_type::swap(main_type& other) noexcept
{
	using std::swap;
	swap(m_mm, other.m_mm);
	swap(m_fi, other.m_fi);
	swap(m_tmpn, other.m_tmpn);
	swap(m_tmpw, other.m_tmpw);
	swap(m_tmpp, other.m_tmpp);
}

wstring const* get_not_found_string()
{
	return &s_not_found_wstr;
}

main_type process(std::wstring const& main_file_path)
{
	return process_impl(main_file_path);
}
