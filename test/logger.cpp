#include "elog/logger.hpp"
#include <cstdint>

using namespace elog;
using namespace std::chrono_literals;

int main(int argc, char* argv[])
{
	// set_log_path("./", argv[0], 100 * 1024 * 1024, 3s, 1024);
	LOG_TRACE("{}", static_cast<uint8_t>(LogLevel::TRACE));
	LOG_DEBUG("{}", static_cast<uint8_t>(LogLevel::DEBUG));
	LOG_INFO("{}", static_cast<uint8_t>(LogLevel::INFO));
	LOG_WARN("{}", static_cast<uint8_t>(LogLevel::WARN));
	LOG_ERROR("{}", static_cast<uint8_t>(LogLevel::ERROR));
	LOG_FATAL("{}", static_cast<uint8_t>(LogLevel::FATAL));
}