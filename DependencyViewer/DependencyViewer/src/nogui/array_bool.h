#pragma once


struct array_bool
{
	unsigned* m_data;
};


int array_bool_space_needed(int const number_of_bools);
void array_bool_set(array_bool const arr, int const idx);
void array_bool_clr(array_bool const arr, int const idx);
bool array_bool_tst(array_bool const arr, int const idx);
