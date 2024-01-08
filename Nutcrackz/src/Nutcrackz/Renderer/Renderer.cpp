#include "nzpch.hpp"
#include "Renderer.hpp"

#include "Nutcrackz/Core/Window.hpp"

namespace Nutcrackz {

    Renderer::Renderer(Window* window)
    {
		m_Window = window;

        m_API = Unique<D3D12API>::New();
		m_CommandList = Unique<D3D12CommandList>::New();
		m_Renderer = Unique<D3D12Renderer>::New();
		m_FrameBuffer = Unique<D3D12FrameBuffer>::New();
		m_Swapchain = Unique<D3D12Swapchain>::New();

        for (uint32_t i = 0; i < s_BackbufferCount; ++i)
            m_CommandList->RenderTargets[i] = nullptr;

        m_API->InitAPI(window);

		// Create Swapchain
		Resize(window->m_WindowSpec.Width, window->m_WindowSpec.Height);

        m_Renderer->InitResources(m_API.Raw(), m_CommandList.Raw());
        m_CommandList->SetupCommands(m_API.Raw());
        m_Renderer->StartTime = std::chrono::high_resolution_clock::now();
    }

    Renderer::~Renderer()
    {
        if (m_Renderer->Swapchain != nullptr)
        {
            m_Renderer->Swapchain->SetFullscreenState(false, nullptr);
            m_Renderer->Swapchain->Release();
            m_Renderer->Swapchain = nullptr;
        }

        m_CommandList->DestroyCommands(m_API.Raw());
        m_FrameBuffer->DestroyFrameBuffer(m_CommandList.Raw());
        m_Renderer->DestroyResources(m_CommandList.Raw());
        m_API->DestroyAPI();
    }

    void Renderer::Render()
    {
        m_Renderer->Render(m_API.Raw(), m_CommandList.Raw());
    }

	void Renderer::Resize(int32_t width, int32_t height)
	{
        m_Swapchain->Resize(width, height, m_Window, m_API.Raw(), m_CommandList.Raw(), m_Renderer.Raw(), m_FrameBuffer.Raw());
	}

}