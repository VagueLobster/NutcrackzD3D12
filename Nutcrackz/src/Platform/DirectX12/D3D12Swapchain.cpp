#include "nzpch.hpp"
#include "D3D12Swapchain.hpp"

#include "Nutcrackz/Core/Window.hpp"

#include "rtmcpp/Scalar.hpp"

namespace Nutcrackz {

	IDXGISwapChain1* CreateSwapchain(Window* window, IDXGIFactory4* factory, ID3D12CommandQueue* queue,
		DXGI_SWAP_CHAIN_DESC1* swapchainDesc, DXGI_SWAP_CHAIN_FULLSCREEN_DESC* fullscreenDesc = nullptr, IDXGIOutput* output = nullptr)
	{
		IDXGISwapChain1* swapchain = nullptr;
		HRESULT hr = factory->CreateSwapChainForHwnd(queue, Window::s_WindowHandles[window], swapchainDesc, fullscreenDesc, output, &swapchain);

		if (!FAILED(hr))
		{
			return swapchain;
		}

		return nullptr;
	}

	void D3D12Swapchain::SetupSwapchain(int32_t width, int32_t height, Window* window, D3D12API* api, D3D12CommandList* commandList, D3D12Renderer* renderer)
	{
		commandList->SurfaceSize.left = 0;
		commandList->SurfaceSize.top = 0;
		commandList->SurfaceSize.right = static_cast<LONG>(m_Width);
		commandList->SurfaceSize.bottom = static_cast<LONG>(m_Height);

		commandList->Viewport.TopLeftX = 0.0f;
		commandList->Viewport.TopLeftY = 0.0f;
		commandList->Viewport.Width = static_cast<float>(m_Width);
		commandList->Viewport.Height = static_cast<float>(m_Height);
		commandList->Viewport.MinDepth = .1f;
		commandList->Viewport.MaxDepth = 1000.f;

		// Update matrices
		renderer->UpdateMVPMatrices(width, height);

		if (renderer->Swapchain != nullptr)
		{
			renderer->Swapchain->ResizeBuffers(s_BackbufferCount, m_Width, m_Height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
		}
		else
		{
			DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
			swapchainDesc.BufferCount = s_BackbufferCount;
			swapchainDesc.Width = width;
			swapchainDesc.Height = height;
			swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			swapchainDesc.SampleDesc.Count = 1;

			IDXGISwapChain1* swapchain = CreateSwapchain(window, api->Factory, api->CommandQueue, &swapchainDesc);
			HRESULT swapchainSupport = swapchain->QueryInterface(__uuidof(IDXGISwapChain3), (void**)&swapchain);

			if (SUCCEEDED(swapchainSupport))
				renderer->Swapchain = (IDXGISwapChain3*)swapchain;
		}
		commandList->FrameIndex = renderer->Swapchain->GetCurrentBackBufferIndex();
	}

	void D3D12Swapchain::Resize(int32_t width, int32_t height, Window* window, D3D12API* api, D3D12CommandList* commandList, D3D12Renderer* renderer, D3D12FrameBuffer* frameBuffer)
	{
		m_Width = rtmcpp::Clamp((float)width, 1.0f, (float)0xffff);
		m_Height = rtmcpp::Clamp((float)height, 1.0f, (float)0xffff);

		// WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
		// This is code implemented as such for simplicity. The
		// D3D12HelloFrameBuffering sample illustrates how to use fences for
		// efficient resource usage and to maximize GPU utilization.

		// Signal and increment the fence value.
		const uint64_t fence = commandList->FenceValue;
		ThrowIfFailed(api->CommandQueue->Signal(api->Fence, fence));
		commandList->FenceValue++;

		// Wait until the previous frame is finished.
		if (api->Fence->GetCompletedValue() < fence)
		{
			ThrowIfFailed(api->Fence->SetEventOnCompletion(fence, commandList->FenceEvent));
			WaitForSingleObjectEx(commandList->FenceEvent, INFINITE, false);
		}

		frameBuffer->DestroyFrameBuffer(commandList);
		SetupSwapchain(width, height, window, api, commandList, renderer);
		frameBuffer->InitFrameBuffer(api, commandList, renderer);
	}

	void D3D12Swapchain::ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
			throw std::exception();
	}

}