#pragma once


#ifdef _DEBUG
#define WANT_STATIC_VECTOR 0
#else
#define WANT_STATIC_VECTOR 1
#endif


#if WANT_STATIC_VECTOR == 1

#include "static_vector.h"
template<typename T> using my_vector = static_vector<T>;
template<typename T> inline void my_vector_resize(my_vector<T>& vec, allocator& alc, int const size) { vec.resize(alc, size); }

#else

#include <vector>
class allocator;
template<typename T> using my_vector = std::vector<T>;
template<typename T> inline void my_vector_resize(my_vector<T>& vec, allocator&, int const size) { vec.resize(size); }

#endif
