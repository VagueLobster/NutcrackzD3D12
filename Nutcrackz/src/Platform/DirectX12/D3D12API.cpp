#include "nzpch.hpp"
#include "D3D12API.hpp"

#include "Nutcrackz/Core/Window.hpp"

//#define NOMINMAX
//#define WIN32_LEAN_AND_MEAN
//#include <windows.h>

namespace Nutcrackz {

	void D3D12API::InitAPI(Window* window)
	{
		// Create Factory
		uint32_t dxgiFactoryFlags = 0;

#ifdef NZ_DEBUG
		ID3D12Debug* localDebugController;
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&localDebugController)));
		ThrowIfFailed(localDebugController->QueryInterface(IID_PPV_ARGS(&m_DebugController)));
		m_DebugController->EnableDebugLayer();
		m_DebugController->SetEnableGPUBasedValidation(true);

		dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;

		localDebugController->Release();
		localDebugController = nullptr;
#endif

		ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&Factory)));

		// Create Adapter
		for (uint32_t adapterIndex = 0; DXGI_ERROR_NOT_FOUND != Factory->EnumAdapters1(adapterIndex, &Adapter); ++adapterIndex)
		{
			DXGI_ADAPTER_DESC1 desc;
			Adapter->GetDesc1(&desc);

			// Don't select the Basic Render Driver adapter.
			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				continue;

			// Check to see if the adapter supports Direct3D 12, but don't create
			// the actual device yet.
			if (SUCCEEDED(D3D12CreateDevice(Adapter, D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), nullptr)))
				break;

			// We won't use this adapter, so release it
			Adapter->Release();
		}

		// Create Device
		ThrowIfFailed(D3D12CreateDevice(Adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&Device)));

		Device->SetName(L"Nutcrackz Device");

#ifdef NZ_DEBUG
		// Get debug device
		ThrowIfFailed(Device->QueryInterface(&m_DebugDevice));
#endif

		// Create Command Queue
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

		ThrowIfFailed(Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&CommandQueue)));

		// Create Command Allocator
		ThrowIfFailed(Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&CommandAllocator)));

		// Sync
		ThrowIfFailed(Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence)));
	}

	void D3D12API::DestroyAPI()
	{
		if (Fence)
		{
			Fence->Release();
			Fence = nullptr;
		}

		if (CommandAllocator)
		{
			ThrowIfFailed(CommandAllocator->Reset());
			CommandAllocator->Release();
			CommandAllocator = nullptr;
		}

		if (CommandQueue)
		{
			CommandQueue->Release();
			CommandQueue = nullptr;
		}

		if (Device)
		{
			Device->Release();
			Device = nullptr;
		}

		if (Adapter)
		{
			Adapter->Release();
			Adapter = nullptr;
		}

		if (Factory)
		{
			Factory->Release();
			Factory = nullptr;
		}

#ifdef NZ_DEBUG
		if (m_DebugController)
		{
			m_DebugController->Release();
			m_DebugController = nullptr;
		}

		D3D12_RLDO_FLAGS flags = D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL;

		m_DebugDevice->ReportLiveDeviceObjects(flags);

		if (m_DebugDevice)
		{
			m_DebugDevice->Release();
			m_DebugDevice = nullptr;
		}
#endif
	}

	void D3D12API::ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
			throw std::exception();
	}

}