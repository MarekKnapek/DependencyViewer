#include "smart_com_interface.h"


void com_interface_deleter::operator()(IUnknown* const& ptr) const
{
	ULONG const new_reference_count = ptr->lpVtbl->Release(ptr);
}
