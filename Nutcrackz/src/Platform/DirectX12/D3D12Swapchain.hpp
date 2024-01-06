#pragma once

#include "Platform/DirectX12/D3D12Common.hpp"
#include "Platform/DirectX12/D3D12API.hpp"
#include "Platform/DirectX12/D3D12CommandList.hpp"
#include "Platform/DirectX12/D3D12FrameBuffer.hpp"

namespace Nutcrackz {

	class Window;

	class D3D12Swapchain
	{
	public:
		void Resize(int32_t width, int32_t height, Window* window, D3D12API* api, D3D12CommandList* commandList, D3D12Renderer* renderer, D3D12FrameBuffer* frameBuffer);

		void SetupSwapchain(int32_t width, int32_t height, Window* window, D3D12API* api, D3D12CommandList* commandList, D3D12Renderer* renderer);

		int32_t GetWidth() const { return m_Width; }
		int32_t GetHeight() const { return m_Height; }

	private:
		void ThrowIfFailed(HRESULT hr);

	private:
		int32_t m_Width, m_Height;
	};

}