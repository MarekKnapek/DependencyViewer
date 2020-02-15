#pragma once


#ifdef NDEBUG
#define WANT_ASSERTS 0
#else
#define WANT_ASSERTS 1
#endif


#if defined WANT_ASSERTS && WANT_ASSERTS == 1


#ifdef NDEBUG
#undef NDEBUG
#define UNDEFFED
#endif

#include <cassert>

#ifdef UNDEFFED
#define NDEBUG
#undef UNDEFFED
#endif


#else


#ifndef NDEBUG
#define NDEBUG
#define DEFFED
#endif

#include <cassert>

#ifdef DEFFED
#undef NDEBUG
#undef DEFFED
#endif


#endif
