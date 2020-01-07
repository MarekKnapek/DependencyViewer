#include "xml_parser.h"

#include "assert.h"

#include <cassert>
#include <cstring>
#include <utility>

#include <shlwapi.h>


#define s_very_big_int (2'147'483'647)


xml_parser::xml_parser(std::byte const* const data, int const data_len) :
	m_xml_reader()
{
	IStream* const stream = SHCreateMemStream(reinterpret_cast<BYTE const*>(data), static_cast<UINT>(data_len));
	WARN_M_RV(stream, L"Failed to SHCreateMemStream.");
	com_ptr<IStream> const sp_stream{stream};
	IXmlReader* xml_reader = nullptr;
	HRESULT const xml_reader_created = CreateXmlReader(IID_IXmlReader, reinterpret_cast<void**>(&xml_reader), nullptr);
	WARN_M_RV(xml_reader_created == S_OK && xml_reader, L"Failed to CreateXmlReader.");
	com_ptr<IXmlReader> sp_xml_reader{xml_reader};
	HRESULT const dtd_set = sp_xml_reader->lpVtbl->SetProperty(sp_xml_reader, XmlReaderProperty_DtdProcessing, DtdProcessing_Prohibit);
	WARN_M_RV(dtd_set == S_OK, L"Failed to IXmlReader::SetProperty(DtdProcessing, Prohibit).");
	HRESULT const input_set = sp_xml_reader->lpVtbl->SetInput(sp_xml_reader, sp_stream.to_iunknown());
	WARN_M_RV(input_set == S_OK, L"Failed to IXmlReader::SetInput.");
	m_xml_reader = std::move(sp_xml_reader);
}

xml_parser::~xml_parser()
{
}

bool xml_parser::ok() const
{
	return !!m_xml_reader;
}

bool xml_parser::find_element(wchar_t const* const element_to_find, int const element_to_find_len, wchar_t const* const xmlns_to_find, int const xmlns_to_find_len)
{
	IXmlReader* const xml_reader = m_xml_reader;
	int depth = 0;
	for(;;)
	{
		XmlNodeType node_type;
		HRESULT const read = xml_reader->lpVtbl->Read(xml_reader, &node_type);
		WARN_M_R(read == S_OK, L"Failed to IXmlReader::Read.", false);
		if(node_type == XmlNodeType_EndElement)
		{
			if(depth == 0)
			{
				return false;
			}
			else
			{
				--depth;
				continue;
			}
		}
		else if(node_type != XmlNodeType_Element)
		{
			continue;
		}
		assert(node_type == XmlNodeType_Element);
		bool const is_empty = xml_reader->lpVtbl->IsEmptyElement(xml_reader) != FALSE;
		if(!is_empty)
		{
			++depth;
		}
		wchar_t const* element;
		UINT element_len;
		HRESULT const got_element = xml_reader->lpVtbl->GetLocalName(xml_reader, &element, &element_len);
		WARN_M_R(got_element == S_OK, L"Failed to IXmlReader::GetLocalName.", false);
		if(static_cast<int>(element_len) != element_to_find_len)
		{
			continue;
		}
		if(std::memcmp(element, element_to_find, (element_to_find_len + 1) * sizeof(wchar_t)) != 0)
		{
			continue;
		}
		wchar_t const* xmlns;
		UINT xmlns_len;
		HRESULT const got_xmlns = xml_reader->lpVtbl->GetNamespaceUri(xml_reader, &xmlns, &xmlns_len);
		WARN_M_R(got_xmlns == S_OK && xmlns != nullptr, L"Failed to IXmlReader::GetNamespaceUri.", false);
		if(static_cast<int>(xmlns_len) != xmlns_to_find_len)
		{
			continue;
		}
		if(std::memcmp(xmlns, xmlns_to_find, (xmlns_to_find_len + 1) * sizeof(wchar_t)) != 0)
		{
			continue;
		}
		if(!((depth == 1 && !is_empty) || (depth == 0 && is_empty)))
		{
			continue;
		}
		return true;
	}
	__assume(false);
}

bool xml_parser::for_each_attribute(void* const param, attribute_name_callback_t const name_callback, attribute_value_callback_t const value_callback)
{
	IXmlReader* const xml_reader = m_xml_reader;
	UINT attr_count;
	HRESULT const got_attrib_count = xml_reader->lpVtbl->GetAttributeCount(xml_reader, &attr_count);
	WARN_M_R(got_attrib_count == S_OK, L"Failed to IXmlReader::GetAttributeCount.", false);
	if(attr_count != 0)
	{
		HRESULT const moved = xml_reader->lpVtbl->MoveToFirstAttribute(xml_reader);
		WARN_M_R(moved == S_OK, L"Failed to IXmlReader::MoveToFirstAttribute.", false);
		for(UINT i = 0; i != attr_count; ++i)
		{
			wchar_t const* attr_name;
			UINT attr_name_len;
			HRESULT const got_attribute = xml_reader->lpVtbl->GetLocalName(xml_reader, &attr_name, &attr_name_len);
			WARN_M_R(got_attribute == S_OK && attr_name != nullptr, L"Failed to IXmlReader::GetLocalName.", false);
			bool const want = name_callback(param, attr_name, static_cast<int>(attr_name_len));
			if(want)
			{
				wchar_t const* attr_value;
				UINT attr_value_len;
				HRESULT const got_value = xml_reader->lpVtbl->GetValue(xml_reader, &attr_value, &attr_value_len);
				WARN_M_R(got_value == S_OK, L"Failed to IXmlReader::GetValue.", false);
				WARN_M_R(attr_value_len < s_very_big_int, L"Attribute is too big.", false);
				bool const attr_processed = value_callback(param, attr_value, attr_value_len);
				WARN_M_R(attr_processed, L"Failed to process attribute data.", false);
			}
			HRESULT const next = xml_reader->lpVtbl->MoveToNextAttribute(xml_reader);
			WARN_M_R((next == S_OK && i != attr_count - 1) || (next == S_FALSE && i == attr_count - 1), L"Failed to IXmlReader::MoveToNextAttribute.", false);
		}
		HRESULT const moved_back = xml_reader->lpVtbl->MoveToElement(xml_reader);
		WARN_M_R(moved_back == S_OK, L"Failed to IXmlReader::MoveToElement.", false);
	}
	return true;
}
