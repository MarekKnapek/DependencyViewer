#pragma once


#include <cassert>

#include <windows.h>


#define WARN(X) do{ if(!(X)){ assert((OutputDebugStringW(L"Warning: " L ## #X L"\x0D\x0A"), true)); } }while(false)
#define WARN_M(X, M) do{ if(!(X)){ assert((OutputDebugStringW(L"Warning: " M L"\x0D\x0A"), true)); } }while(false)
#define WARN_M_R(X, M, R) do{ if(!(X)){ assert((OutputDebugStringW(L"Warning: " M L"\x0D\x0A"), true)); return R; } }while(false)
