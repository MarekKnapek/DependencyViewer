#pragma once


#include "cassert_my.h"


#define WARN_XXX_1(X) L#X
#define WARN_XXX_2(X) WARN_XXX_1(X)
#define WARN_XXX_3(X) L##X
#define WARN_XXX_4(X) WARN_XXX_3(X)


#define WARN_M(X, M) do{ if(!(X)) [[unlikely]] { assert((assert_function(L"DependencyViewer Warning! File: " WARN_XXX_4(__FILE__) L", Line: " WARN_XXX_2(__LINE__) L", Message: " M L"\x0D\x0A"), true)); } }while(false)
#define WARN_M_R(X, M, R) do{ if(!(X)) [[unlikely]] { assert((assert_function(L"DependencyViewer Warning! File: " WARN_XXX_4(__FILE__) L", Line: " WARN_XXX_2(__LINE__) L", Message: " M L"\x0D\x0A"), true)); return R; } }while(false)
#define WARN_M_RV(X, M) do{ if(!(X)) [[unlikely]] { assert((assert_function(L"DependencyViewer Warning! File: " WARN_XXX_4(__FILE__) L", Line: " WARN_XXX_2(__LINE__) L", Message: " M L"\x0D\x0A"), true)); return; } }while(false)


void assert_function(wchar_t const* const& str);
