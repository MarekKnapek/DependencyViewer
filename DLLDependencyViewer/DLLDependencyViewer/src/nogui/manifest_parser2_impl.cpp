#include "manifest_parser2_impl.h"

#include "assert.h"

#include <utility>


manifest_parser2_impl::manifest_parser2_impl(std::byte const* const data, int const data_len) :
	m_stream(),
	m_xml_reader()
{
	IStream* const stream = SHCreateMemStream(reinterpret_cast<BYTE const*>(data), static_cast<UINT>(data_len));
	WARN_M_RV(stream, L"Failed to SHCreateMemStream.");
	com_ptr<IStream> sp_stream{stream};
	IXmlReader* xml_reader = nullptr;
	HRESULT const xml_reader_created = CreateXmlReader(IID_IXmlReader, reinterpret_cast<void**>(&xml_reader), nullptr);
	WARN_M_RV(xml_reader_created == S_OK && xml_reader, L"Failed to CreateXmlReader.");
	com_ptr<IXmlReader> sp_xml_reader{xml_reader};
	HRESULT const dtd_set = sp_xml_reader->lpVtbl->SetProperty(sp_xml_reader, XmlReaderProperty_DtdProcessing, DtdProcessing_Prohibit);
	WARN_M_RV(dtd_set == S_OK, L"Failed to IXmlReader::SetProperty(DtdProcessing, Prohibit).");
	HRESULT const input_set = sp_xml_reader->lpVtbl->SetInput(sp_xml_reader, sp_stream.to_iunknown());
	WARN_M_RV(input_set == S_OK, L"Failed to IXmlReader::SetInput.");
	m_stream = std::move(sp_stream);
	m_xml_reader = std::move(sp_xml_reader);
}

manifest_parser2_impl::~manifest_parser2_impl()
{
}

bool manifest_parser2_impl::ok() const
{
	return m_stream && m_xml_reader;
}
