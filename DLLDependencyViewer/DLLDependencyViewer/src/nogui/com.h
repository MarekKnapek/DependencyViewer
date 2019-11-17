#pragma once


class com
{
public:
	com();
	com(com const&) = delete;
	com(com&&) noexcept = delete;
	com& operator=(com const&) = delete;
	com& operator=(com&&) noexcept = delete;
	~com();
};
