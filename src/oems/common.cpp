#include "oems/common.h"

#include <cassert>
#include <memory>


namespace {

void accumulate_exception_message_impl(std::string& dest, const std::exception& exc)
{
	dest.append("- ");
	dest.append(exc.what());
	dest.push_back('\n');

	try {
		std::rethrow_if_nested(exc);
	}
	catch (const std::exception& nested_exc) {
		accumulate_exception_message_impl(dest, nested_exc);
	}
	catch (...) {
		// https://www.youtube.com/watch?v=umDr0mPuyQc
		// do not throw non descendants of std::exception
		assert(false);
	}
}

} // namespace


namespace oems {

std::string make_exception_message(const std::exception& exc)
{
	std::string msg;
	accumulate_exception_message_impl(msg, exc);
	return msg;
}

std::string read_text(const char* p_filename)
{
#pragma warning(push)
#pragma warning(disable:4996) // C4996 'fopen': This function or variable may be unsafe.

	std::unique_ptr<FILE, decltype(&std::fclose)> file(std::fopen(p_filename, "rb"), &std::fclose);

#pragma warning(pop)

	if (!file) {
		const std::string msg = concat("Failed to open file ", p_filename);
		throw std::runtime_error(msg);
	}

	// size of the file in bytes ---
	std::fseek(file.get(), 0, SEEK_END);
	const size_t byte_count = std::ftell(file.get());
	std::rewind(file.get());
	if (byte_count == 0) return {};

	// read the file's contents ---
	std::string str;
	str.reserve(byte_count);

	constexpr size_t buffer_byte_count = 1024;
	char buffer[buffer_byte_count];
	while (std::feof(file.get()) == 0) {
		// n - actual number of bytes read.
		const size_t n = std::fread(buffer, sizeof(char), buffer_byte_count, file.get());
		str.append(buffer, n);
	}

	return str;
}

} // namespace oems
