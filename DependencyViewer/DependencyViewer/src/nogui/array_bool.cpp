#include "array_bool.h"

#include "cassert_my.h"

#include <climits>


int array_bool_space_needed(int const number_of_bools)
{
	assert(number_of_bools >= 0);
	if(number_of_bools == 0)
	{
		return 0;
	}
	else
	{
		return ((number_of_bools - 1) / (static_cast<int>(sizeof(unsigned)) * CHAR_BIT)) + 1;
	}
}

void array_bool_set(array_bool const arr, int const idx)
{
	int const i = idx / (static_cast<int>(sizeof(unsigned)) * CHAR_BIT);
	int const j = idx % (static_cast<int>(sizeof(unsigned)) * CHAR_BIT);
	arr.m_data[i] |= (1u << j);
}

void array_bool_clr(array_bool const arr, int const idx)
{
	int const i = idx / (static_cast<int>(sizeof(unsigned)) * CHAR_BIT);
	int const j = idx % (static_cast<int>(sizeof(unsigned)) * CHAR_BIT);
	arr.m_data[i] &=~ (1u << j);
}

bool array_bool_tst(array_bool const arr, int const idx)
{
	int const i = idx / (static_cast<int>(sizeof(unsigned)) * CHAR_BIT);
	int const j = idx % (static_cast<int>(sizeof(unsigned)) * CHAR_BIT);
	return (arr.m_data[i] & (1u << j)) != 0;
}
