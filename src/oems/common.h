#pragma once

#include <cstdint>
#include <sstream>


#ifdef _DEBUG
	#define OEMS_DEBUG 1
#else
	#define NDEBUG 1
	#define OEMS_RELEASE 1
#endif // #ifdef _DEBUG


#define EXCEPTION_MSG(...) oems::concat(__FILE__, '(', __LINE__, "): ", __VA_ARGS__)

#define ENFORCE(expression, ...)								\
	if (!(expression)) {										\
		throw std::runtime_error(EXCEPTION_MSG(__VA_ARGS__));	\
	}

namespace oems { namespace intrinsic {

inline void concat_impl(std::ostringstream&) {}

template<typename T, typename... Args>
void concat_impl(std::ostringstream& string_stream, const T& obj, const Args&... args)
{
	string_stream << obj;
	concat_impl(string_stream, args...);
}

}} // namespace oems::intrinsic


namespace oems {

struct uint2 final {
	uint32_t x;
	uint32_t y;
};


inline bool operator>(const uint2& l, uint32_t val) noexcept
{
	return (l.x > val) && (l.y > val);
}

// Concatenates any number of std::string, c-style string or char arguments into one string.
template<typename... Args>
inline std::string concat(const Args&... args)
{
	std::ostringstream string_stream;
	oems::intrinsic::concat_impl(string_stream, args...);
	return string_stream.str();
}

// Constructs exception message string considering all the nested exceptions.
// Each exception message is formatted as a new line and starts with " - " prefix. 
std::string make_exception_message(const std::exception& exc);

std::string read_text(const char* p_filename);

} // namespace oems
