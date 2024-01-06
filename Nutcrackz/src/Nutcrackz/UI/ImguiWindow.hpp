#pragma once

//#include "Platform/DirectX12/D3D12Common.hpp"
//#include "Platform/DirectX12/D3D12API.hpp"
//#include "Platform/DirectX12/D3D12Renderer.hpp"
//#include "Platform/DirectX12/D3D12FrameBuffer.hpp"
//#include "Platform/DirectX12/D3D12Swapchain.hpp"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

struct ImVec4;

namespace Nutcrackz {

	struct FrameContext;

	class Window;

	class ImGuiWindow
	{
	public:
		~ImGuiWindow();

		bool Init(Window* window);
		void Shutdown();

		void Begin();
		void End(Window* window, uint32_t width, uint32_t height);

		/*static void SetDarkThemeColors();
		static void SetGoldDarkThemeColors();
		static void SetChocolateThemeColors();
		static void SetLightThemeColors();

		static ImVec4 ConvertColorFromByteToFloats(uint32_t r, uint32_t g, uint32_t b, uint32_t a);
		static uint32_t ConvertColorFromFloatToByte(float value);*/

	private:
		bool CreateDeviceD3D(HWND hWnd);
		void CleanupDeviceD3D();
		void CreateRenderTarget();
		void CleanupRenderTarget();
		void WaitForLastSubmittedFrame();
		FrameContext* WaitForNextFrameResources();
	};

}