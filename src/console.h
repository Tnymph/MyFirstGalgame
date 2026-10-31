#pragma once
#include <cstdio>
#include <format>
#include <string>

// 控制台字体颜色
namespace consoleColor
{
	inline constexpr const char* RED = "\033[31m";		// 操作失败
	inline constexpr const char* GREEN = "\033[32m";	// 操作成功
	inline constexpr const char* YELLOW = "\033[1;33m";	// 执行操作
	inline constexpr const char* RESET = "\033[0m";		// 重置颜色

	template<typename... T>
	void fail(std::format_string<T...> tpl, T&&... contents)
	{
		std::string output = std::format(tpl, std::forward<T>(contents)...);
		printf("%s%s%s\n", RED, output.c_str(), RESET);
	}

	template<typename... T>
	void success(std::format_string<T...> tpl, T&&... contents)
	{
		std::string output = std::format(tpl, std::forward<T>(contents)...);
		printf("%s%s%s\n", GREEN, output.c_str(), RESET);
	}

	template<typename... T>
	void excute(std::format_string<T...> tpl, T&&... contents)
	{
		std::string output = std::format(tpl, std::forward<T>(contents)...);
		printf("%s%s%s\n", YELLOW, output.c_str(), RESET);
	}
}