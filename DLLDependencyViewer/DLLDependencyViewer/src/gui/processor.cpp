#include "processor.h"

#include "processor_impl.h"


static constexpr wchar_t const s_not_found_path[] = L"* not found *";
static constexpr wstring const s_not_found_wstr = wstring{static_cast<wchar_t const*>(s_not_found_path), static_cast<int>(std::size(s_not_found_path)) - 1};
static constexpr wstring_handle s_not_found_wstr_hndl = wstring_handle{&s_not_found_wstr};


void main_type::swap(main_type& other) noexcept
{
	using std::swap;
	swap(m_mm, other.m_mm);
	swap(m_fi, other.m_fi);
	swap(m_tmpn, other.m_tmpn);
	swap(m_tmpw, other.m_tmpw);
	swap(m_tmpp, other.m_tmpp);
}

wstring_handle const& get_not_found_string()
{
	return s_not_found_wstr_hndl;
}

main_type process(std::vector<std::wstring> const& file_paths)
{
	return process_impl(file_paths);
}
