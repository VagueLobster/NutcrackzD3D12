#pragma once

#include "Platform/DirectX12/D3D12Common.hpp"
#include "Platform/DirectX12/D3D12API.hpp"
#include "Platform/DirectX12/D3D12CommandList.hpp"
#include "Platform/DirectX12/D3D12Renderer.hpp"

namespace Nutcrackz {

	class D3D12FrameBuffer
	{
	public:
		void InitFrameBuffer(D3D12API* api, D3D12CommandList* commandList, D3D12Renderer* renderer);
		void DestroyFrameBuffer(D3D12CommandList* commandList);

	private:
		void ThrowIfFailed(HRESULT hr);
	};

}