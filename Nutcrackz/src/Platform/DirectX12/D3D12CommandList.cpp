#include "nzpch.hpp"
#include "D3D12CommandList.hpp"

namespace Nutcrackz {

	void D3D12CommandList::CreateCommands(D3D12API* api)
	{
		// Create the command list.
		ThrowIfFailed(api->Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, api->CommandAllocator, PipelineState, IID_PPV_ARGS(&CommandList)));
		CommandList->SetName(L"Nutcrackz Command List");
	}

	void D3D12CommandList::SetupCommands(D3D12API* api)
	{
		// Command list allocators can only be reset when the associated
		// command lists have finished execution on the GPU; apps should use
		// fences to determine GPU execution progress.
		ThrowIfFailed(api->CommandAllocator->Reset());

		// However, when ExecuteCommandList() is called on a particular command
		// list, that command list can then be reset at any time and must be before re-recording.
		ThrowIfFailed(CommandList->Reset(api->CommandAllocator, PipelineState));

		// Set necessary state.
		CommandList->SetGraphicsRootSignature(RootSignature);
		CommandList->RSSetViewports(1, &Viewport);
		CommandList->RSSetScissorRects(1, &SurfaceSize);

		ID3D12DescriptorHeap* pDescriptorHeaps[] = { DescriptorHeap };
		CommandList->SetDescriptorHeaps(_countof(pDescriptorHeaps), pDescriptorHeaps);

		D3D12_GPU_DESCRIPTOR_HANDLE srvHandle(DescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		CommandList->SetGraphicsRootDescriptorTable(0, srvHandle);
		GPURenderTargetDescriptor = srvHandle;

		// Indicate that the back buffer will be used as a render target.
		D3D12_RESOURCE_BARRIER renderTargetBarrier;
		renderTargetBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		renderTargetBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		renderTargetBarrier.Transition.pResource = RenderTargets[FrameIndex];
		renderTargetBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		renderTargetBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		renderTargetBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		CommandList->ResourceBarrier(1, &renderTargetBarrier);

		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(RtvHeap->GetCPUDescriptorHandleForHeapStart());
		rtvHandle.ptr = rtvHandle.ptr + (FrameIndex * RtvDescriptorSize);
		CommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
		CPURenderTargetDescriptor = rtvHandle;

		/*D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = RtvHeap->GetCPUDescriptorHandleForHeapStart();
		for (UINT i = 0; i < s_BackbufferCount; i++)
		{
			MainRenderTargetDescriptor[i] = rtvHandle;
			rtvHandle.ptr += RtvDescriptorSize;
			CommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
		}*/

		// Record commands.
		const float clearColor[] = { 0.2f, 0.2f, 0.2f, 1.0f };
		CommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
		CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		CommandList->IASetVertexBuffers(0, 1, &VertexBufferView);
		CommandList->IASetIndexBuffer(&IndexBufferView);

		CommandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

		// Indicate that the back buffer will now be used to present.
		D3D12_RESOURCE_BARRIER presentBarrier;
		presentBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		presentBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		presentBarrier.Transition.pResource = RenderTargets[FrameIndex];
		presentBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		presentBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		presentBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		CommandList->ResourceBarrier(1, &presentBarrier);

		ThrowIfFailed(CommandList->Close());
	}

	void D3D12CommandList::DestroyCommands(D3D12API* api)
	{
		if (CommandList)
		{
			CommandList->Reset(api->CommandAllocator, PipelineState);
			CommandList->ClearState(PipelineState);
			ThrowIfFailed(CommandList->Close());
			ID3D12CommandList* ppCommandLists[] = { CommandList };
			api->CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

			// Wait for GPU to finish work
			const uint64_t fence = FenceValue;
			ThrowIfFailed(api->CommandQueue->Signal(api->Fence, fence));
			FenceValue++;

			if (api->Fence->GetCompletedValue() < fence)
			{
				ThrowIfFailed(api->Fence->SetEventOnCompletion(fence, FenceEvent));
				WaitForSingleObject(FenceEvent, INFINITE);
			}

			CommandList->Release();
			CommandList = nullptr;
		}
	}

	void D3D12CommandList::ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
			throw std::exception();
	}

}