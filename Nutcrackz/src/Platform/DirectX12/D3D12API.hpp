#pragma once

#include "D3D12Common.hpp"

namespace Nutcrackz {

	class Window;

	class D3D12API
	{
	public:
		void InitAPI(Window* window);
		void DestroyAPI();

	private:
		void ThrowIfFailed(HRESULT hr);

	public:
		IDXGIAdapter1* Adapter = nullptr;
		IDXGIFactory4* Factory = nullptr;
		ID3D12Device* Device = nullptr;
		ID3D12CommandQueue* CommandQueue = nullptr;
		ID3D12CommandAllocator* CommandAllocator = nullptr;
		ID3D12Fence* Fence = nullptr;

	private:
#if defined(NZ_DEBUG)
		ID3D12Debug1* m_DebugController = nullptr;
		ID3D12DebugDevice* m_DebugDevice = nullptr;
#endif
	};

}