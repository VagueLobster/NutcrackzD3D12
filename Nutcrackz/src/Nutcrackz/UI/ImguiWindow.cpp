#include "nzpch.hpp"
#include "ImguiWindow.hpp"

#include "Nutcrackz/Core/Window.hpp"

#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx12.h>
#include <d3d12.h>
#include <dxgi1_4.h>

#ifdef NZ_DEBUG
#define DX12_ENABLE_DEBUG_LAYER
#endif

#ifdef DX12_ENABLE_DEBUG_LAYER
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif

#define IMGUI_UNLIMITED_FRAME_RATE

namespace Nutcrackz {

	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	struct FrameContext
	{
		ID3D12CommandAllocator* CommandAllocator;
		UINT64                  FenceValue;
	};

	// Data
	static int const NUM_FRAMES_IN_FLIGHT = 3;
	static FrameContext g_frameContext[NUM_FRAMES_IN_FLIGHT] = {};
	static UINT g_frameIndex = 0;

	static int const NUM_BACK_BUFFERS = 3;
	static ID3D12Device* g_pd3dDevice = NULL;
	static ID3D12DescriptorHeap* g_pd3dRtvDescHeap = NULL;
	static ID3D12DescriptorHeap* g_pd3dSrvDescHeap = NULL;
	static ID3D12CommandQueue* g_pd3dCommandQueue = NULL;
	static ID3D12GraphicsCommandList* g_pd3dCommandList = NULL;
	static ID3D12Fence* g_fence = NULL;
	static HANDLE g_fenceEvent = NULL;
	static UINT64 g_fenceLastSignaledValue = 0;
	static IDXGISwapChain3* g_pSwapChain = NULL;
	static HANDLE g_hSwapChainWaitableObject = NULL;
	static ID3D12Resource* g_mainRenderTargetResource[NUM_BACK_BUFFERS] = {};
	static D3D12_CPU_DESCRIPTOR_HANDLE g_mainRenderTargetDescriptor[NUM_BACK_BUFFERS] = {};

	ImGuiWindow::~ImGuiWindow()
	{
		Shutdown();
	}

	bool ImGuiWindow::Init(Window* window)
	{
		if (!CreateDeviceD3D(Window::s_WindowHandles[window]))
		{
			CleanupDeviceD3D();
			return false;
		}

		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
		io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
		//io.ConfigViewportsNoAutoMerge = true;
		//io.ConfigViewportsNoTaskBarIcon = true;

		//io.FontDefault = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/consola.ttf", 1.5f * 13.333333f);
		//io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/consola.ttf", 1.5f * 13.333333f);
		//io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/arial.ttf", 1.5f * 16.0f);

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsClassic();

		// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
		ImGuiStyle& style = ImGui::GetStyle();

		style.WindowMenuButtonPosition = ImGuiDir_None; // Comment this out, to get the windows' (hide) arrows back!
		style.TabRounding = 0.0f; // Comment this out, to get rounded tabs back!
		style.ScaleAllSizes(1.5f);

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		ImGui_ImplWin32_Init(Window::s_WindowHandles[window]);
		ImGui_ImplDX12_Init(g_pd3dDevice, NUM_FRAMES_IN_FLIGHT,
			DXGI_FORMAT_R8G8B8A8_UNORM, g_pd3dSrvDescHeap,
			g_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
			g_pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart());

		//ImGui_ImplWin32_Init(Window::s_WindowHandles[window]);
		//ImGui_ImplDX12_Init(window->D3D12Renderer->GetAPI()->Device, s_BackbufferCount,
		//	DXGI_FORMAT_R8G8B8A8_UNORM, window->D3D12Renderer->GetCommandList()->DescriptorHeap,
		//	window->D3D12Renderer->GetCommandList()->DescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		//	window->D3D12Renderer->GetCommandList()->DescriptorHeap->GetGPUDescriptorHandleForHeapStart());

		return true;
	}

	void ImGuiWindow::Shutdown()
	{
		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}

	void ImGuiWindow::Begin()
	{
		//NZ_PROFILE_FUNCTION();

		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		//ImGuizmo::BeginFrame();
	}

	void ImGuiWindow::End(Window* window, uint32_t width, uint32_t height)
	{
		//NZ_PROFILE_FUNCTION();

		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2((float)width, (float)height);

		// Rendering
		ImGui::Render();

		FrameContext* frameCtx = WaitForNextFrameResources();
		UINT backBufferIdx = g_pSwapChain->GetCurrentBackBufferIndex();
		frameCtx->CommandAllocator->Reset();

		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = g_mainRenderTargetResource[backBufferIdx];
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		g_pd3dCommandList->Reset(frameCtx->CommandAllocator, NULL);
		g_pd3dCommandList->ResourceBarrier(1, &barrier);

		// Render Dear ImGui graphics
		const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
		g_pd3dCommandList->ClearRenderTargetView(g_mainRenderTargetDescriptor[backBufferIdx], clear_color_with_alpha, 0, NULL);
		g_pd3dCommandList->OMSetRenderTargets(1, &g_mainRenderTargetDescriptor[backBufferIdx], FALSE, NULL);
		g_pd3dCommandList->SetDescriptorHeaps(1, &g_pd3dSrvDescHeap);
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), g_pd3dCommandList);
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		g_pd3dCommandList->ResourceBarrier(1, &barrier);
		g_pd3dCommandList->Close();

		g_pd3dCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&g_pd3dCommandList);

		// Update and Render additional Platform Windows
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault(NULL, (void*)g_pd3dCommandList);
		}

		g_pSwapChain->Present(1, 0); // Present with vsync
		//g_pSwapChain->Present(0, 0); // Present without vsync

		UINT64 fenceValue = g_fenceLastSignaledValue + 1;
		g_pd3dCommandQueue->Signal(g_fence, fenceValue);
		g_fenceLastSignaledValue = fenceValue;
		frameCtx->FenceValue = fenceValue;
		// Rendering
		/*ImGui::Render();

		FrameContext* frameCtx = WaitForNextFrameResources(window);
		UINT backBufferIdx = window->D3D12Renderer->GetRenderer()->Swapchain->GetCurrentBackBufferIndex();
		frameCtx->CommandAllocator->Reset();

		window->D3D12Renderer->GetCommandList()->CommandList->Reset(frameCtx->CommandAllocator, NULL);

		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), window->D3D12Renderer->GetCommandList()->CommandList);
		//ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), NULL);
		
		// Update and Render additional Platform Windows
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();		
			ImGui::RenderPlatformWindowsDefault(NULL, (void*)window->D3D12Renderer->GetCommandList()->CommandList);		
			//ImGui::RenderPlatformWindowsDefault();
		}

		UINT64 fenceValue = g_fenceLastSignaledValue + 1;
		window->D3D12Renderer->GetAPI()->CommandQueue->Signal(window->D3D12Renderer->GetAPI()->Fence, fenceValue);
		g_fenceLastSignaledValue = fenceValue;
		frameCtx->FenceValue = fenceValue;*/
	}

	bool ImGuiWindow::CreateDeviceD3D(HWND hWnd)
	{
		// Setup swap chain
		DXGI_SWAP_CHAIN_DESC1 sd;
		{
			ZeroMemory(&sd, sizeof(sd));
			sd.BufferCount = NUM_BACK_BUFFERS;
			sd.Width = 0;
			sd.Height = 0;
			sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			sd.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
			sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			sd.SampleDesc.Count = 1;
			sd.SampleDesc.Quality = 0;
			sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
			sd.Scaling = DXGI_SCALING_STRETCH;
			sd.Stereo = FALSE;
		}

		// [DEBUG] Enable debug interface
#ifdef DX12_ENABLE_DEBUG_LAYER
		ID3D12Debug* pdx12Debug = NULL;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pdx12Debug))))
			pdx12Debug->EnableDebugLayer();
#endif

		// Create device
		D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
		if (D3D12CreateDevice(NULL, featureLevel, IID_PPV_ARGS(&g_pd3dDevice)) != S_OK)
			return false;

		// [DEBUG] Setup debug interface to break on any warnings/errors
#ifdef DX12_ENABLE_DEBUG_LAYER
		if (pdx12Debug != NULL)
		{
			ID3D12InfoQueue* pInfoQueue = NULL;
			g_pd3dDevice->QueryInterface(IID_PPV_ARGS(&pInfoQueue));
			pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
			pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
			pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
			pInfoQueue->Release();
			pdx12Debug->Release();
		}
#endif

		{
			D3D12_DESCRIPTOR_HEAP_DESC desc = {};
			desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			desc.NumDescriptors = NUM_BACK_BUFFERS;
			desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			desc.NodeMask = 1;
			if (g_pd3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dRtvDescHeap)) != S_OK)
				return false;

			SIZE_T rtvDescriptorSize = g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = g_pd3dRtvDescHeap->GetCPUDescriptorHandleForHeapStart();
			for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
			{
				g_mainRenderTargetDescriptor[i] = rtvHandle;
				rtvHandle.ptr += rtvDescriptorSize;
			}
		}

		{
			D3D12_DESCRIPTOR_HEAP_DESC desc = {};
			desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			desc.NumDescriptors = 1;
			desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			if (g_pd3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dSrvDescHeap)) != S_OK)
				return false;
		}

		{
			D3D12_COMMAND_QUEUE_DESC desc = {};
			desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
			desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			desc.NodeMask = 1;
			if (g_pd3dDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&g_pd3dCommandQueue)) != S_OK)
				return false;
		}

		for (UINT i = 0; i < NUM_FRAMES_IN_FLIGHT; i++)
			if (g_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_frameContext[i].CommandAllocator)) != S_OK)
				return false;

		if (g_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_frameContext[0].CommandAllocator, NULL, IID_PPV_ARGS(&g_pd3dCommandList)) != S_OK ||
			g_pd3dCommandList->Close() != S_OK)
			return false;

		if (g_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence)) != S_OK)
			return false;

		g_fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		if (g_fenceEvent == NULL)
			return false;

		{
			IDXGIFactory4* dxgiFactory = NULL;
			IDXGISwapChain1* swapChain1 = NULL;
			if (CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)) != S_OK)
				return false;
			if (dxgiFactory->CreateSwapChainForHwnd(g_pd3dCommandQueue, hWnd, &sd, NULL, NULL, &swapChain1) != S_OK)
				return false;
			if (swapChain1->QueryInterface(IID_PPV_ARGS(&g_pSwapChain)) != S_OK)
				return false;
			swapChain1->Release();
			dxgiFactory->Release();
			g_pSwapChain->SetMaximumFrameLatency(NUM_BACK_BUFFERS);
			g_hSwapChainWaitableObject = g_pSwapChain->GetFrameLatencyWaitableObject();
		}

		CreateRenderTarget();
		return true;
	}

	void ImGuiWindow::CleanupDeviceD3D()
	{
		CleanupRenderTarget();
		if (g_pSwapChain) { g_pSwapChain->SetFullscreenState(false, NULL); g_pSwapChain->Release(); g_pSwapChain = NULL; }
		if (g_hSwapChainWaitableObject != NULL) { CloseHandle(g_hSwapChainWaitableObject); }
		for (UINT i = 0; i < NUM_FRAMES_IN_FLIGHT; i++)
			if (g_frameContext[i].CommandAllocator) { g_frameContext[i].CommandAllocator->Release(); g_frameContext[i].CommandAllocator = NULL; }
		if (g_pd3dCommandQueue) { g_pd3dCommandQueue->Release(); g_pd3dCommandQueue = NULL; }
		if (g_pd3dCommandList) { g_pd3dCommandList->Release(); g_pd3dCommandList = NULL; }
		if (g_pd3dRtvDescHeap) { g_pd3dRtvDescHeap->Release(); g_pd3dRtvDescHeap = NULL; }
		if (g_pd3dSrvDescHeap) { g_pd3dSrvDescHeap->Release(); g_pd3dSrvDescHeap = NULL; }
		if (g_fence) { g_fence->Release(); g_fence = NULL; }
		if (g_fenceEvent) { CloseHandle(g_fenceEvent); g_fenceEvent = NULL; }
		if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }

#ifdef DX12_ENABLE_DEBUG_LAYER
		IDXGIDebug1* pDebug = NULL;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))) == S_OK) // (== S_OK) inserted by me to not crash the application!
		{
			pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
			pDebug->Release();
		}
#endif
	}

	void ImGuiWindow::CreateRenderTarget()
	{
		for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
		{
			ID3D12Resource* pBackBuffer = NULL;
			g_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
			g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, g_mainRenderTargetDescriptor[i]);
			g_mainRenderTargetResource[i] = pBackBuffer;
		}
	}

	void ImGuiWindow::CleanupRenderTarget()
	{
		WaitForLastSubmittedFrame();

		for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
			if (g_mainRenderTargetResource[i]) { g_mainRenderTargetResource[i]->Release(); g_mainRenderTargetResource[i] = NULL; }
	}

	void ImGuiWindow::WaitForLastSubmittedFrame()
	{
		FrameContext* frameCtx = &g_frameContext[g_frameIndex % NUM_FRAMES_IN_FLIGHT];

		UINT64 fenceValue = frameCtx->FenceValue;
		if (fenceValue == 0)
			return; // No fence was signaled

		frameCtx->FenceValue = 0;
		if (g_fence->GetCompletedValue() >= fenceValue)
			return;

		g_fence->SetEventOnCompletion(fenceValue, g_fenceEvent);
		WaitForSingleObject(g_fenceEvent, INFINITE);
	}

	FrameContext* ImGuiWindow::WaitForNextFrameResources()
	{
		UINT nextFrameIndex = g_frameIndex + 1;
		g_frameIndex = nextFrameIndex;

		HANDLE waitableObjects[] = { g_hSwapChainWaitableObject, NULL };
		DWORD numWaitableObjects = 1;

		FrameContext* frameCtx = &g_frameContext[nextFrameIndex % NUM_FRAMES_IN_FLIGHT];
		UINT64 fenceValue = frameCtx->FenceValue;
		if (fenceValue != 0) // means no fence was signaled
		{
			frameCtx->FenceValue = 0;
			g_fence->SetEventOnCompletion(fenceValue, g_fenceEvent);
			waitableObjects[1] = g_fenceEvent;
			numWaitableObjects = 2;
		}

		WaitForMultipleObjects(numWaitableObjects, waitableObjects, TRUE, INFINITE);

		return frameCtx;
	}

	/*void ImGuiWindow::SetDarkThemeColors()
	{
		Hazard::ImUI::Style colorStyle = Hazard::ImUI::Style();

		colorStyle.BackgroundColor = ImVec4{ 0.1f,  0.105f,  0.105f,  1.0f };
		colorStyle.Window.PopupBgColor = ImVec4{ 0.05f,  0.0505f,  0.051f,  1.0f };
		colorStyle.DragDropTarget = ConvertColorFromByteToFloats(93, 197, 5, 112);

		// Misc.
		colorStyle.Window.SliderGrab = ConvertColorFromByteToFloats(93, 197, 5, 112);
		colorStyle.Window.SliderGrabActive = ConvertColorFromByteToFloats(73, 159, 5, 255);

		// Headers
		colorStyle.Window.Header = ImVec4{ 0.15f,  0.1505f,  0.151f,  1.0f };
		colorStyle.Window.HeaderHovered = ConvertColorFromByteToFloats(93, 197, 5, 112);
		colorStyle.Window.HeaderActive = ConvertColorFromByteToFloats(93, 197, 5, 168);

		// Buttons
		colorStyle.Button.Button = ImVec4{ 0.15f,  0.1505f,  0.151f,  1.0f };
		colorStyle.Button.ButtonHovered = ConvertColorFromByteToFloats(93, 197, 5, 112);
		colorStyle.Button.ButtonActive = ConvertColorFromByteToFloats(73, 159, 5, 255);

		// Frame BG
		colorStyle.Frame.FrameColor = ImVec4{ 0.15f,  0.1505f,  0.151f,  1.0f };
		colorStyle.Frame.FrameHovered = ImVec4{ 0.15f,  0.1505f,  0.151f,  1.0f };
		colorStyle.Frame.FrameActive = ImVec4{ 0.15f,  0.1505f,  0.151f,  1.0f };

		// Tabs
		colorStyle.Tab.Tab = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colorStyle.Tab.TabHovered = ConvertColorFromByteToFloats(93, 197, 5, 197);
		colorStyle.Tab.TabActiveFocus = ConvertColorFromByteToFloats(93, 197, 5, 112);
		colorStyle.Tab.TabUnfocus = ImVec4{ 0.13f, 0.13005f, 0.1301f, 1.0f };

		Hazard::ImUI::StyleManager::LoadStyle(colorStyle);
		//Hazard::ImUI::StyleManager::LoadStyle(Hazard::ImUI::Style());
	}

	void ImGuiWindow::SetGoldDarkThemeColors()
	{
		Hazard::ImUI::Style colorStyle = Hazard::ImUI::Style();

		colorStyle.BackgroundColor = ImVec4{ 0.1f, 0.105f, 0.11f, 1.0f };
		colorStyle.Window.PopupBgColor = ImVec4{ 0.1f,  0.105f,  0.11f,  0.95f };
		colorStyle.ChildBackgroundColor = ImVec4{ 0.1f, 0.105f, 0.11f, 1.0f };
		colorStyle.Window.Border = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colorStyle.Window.Text = ImVec4{ 0.8f, 0.805f, 0.81f, 1.0f };
		colorStyle.Window.TextSelectedBg = ImVec4{ 0.48f, 0.4805f, 0.381f, 1.0f };
		colorStyle.Window.TextDisabled = ImVec4{ 0.48f, 0.4805f, 0.381f, 1.0f };
		colorStyle.DragDropTarget = ImVec4{ 0.48f, 0.4805f, 0.381f, 1.0f };
		colorStyle.MenuBarBackground = ImVec4{ 0.1f,  0.105f,  0.11f,  1.0f };

		// Misc.
		colorStyle.Window.Checkmark = ImVec4{ 0.55f, 0.555f, 0.31f, 1.0f };
		colorStyle.Window.SliderGrab = ImVec4{ 0.25f, 0.2505f, 0.151f, 1.0f };
		colorStyle.Window.SliderGrabActive = ImVec4{ 0.38f, 0.3805f, 0.281f, 1.0f };

		// Separator
		colorStyle.Separator.Separator = ImVec4{ 0.45f, 0.455f, 0.21f, 1.0f };
		colorStyle.Separator.SeparatorHovered = ImVec4{ 0.55f, 0.555f, 0.31f, 1.0f };
		colorStyle.Separator.SeparatorActive = ImVec4{ 0.4f, 0.405f, 0.151f, 1.0f };

		// Scrollbar
		colorStyle.ScrollBar.ScrollbarGrab = ImVec4{ 0.45f, 0.455f, 0.21f, 1.0f };
		colorStyle.ScrollBar.ScrollbarGrabHovered = ImVec4{ 0.55f, 0.555f, 0.31f, 1.0f };
		colorStyle.ScrollBar.ScrollbarGrabActive = ImVec4{ 0.4f, 0.405f, 0.151f, 1.0f };

		// Headers
		colorStyle.Window.Header = ImVec4{ 0.45f, 0.455f, 0.21f, 1.0f };
		colorStyle.Window.HeaderHovered = ImVec4{ 0.55f, 0.555f, 0.31f, 1.0f };
		colorStyle.Window.HeaderActive = ImVec4{ 0.4f, 0.405f, 0.151f, 1.0f };

		// Buttons
		colorStyle.Button.Button = ImVec4{ 0.45f, 0.455f, 0.21f, 1.0f };
		colorStyle.Button.ButtonHovered = ImVec4{ 0.55f, 0.555f, 0.31f, 1.0f };
		colorStyle.Button.ButtonActive = ImVec4{ 0.4f, 0.405f, 0.151f, 1.0f };

		// Frame BG
		colorStyle.Frame.FrameColor = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
		colorStyle.Frame.FrameHovered = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
		colorStyle.Frame.FrameActive = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		// Tabs
		colorStyle.Tab.Tab = ImVec4{ 0.45f, 0.455f, 0.21f, 1.0f };
		colorStyle.Tab.TabHovered = ImVec4{ 0.55f, 0.555f, 0.31f, 1.0f };
		colorStyle.Tab.TabActive = ImVec4{ 0.4f, 0.405f, 0.151f, 1.0f };
		colorStyle.Tab.TabActiveFocus = ImVec4{ 0.4f, 0.405f, 0.151f, 1.0f };
		colorStyle.Tab.TabUnfocus = ImVec4{ 0.25f, 0.2505f, 0.151f, 1.0f };

		// Title
		colorStyle.Window.TitleBgColor = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colorStyle.Window.TitleBgActive = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		//Hazard::ImUI::StyleManager::LoadStyle(Hazard::ImUI::Style());
		Hazard::ImUI::StyleManager::LoadStyle(colorStyle);
	}

	void ImGuiWindow::SetChocolateThemeColors()
	{
		Hazard::ImUI::Style colorStyle = Hazard::ImUI::Style();

		colorStyle.BackgroundColor = ConvertColorFromByteToFloats(42, 33, 28, 255);
		colorStyle.Window.PopupBgColor = ConvertColorFromByteToFloats(42, 33, 28, 243);
		colorStyle.ChildBackgroundColor = ConvertColorFromByteToFloats(42, 33, 28, 255);
		colorStyle.Window.Border = ConvertColorFromByteToFloats(42, 33, 28, 255);
		colorStyle.Window.Text = ConvertColorFromByteToFloats(189, 174, 157, 255);
		colorStyle.Window.TextSelectedBg = ConvertColorFromByteToFloats(75, 60, 52, 255);
		colorStyle.Window.TextDisabled = ConvertColorFromByteToFloats(42, 33, 28, 255);
		colorStyle.DragDropTarget = ConvertColorFromByteToFloats(75, 60, 52, 255);
		colorStyle.MenuBarBackground = ConvertColorFromByteToFloats(42, 33, 28, 255);

		// Misc.
		colorStyle.Window.Checkmark = ConvertColorFromByteToFloats(189, 174, 157, 255);
		colorStyle.Window.SliderGrab = ConvertColorFromByteToFloats(189, 174, 157, 255);
		colorStyle.Window.SliderGrabActive = ConvertColorFromByteToFloats(42, 33, 28, 255);

		// Separator
		colorStyle.Separator.Separator = ConvertColorFromByteToFloats(119, 92, 79, 255);
		colorStyle.Separator.SeparatorHovered = ConvertColorFromByteToFloats(158, 126, 109, 255);
		colorStyle.Separator.SeparatorActive = ConvertColorFromByteToFloats(75, 60, 52, 255);

		// Scrollbar
		colorStyle.ScrollBar.ScrollbarGrab = ConvertColorFromByteToFloats(119, 92, 79, 255);
		colorStyle.ScrollBar.ScrollbarGrabHovered = ConvertColorFromByteToFloats(158, 126, 109, 255);
		colorStyle.ScrollBar.ScrollbarGrabActive = ConvertColorFromByteToFloats(75, 60, 52, 255);

		// Headers
		colorStyle.Window.Header = ConvertColorFromByteToFloats(119, 92, 79, 255);
		colorStyle.Window.HeaderHovered = ConvertColorFromByteToFloats(158, 126, 109, 255);
		colorStyle.Window.HeaderActive = ConvertColorFromByteToFloats(75, 60, 52, 255);

		// Buttons
		colorStyle.Button.Button = ConvertColorFromByteToFloats(119, 92, 79, 255);
		colorStyle.Button.ButtonHovered = ConvertColorFromByteToFloats(158, 126, 109, 255);
		colorStyle.Button.ButtonActive = ConvertColorFromByteToFloats(75, 60, 52, 255);

		// Frame BG
		colorStyle.Frame.FrameColor = ConvertColorFromByteToFloats(119, 92, 79, 255);
		colorStyle.Frame.FrameHovered = ConvertColorFromByteToFloats(158, 126, 109, 255);
		colorStyle.Frame.FrameActive = ConvertColorFromByteToFloats(75, 60, 52, 255);

		// Tabs
		colorStyle.Tab.Tab = ConvertColorFromByteToFloats(119, 92, 79, 255);
		colorStyle.Tab.TabHovered = ConvertColorFromByteToFloats(149, 117, 100, 255);
		colorStyle.Tab.TabActive = ConvertColorFromByteToFloats(42, 33, 28, 255);
		colorStyle.Tab.TabActiveFocus = ConvertColorFromByteToFloats(42, 33, 28, 255);
		colorStyle.Tab.TabUnfocus = ConvertColorFromByteToFloats(119, 92, 79, 255);

		// Title
		colorStyle.Window.TitleBgColor = ConvertColorFromByteToFloats(119, 92, 79, 255);
		colorStyle.Window.TitleBgActive = ConvertColorFromByteToFloats(119, 92, 79, 255);

		//Hazard::ImUI::StyleManager::LoadStyle(Hazard::ImUI::Style());
		Hazard::ImUI::StyleManager::LoadStyle(colorStyle);
	}

	void ImGuiWindow::SetLightThemeColors()
	{
		Hazard::ImUI::Style colorStyle = Hazard::ImUI::Style();

		colorStyle.BackgroundColor = ImVec4{ 0.38f, 0.3805f, 0.381f, 1.0f };
		colorStyle.ChildBackgroundColor = ImVec4{ 0.38f, 0.3805f, 0.381f, 1.0f };
		colorStyle.Window.PopupBgColor = ImVec4{ 0.65f, 0.6505f, 0.651f, 0.95f };
		colorStyle.Window.Border = ImVec4{ 0.55f, 0.5505f, 0.551f, 1.0f };
		colorStyle.DragDropTarget = ImVec4{ 0.88f, 0.8805f, 0.881f, 1.0f };
		colorStyle.MenuBarBackground = ImVec4{ 0.8f, 0.805f, 0.81f, 1.0f };
		colorStyle.ModalBackgroundColor = ImVec4{ 0.1f, 0.105f, 0.11f, 0.65f };

		// Text
		colorStyle.Window.Text = ImVec4{ 0.18f, 0.1805f, 0.181f, 1.0f };
		colorStyle.Window.TextSelectedBg = ImVec4{ 0.85f, 0.8505f, 0.851f, 1.0f };
		colorStyle.Window.TextDisabled = ImVec4{ 0.08f, 0.0805f, 0.081f, 1.0f };

		// Misc.
		colorStyle.Window.Checkmark = ImVec4{ 0.88f, 0.8805f, 0.881f, 1.0f };
		colorStyle.Window.SliderGrab = ImVec4{ 0.55f, 0.5505f, 0.551f, 1.0f };
		colorStyle.Window.SliderGrabActive = ImVec4{ 0.68f, 0.6805f, 0.681f, 1.0f };

		// Separator
		colorStyle.Separator.Separator = ImVec4{ 0.45f, 0.455f, 0.46f, 1.0f };
		colorStyle.Separator.SeparatorHovered = ImVec4{ 0.55f, 0.555f, 0.56f, 1.0f };
		colorStyle.Separator.SeparatorActive = ImVec4{ 0.3f, 0.3005f, 0.301f, 1.0f };

		// Scrollbar
		colorStyle.ScrollBar.ScrollbarGrab = ImVec4{ 0.45f, 0.455f, 0.46f, 1.0f };
		colorStyle.ScrollBar.ScrollbarGrabHovered = ImVec4{ 0.55f, 0.555f, 0.56f, 1.0f };
		colorStyle.ScrollBar.ScrollbarGrabActive = ImVec4{ 0.3f, 0.3005f, 0.301f, 1.0f };

		// Headers
		colorStyle.Window.Header = ImVec4{ 0.55f, 0.5505f, 0.551f, 1.0f };
		colorStyle.Window.HeaderHovered = ImVec4{ 0.85f, 0.8505f, 0.851f, 1.0f };
		colorStyle.Window.HeaderActive = ImVec4{ 0.8f, 0.805f, 0.81f, 1.0f };

		// Buttons
		colorStyle.Button.Button = ImVec4{ 0.45f, 0.455f, 0.46f, 1.0f };
		colorStyle.Button.ButtonHovered = ImVec4{ 0.55f, 0.555f, 0.56f, 1.0f };
		colorStyle.Button.ButtonActive = ImVec4{ 0.8f, 0.805f, 0.81f, 1.0f };

		// Frame BG
		colorStyle.Frame.FrameColor = ImVec4{ 0.6f, 0.605f, 0.61f, 1.0f };
		colorStyle.Frame.FrameHovered = ImVec4{ 0.7f, 0.705f, 0.71f, 1.0f };
		colorStyle.Frame.FrameActive = ImVec4{ 0.55f, 0.5505f, 0.551f, 1.0f };

		// Tabs
		colorStyle.Tab.Tab = ImVec4{ 0.55f, 0.5505f, 0.551f, 1.0f };
		colorStyle.Tab.TabHovered = ImVec4{ 0.78f, 0.7805f, 0.781f, 1.0f };
		colorStyle.Tab.TabActive = ImVec4{ 0.68f, 0.6805f, 0.681f, 1.0f };
		colorStyle.Tab.TabActiveFocus = ImVec4{ 0.68f, 0.6805f, 0.681f, 1.0f };
		colorStyle.Tab.TabUnfocus = ImVec4{ 0.65f, 0.6505f, 0.651f, 1.0f };

		// Title
		colorStyle.Window.TitleBgColor = ImVec4{ 0.55f, 0.5505f, 0.551f, 1.0f };
		colorStyle.Window.TitleBgActive = ImVec4{ 0.55f, 0.5505f, 0.551f, 1.0f };

		//Hazard::ImUI::StyleManager::LoadStyle(Hazard::ImUI::Style());
		Hazard::ImUI::StyleManager::LoadStyle(colorStyle);
	}

	ImVec4 ImGuiWindow::ConvertColorFromByteToFloats(uint32_t r, uint32_t g, uint32_t b, uint32_t a)
	{
		return ImVec4((float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f, (float)a / 255.0f);
	}

	uint32_t ImGuiWindow::ConvertColorFromFloatToByte(float value)
	{
		return (value >= 1.0f ? 255 : (value <= 0.0f ? 0 : (int)floor(value * 256.0f)));
	}*/

}