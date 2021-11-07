#pragma once
#include <stdint.h>
#include <string>

template <class T>
class BitArray {
	virtual bool get(uint32_t index) const = 0;

	virtual void zero() = 0;

	virtual void set(uint32_t index, bool bit) = 0;

	virtual bool none() const = 0;

	virtual T* invert() = 0;

	virtual T operator~() const = 0;

	virtual T& operator|=(const T& other) = 0;

	virtual T operator|(const T& other) const = 0;

	virtual T& operator&=(const T& other) = 0;

	virtual T operator&(const T& other) const = 0;

	virtual void and_out(const T& other, T& out) const = 0;

	virtual T& operator^=(const T& other) = 0;

	virtual T operator^(const T& other) const = 0;

	virtual T& operator<<=(uint32_t amount) = 0;

	virtual T operator<<(uint32_t amount) const = 0;

	virtual T& operator>>=(uint32_t amount) = 0;

	virtual T operator>>(uint32_t amount) const = 0;

	virtual bool operator==(T const& other) const = 0;

	virtual std::string toString() const = 0;

	virtual std::string toString(int size) const = 0;
};