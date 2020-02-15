#include "file_name_provider.h"

#include "assert_my.h"
#include "cassert_my.h"
#include "scope_exit.h"

#include <utility>


static file_name_provider* g_file_name_provider = nullptr;


void file_name_provider::init()
{
	assert(!g_file_name_provider);
	g_file_name_provider = new file_name_provider();
}

void file_name_provider::deinit()
{
	assert(g_file_name_provider);
	delete g_file_name_provider;
	g_file_name_provider = nullptr;
}

wstring_handle file_name_provider::get_correct_file_name(wchar_t const* const& file_name, int const& file_name_len, wunique_strings& us, allocator& alc)
{
	assert(g_file_name_provider);
	if(!g_file_name_provider->ok())
	{
		return {};
	}
	return g_file_name_provider->get_correct_file_name_(file_name, file_name_len, us, alc);
}

file_name_provider::file_name_provider() :
	m_clsid(),
	m_object(),
	m_method()
{
	CLSID clsid;
	HRESULT const get_clsid = CLSIDFromProgID(L"Scripting.FileSystemObject", &clsid);
	WARN_M_RV(get_clsid == S_OK, L"Failed to CLSIDFromProgID.");
	IDispatch* object = nullptr;
	HRESULT const instance_created = CoCreateInstance(clsid, nullptr, CLSCTX_INPROC_SERVER, IID_IDispatch, reinterpret_cast<void**>(&object));
	WARN_M_RV(instance_created == S_OK && object != nullptr, L"Failed to CoCreateInstance.");
	com_ptr<IDispatch> object_sp(object);
	static constexpr wchar_t const s_method_name[] = L"GetAbsolutePathName";
	wchar_t const* const method_name = s_method_name;
	DISPID method;
	HRESULT const got_method = object->lpVtbl->GetIDsOfNames(object, IID_NULL, const_cast<wchar_t**>(&method_name), 1, LOCALE_USER_DEFAULT, &method);
	WARN_M_RV(got_method == S_OK, L"Failed to IDispatch::GetIDsOfNames.");
	m_clsid = clsid;
	m_object = std::move(object_sp);
	m_method = method;
}

file_name_provider::~file_name_provider()
{
}

bool file_name_provider::ok() const
{
	return m_object != nullptr;
}

wstring_handle file_name_provider::get_correct_file_name_(wchar_t const* const& file_name, int const& file_name_len, wunique_strings& us, allocator& alc)
{
	VARIANT arg_1;
	arg_1.vt = VT_BSTR;
	arg_1.bstrVal = SysAllocStringLen(file_name, file_name_len);
	auto const free_sys_string_1 = mk::make_scope_exit([&](){ SysFreeString(arg_1.bstrVal); });
	DISPPARAMS input;
	input.rgvarg = &arg_1;
	input.rgdispidNamedArgs = nullptr;
	input.cArgs = 1;
	input.cNamedArgs = 0;
	VARIANT output;
	output.vt = VT_BSTR;
	output.bstrVal = nullptr;
	HRESULT const invoked = m_object->lpVtbl->Invoke(m_object, m_method, m_clsid, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &input, &output, nullptr, nullptr);
	WARN_M_R(invoked == S_OK, L"Failed to IDispatch::Invoke.", {});
	WARN_M_R(output.bstrVal, L"Failed to IDispatch::Invoke.", {});
	auto const free_sys_string_2 = mk::make_scope_exit([&](){ SysFreeString(output.bstrVal); });
	int const output_len = static_cast<int>(*reinterpret_cast<std::uint32_t const*>(reinterpret_cast<char const*>(output.bstrVal) - sizeof(std::uint32_t)));
	assert(output_len % 2 == 0);
	wstring_handle const ret = us.add_string(output.bstrVal, output_len / 2, alc);
	return ret;
}
