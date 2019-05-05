#include "main_window.h"

#include <utility>


main_window::main_window() noexcept
{
}

main_window::main_window(main_window&& other) noexcept :
	main_window()
{
	swap(other);
}

main_window& main_window::operator=(main_window&& other) noexcept
{
	swap(other);
	return *this;
}

main_window::~main_window() noexcept
{
}

void main_window::swap(main_window& other) noexcept
{
	using std::swap;
}
