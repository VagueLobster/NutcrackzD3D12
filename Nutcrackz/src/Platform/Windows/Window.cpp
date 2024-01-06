#include "Nutcrackz/Core/Window.hpp"
#include "Nutcrackz/Renderer/Renderer.hpp"

#include "WindowsCommon.hpp"

namespace Nutcrackz {

	//static std::unordered_map<Window*, HWND> s_WindowHandles;

	LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		if (message == WM_NCCREATE || message == WM_CREATE)
		{
			SetWindowLongPtrA(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams));
		}

		auto* window = reinterpret_cast<Window*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));

		switch (message)
		{
		case WM_SIZE:
		{
			if (window->D3D12Renderer)
			{
				if (window->D3D12Renderer->GetAPI()->Device != NULL && wParam != SIZE_MINIMIZED)
				{
					//HRESULT result = window->D3D12Renderer->GetRenderer()->Swapchain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT);
					HRESULT result = window->D3D12Renderer->GetRenderer()->Swapchain->ResizeBuffers(s_BackbufferCount, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT);
					//HRESULT result = window->D3D12Renderer->GetRenderer()->Swapchain->ResizeBuffers(s_BackbufferCount, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_R8G8B8A8_UNORM, 0);
					//assert(SUCCEEDED(result) && "Failed to resize swapchain.");
				}

				UINT width = LOWORD(lParam);
				UINT height = HIWORD(lParam);
				window->D3D12Renderer->Resize(static_cast<int32_t>(width), static_cast<int32_t>(height));
			}
			break;
		}
		case WM_CLOSE:
		{
			window->Close();
			break;
		}
		}

		return DefWindowProcA(hwnd, message, wParam, lParam);
	}

	WindowSystem::WindowSystem()
	{
		WNDCLASSEXA windowClass =
		{
			.cbSize = sizeof(WNDCLASSEXA),
			.lpfnWndProc = WindowProc,
			.hInstance = GetModuleHandle(nullptr),
			.hCursor = LoadCursor(nullptr, IDC_ARROW),
			.lpszClassName = "NutcrackzWindowClass",
		};

		RegisterClassExA(&windowClass);
	}

	WindowSystem::~WindowSystem()
	{

	}

	Window* WindowSystem::NewWindow(std::string_view title, uint32_t width, uint32_t height)
	{
		Unique<Window> window = Unique<Window>::New();
		Window* windowPtr = window.Raw();

		windowPtr->m_WindowSpec.Width = width;
		windowPtr->m_WindowSpec.Height = height;

		HWND handle = CreateWindowExA(
			NULL,
			"NutcrackzWindowClass",
			title.data(),
			WS_OVERLAPPEDWINDOW,
			(GetSystemMetrics(SM_CXSCREEN) / 2) - (width / 2),
			(GetSystemMetrics(SM_CYSCREEN) / 2) - (height / 2),
			width,
			height,
			NULL,
			NULL,
			GetModuleHandle(nullptr),
			windowPtr
		);

		Window::s_WindowHandles[windowPtr] = handle;
		m_Windows.push_back(std::move(window));

		ShowWindow(handle, SW_SHOWNORMAL);

		return windowPtr;
	}

	void WindowSystem::Init(Window* window)
	{
		window->D3D12Renderer = Unique<Renderer>::New(window);
	}

	void WindowSystem::Render(Window* window)
	{
		window->D3D12Renderer->Render();
	}

	void WindowSystem::PollEvents() const
	{
		MSG msg;

		for (auto [window, handle] : Window::s_WindowHandles)
		{
			while (PeekMessageA(&msg, handle, NULL, NULL, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

}