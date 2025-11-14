#pragma once


#define CINTERFACE
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifdef ISOLATION_AWARE_ENABLED
#undef ISOLATION_AWARE_ENABLED
#endif


#include <windows.h>


typedef GUID* PGUID;
typedef GUID const* PCGUID;
typedef DWORD DEVICE_TYPE;
