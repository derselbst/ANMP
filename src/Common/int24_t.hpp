
// stolen from: https://gist.github.com/mntone/25d591ff5db04de26e95

#pragma once

#pragma pack(push, 1)
using int24_t = struct _int24_t
{
	int32_t data : 24;

	_int24_t(int8_t integer)
		: data(integer)
	{ }
	_int24_t(uint8_t integer)
		: data(integer)
	{ }
	_int24_t(int16_t integer)
		: data(integer)
	{ }
	_int24_t(uint16_t integer)
		: data(integer)
	{ }
	explicit _int24_t(int32_t integer)
		: data(integer)
	{ }
	explicit _int24_t(uint32_t integer)
		: data(integer)
	{ }
	explicit _int24_t(uint64_t integer)
		: data(integer)
	{ }
	explicit _int24_t(int64_t integer)
		: data(integer)
	{ }

	_int24_t& operator=(int8_t const& integer)
	{
		data = integer;
		return *this;
	}
	_int24_t& operator=(uint8_t const& integer)
	{
		data = integer;
		return *this;
	}
	_int24_t& operator=(int16_t const& integer)
	{
		data = integer;
		return *this;
	}
	_int24_t& operator=(uint16_t const& integer)
	{
		data = integer;
		return *this;
	}

	explicit operator int8_t() { return data; }
	explicit operator int8_t() const { return data; }
	explicit operator uint8_t() { return data; }
	explicit operator uint8_t() const { return data; }
	explicit operator int16_t() { return data; }
	explicit operator int16_t() const { return data; }
	explicit operator uint16_t() { return data; }
	explicit operator uint16_t() const { return data; }
	operator int32_t() { return data; }
	operator int32_t() const { return data; }
	explicit operator uint32_t() { return data; }
	explicit operator uint32_t() const { return data; }
	operator int64_t() { return data; }
	operator int64_t() const { return data; }
	explicit operator uint64_t() { return data; }
	explicit operator uint64_t() const { return data; }
};
#pragma pack(pop)