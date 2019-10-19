#pragma once


int array_bool_space_needed(int const number_of_bools);
void array_bool_set(unsigned* const arr, int const idx);
void array_bool_clr(unsigned* const arr, int const idx);
bool array_bool_tst(unsigned const* const arr, int const idx);
