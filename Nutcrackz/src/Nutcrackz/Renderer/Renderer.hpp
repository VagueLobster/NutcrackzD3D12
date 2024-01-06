#pragma once

#include "Nutcrackz/Core/Unique.hpp"

#include "Platform/DirectX12/D3D12Common.hpp"
#include "Platform/DirectX12/D3D12API.hpp"
#include "Platform/DirectX12/D3D12Renderer.hpp"
#include "Platform/DirectX12/D3D12FrameBuffer.hpp"
#include "Platform/DirectX12/D3D12Swapchain.hpp"

namespace Nutcrackz {

    class Window;

    class Renderer
    {
    public:
        Renderer(Window* window);

        ~Renderer();

        void Render();
        void Resize(int32_t width, int32_t height);

		D3D12API* GetAPI() { return m_API.Raw(); }
		D3D12CommandList* GetCommandList() { return m_CommandList.Raw(); }
		D3D12Renderer* GetRenderer() { return m_Renderer.Raw(); }
		D3D12FrameBuffer* GetFrameBuffer() { return m_FrameBuffer.Raw(); }
		D3D12Swapchain* GetSwapchain() { return m_Swapchain.Raw(); }

	private:
        //void InitializeAPI(Window* window);
        //void DestroyAPI();

        //void InitializeResources();
        //void DestroyResources();

        // Create graphics API specific data structures to send commands to the GPU
        //void CreateCommands();

        // Set up commands used when rendering frame by this app
        //void SetupCommands();
        //void DestroyCommands();

        //void InitFrameBuffer();
        //void DestroyFrameBuffer();
        
		//void SetupSwapchain(int32_t width, int32_t height);

        // Set up the RenderPass
        //void CreateRenderPass();

        //void CreateSynchronization();

		void ThrowIfFailed(HRESULT hr);

    private:
		Window* m_Window;
		//int32_t m_Width, m_Height;

		Unique<D3D12API> m_API;
		Unique<D3D12CommandList> m_CommandList;
		Unique<D3D12Renderer> m_Renderer;
		Unique<D3D12FrameBuffer> m_FrameBuffer;
		Unique<D3D12Swapchain> m_Swapchain;

		// Initialization
		//IDXGIFactory4* m_Factory;
		//IDXGIAdapter1* m_Adapter;
//#if defined(NZ_DEBUG)
		//ID3D12Debug1* m_DebugController;
		//ID3D12DebugDevice* m_DebugDevice;
//#endif
		//ID3D12Device* m_Device;
		//ID3D12CommandQueue* m_CommandQueue;
		//ID3D12CommandAllocator* m_CommandAllocator;
		//ID3D12GraphicsCommandList* m_CommandList = nullptr;

		// Current Frame
		//uint32_t m_CurrentBuffer;
		//ID3D12DescriptorHeap* m_RtvHeap = nullptr;
		//ID3D12Resource* m_RenderTargets[s_BackbufferCount];
		//IDXGISwapChain3* m_Swapchain = nullptr;

		//ID3D12Resource* m_VertexBuffer = nullptr;
		//ID3D12Resource* m_IndexBuffer = nullptr;

		//ID3D12Resource* m_UniformBuffer = nullptr;
		//ID3D12DescriptorHeap* m_UniformBufferHeap = nullptr;
		//uint8_t* m_MappedUniformBuffer = nullptr;

		//D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
		//D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;

		//uint32_t m_RtvDescriptorSize;
		//ID3D12RootSignature* m_RootSignature = nullptr;
		//ID3D12PipelineState* m_PipelineState = nullptr;

		// Sync
		//uint32_t m_FrameIndex;
		//HANDLE m_FenceEvent;
		//ID3D12Fence* m_Fence;
		//uint64_t m_FenceValue;
    };

}