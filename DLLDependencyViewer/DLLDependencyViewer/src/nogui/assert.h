#pragma once


#include <cassert>


#define WARN(X) do{ if(!(X)){ assert((assert_function(L"DLLDependencyViewer Warning: " L ## #X L"\x0D\x0A"), true)); } }while(false)
#define WARN_M(X, M) do{ if(!(X)){ assert((assert_function(L"DLLDependencyViewer Warning: " M L"\x0D\x0A"), true)); } }while(false)
#define WARN_M_R(X, M, R) do{ if(!(X)){ assert((assert_function(L"DLLDependencyViewer Warning: " M L"\x0D\x0A"), true)); return R; } }while(false)


void assert_function(wchar_t const* const& str);
