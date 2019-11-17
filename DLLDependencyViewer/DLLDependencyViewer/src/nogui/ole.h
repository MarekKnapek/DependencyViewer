#pragma once


class ole
{
public:
	ole();
	ole(ole const&) = delete;
	ole(ole&&) noexcept = delete;
	ole& operator=(ole const&) = delete;
	ole& operator=(ole&&) noexcept = delete;
	~ole();
};
