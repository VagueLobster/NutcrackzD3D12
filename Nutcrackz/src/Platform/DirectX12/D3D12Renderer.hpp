#pragma once

#include "Platform/DirectX12/D3D12Common.hpp"
#include "Platform/DirectX12/D3D12API.hpp"
#include "Platform/DirectX12/D3D12CommandList.hpp"

#include "rtmcpp/Quat.hpp"
#include "rtmcpp/QuatOps.hpp"

namespace Nutcrackz {

	class D3D12Renderer
	{
	public:
		void InitResources(D3D12API* api, D3D12CommandList* commandList);
		void DestroyResources(D3D12CommandList* commandList);

		void Render(D3D12API* api, D3D12CommandList* commandList);

		void UpdateMVPMatrices(int32_t width, int32_t height);

	private:
		void ThrowIfFailed(HRESULT hr);

	public:
		std::chrono::time_point<std::chrono::steady_clock> StartTime, EndTime;

		uint32_t CurrentBuffer;
		IDXGISwapChain3* Swapchain = nullptr;
		ID3D12Resource* UniformBuffer = nullptr;

	private:
		ID3D12Resource* m_VertexBuffer = nullptr;
		ID3D12Resource* m_IndexBuffer = nullptr;

		uint8_t* m_MappedUniformBuffer = nullptr;

		rtmcpp::Quat4 m_Quat = rtmcpp::Quat4();
	};

}