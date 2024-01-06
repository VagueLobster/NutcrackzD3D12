#pragma once

#include "Nutcrackz/Core/Exception.hpp"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <comdef.h>

namespace Nutcrackz {

	inline std::string Utf16ToUtf8(std::wstring_view str)
	{
		int length = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.length()), nullptr, 0, nullptr, nullptr);
		std::string message(length, 0);
		(void)WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.length()), message.data(), length, nullptr, nullptr);
		return message;
	}

	inline void CheckHR(HRESULT result)
	{
		if (FAILED(result))
		{
			wchar_t* msgBuffer;
			FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
				FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, result,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<wchar_t*>(&msgBuffer),
				0, nullptr);

			auto msg = Utf16ToUtf8(msgBuffer);
			LocalFree(msgBuffer);

			throw Exception(msg);
		}
	}

}