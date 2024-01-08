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
		Window* m_Window;

		Unique<D3D12API> m_API;
		Unique<D3D12CommandList> m_CommandList;
		Unique<D3D12Renderer> m_Renderer;
		Unique<D3D12FrameBuffer> m_FrameBuffer;
		Unique<D3D12Swapchain> m_Swapchain;
    };

}