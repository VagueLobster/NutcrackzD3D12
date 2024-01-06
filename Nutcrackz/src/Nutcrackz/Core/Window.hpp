#pragma once

#include "Unique.hpp"
#include "Nutcrackz/Renderer/Renderer.hpp"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <string_view>
#include <vector>

namespace Nutcrackz {

	struct WindowSpecification
	{
		uint32_t Width = 1920;
		uint32_t Height = 1080;
	};

	class Window
	{
	public:
		bool IsClosed() const { return m_Closed; }
		void Close() { m_Closed = true; }

	public:
		inline static std::unordered_map<Window*, HWND> s_WindowHandles;
		
		WindowSpecification m_WindowSpec;
		Unique<Renderer> D3D12Renderer;

	private:
		bool m_Closed = false;

		friend class WindowSystem;
	};

	class WindowSystem final
	{
	public:
		WindowSystem();
		~WindowSystem();

		Window* NewWindow(std::string_view title, uint32_t width = 1920, uint32_t height = 1080);

		void Init(Window* window);

		void Render(Window* window);

		void PollEvents() const;

	private:
		std::vector<Unique<Window>> m_Windows;
	};

}