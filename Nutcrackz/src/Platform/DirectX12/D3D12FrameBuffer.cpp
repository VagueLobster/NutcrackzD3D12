#include "nzpch.hpp"
#include "D3D12FrameBuffer.hpp"

namespace Nutcrackz {

	void D3D12FrameBuffer::InitFrameBuffer(D3D12API* api, D3D12CommandList* commandList, D3D12Renderer* renderer)
	{
		renderer->CurrentBuffer = renderer->Swapchain->GetCurrentBackBufferIndex();

		// Create descriptor heaps.
		{
			// Describe and create a render target view (RTV) descriptor heap.
			D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
			rtvHeapDesc.NumDescriptors = s_BackbufferCount;
			rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			ThrowIfFailed(api->Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&commandList->RtvHeap)));

			commandList->RtvDescriptorSize = api->Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		}

		// Create frame resources.
		{
			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(commandList->RtvHeap->GetCPUDescriptorHandleForHeapStart());

			// Create a RTV for each frame.
			for (uint32_t n = 0; n < s_BackbufferCount; n++)
			{
				ThrowIfFailed(renderer->Swapchain->GetBuffer(n, IID_PPV_ARGS(&commandList->RenderTargets[n])));
				api->Device->CreateRenderTargetView(commandList->RenderTargets[n], nullptr, rtvHandle);
				rtvHandle.ptr += (1 * commandList->RtvDescriptorSize);
			}
		}
	}

	void D3D12FrameBuffer::DestroyFrameBuffer(D3D12CommandList* commandList)
	{
		for (uint32_t i = 0; i < s_BackbufferCount; ++i)
		{
			if (commandList->RenderTargets[i])
			{
				commandList->RenderTargets[i]->Release();
				commandList->RenderTargets[i] = 0;
			}
		}

		if (commandList->RtvHeap)
		{
			commandList->RtvHeap->Release();
			commandList->RtvHeap = nullptr;
		}
	}

	void D3D12FrameBuffer::ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
			throw std::exception();
	}

}