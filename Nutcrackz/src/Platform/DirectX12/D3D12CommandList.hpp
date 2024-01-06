#pragma once

#include "Platform/DirectX12/D3D12Common.hpp"
#include "Platform/DirectX12/D3D12API.hpp"

namespace Nutcrackz {

	class D3D12CommandList
	{
	public:
		// Create graphics API specific data structures to send commands to the GPU
		void CreateCommands(D3D12API* api);

		// Set up commands used when rendering frame by this app
		void SetupCommands(D3D12API* api);
		void DestroyCommands(D3D12API* api);

	private:
		void ThrowIfFailed(HRESULT hr);

	public:
		ID3D12DescriptorHeap* RtvHeap = nullptr;
		ID3D12DescriptorHeap* DescriptorHeap = nullptr;

		ID3D12Resource* RenderTargets[s_BackbufferCount];

		D3D12_VERTEX_BUFFER_VIEW VertexBufferView;
		D3D12_INDEX_BUFFER_VIEW IndexBufferView;

		uint32_t RtvDescriptorSize;

		ID3D12RootSignature* RootSignature = nullptr;
		ID3D12PipelineState* PipelineState = nullptr;

		D3D12_VIEWPORT Viewport;

		// Sync
		uint32_t FrameIndex;
		HANDLE FenceEvent;
		uint64_t FenceValue;

		D3D12_RECT SurfaceSize;
		ID3D12GraphicsCommandList* CommandList = nullptr;

		D3D12_CPU_DESCRIPTOR_HANDLE CPURenderTargetDescriptor;
		D3D12_GPU_DESCRIPTOR_HANDLE GPURenderTargetDescriptor;

	private:

	};

}