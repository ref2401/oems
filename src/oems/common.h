#pragma once

#include <cstdint>


#ifdef _DEBUG
	#define OEMS_DEBUG 1
#else
	#define NDEBUG 1
	#define OEMS_RELEASE 1
#endif // #ifdef _DEBUG


namespace oems {

struct uint2 final {
	uint32_t x;
	uint32_t y;
};


inline bool operator>(const uint2& l, uint32_t val) noexcept
{
	return (l.x > val) && (l.y > val);
}

} // namespace oems
