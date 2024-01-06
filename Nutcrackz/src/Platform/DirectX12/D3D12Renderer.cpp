#include "nzpch.hpp"
#include "D3D12Renderer.hpp"

namespace Nutcrackz {

	struct QuadVertex
	{
		float Position[3]; // Switch to RTM ASAP!
		float Color[3];    // Switch to RTM ASAP!
	};

	struct Renderer2DData
	{
		//glm::vec3 QuadVertexPositions[4];
		QuadVertex QuadVertexPositions[4] = {
			{ { -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
			{ {  0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
			{ {  0.5f,  0.5f, 0.0f }, { 0.0f, 1.0f, 1.0f } },
			{ { -0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
		};

		uint32_t Indices[6] = {
			0, 1, 2,
			2, 3, 0
		};

		MVPMatrices MvpMatrices;
	};

	static Renderer2DData s_Data;

	void D3D12Renderer::InitResources(D3D12API* api, D3D12CommandList* commandList)
	{
		// Create the root signature.
		{
			D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

			// This is the highest version the sample supports. If
			// CheckFeatureSupport succeeds, the HighestVersion returned will not be
			// greater than this.
			featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

			if (FAILED(api->Device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
				featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;

			D3D12_DESCRIPTOR_RANGE1 ranges[1];
			ranges[0].BaseShaderRegister = 0;
			ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
			ranges[0].NumDescriptors = 1;
			ranges[0].RegisterSpace = 0;
			ranges[0].OffsetInDescriptorsFromTableStart = 0;
			ranges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

			D3D12_ROOT_PARAMETER1 rootParameters[1];
			rootParameters[0].ParameterType =
				D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

			rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
			rootParameters[0].DescriptorTable.pDescriptorRanges = ranges;

			D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
			rootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
			rootSignatureDesc.Desc_1_1.Flags =
				D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
			rootSignatureDesc.Desc_1_1.NumParameters = 1;
			rootSignatureDesc.Desc_1_1.pParameters = rootParameters;
			rootSignatureDesc.Desc_1_1.NumStaticSamplers = 0;
			rootSignatureDesc.Desc_1_1.pStaticSamplers = nullptr;

			ID3DBlob* signature;
			ID3DBlob* error;

			try
			{
				ThrowIfFailed(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &signature, &error));
				ThrowIfFailed(api->Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&commandList->RootSignature)));
				commandList->RootSignature->SetName(L"Hello Triangle Root Signature");
			}
			catch (std::exception e)
			{
				const char* errStr = (const char*)error->GetBufferPointer();
				std::cout << errStr;
				error->Release();
				error = nullptr;
			}

			if (signature)
			{
				signature->Release();
				signature = nullptr;
			}
		}

		// Create the pipeline state, which includes compiling and loading shaders.
		{
			ID3DBlob* vertexShader = nullptr;
			ID3DBlob* pixelShader = nullptr;
			ID3DBlob* errors = nullptr;

#if defined(NZ_DEBUG)
			// Enable better shader debugging with the graphics debugging tools.
			uint32_t compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
			uint32_t compileFlags = 0;
#endif

			std::string path = "";
			char pBuf[1024];

			_getcwd(pBuf, 1024);
			path = pBuf;
			path += "\\";
			std::wstring wpath = std::wstring(path.begin(), path.end());

			std::string vertCompiledPath = path, fragCompiledPath = path;
			vertCompiledPath += "assets\\triangle.vert.dxbc";
			fragCompiledPath += "assets\\triangle.frag.dxbc";

#define COMPILESHADERS
#ifdef COMPILESHADERS
			std::wstring vertPath = wpath + L"assets\\triangle.vert.hlsl";
			std::wstring fragPath = wpath + L"assets\\triangle.frag.hlsl";

			try
			{
				ThrowIfFailed(D3DCompileFromFile(vertPath.c_str(), nullptr, nullptr, "main", "vs_5_0", compileFlags, 0, &vertexShader, &errors));
				ThrowIfFailed(D3DCompileFromFile(fragPath.c_str(), nullptr, nullptr, "main", "ps_5_0", compileFlags, 0, &pixelShader, &errors));
			}
			catch (std::exception e)
			{
				const char* errStr = (const char*)errors->GetBufferPointer();
				std::cout << errStr;
				errors->Release();
				errors = nullptr;
			}

			std::ofstream vsOut(vertCompiledPath, std::ios::out | std::ios::binary), fsOut(fragCompiledPath, std::ios::out | std::ios::binary);

			vsOut.write((const char*)vertexShader->GetBufferPointer(), vertexShader->GetBufferSize());
			fsOut.write((const char*)pixelShader->GetBufferPointer(), pixelShader->GetBufferSize());

#else
			std::vector<char> vsBytecodeData = readFile(vertCompiledPath);
			std::vector<char> fsBytecodeData = readFile(fragCompiledPath);

#endif
			// Define the vertex input layout.
			D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
			};

			// Create the UBO.
			{
				// Note: using upload heaps to transfer static data like vert
				// buffers is not recommended. Every time the GPU needs it, the
				// upload heap will be marshalled over. Please read up on Default
				// Heap usage. An upload heap is used here for code simplicity and
				// because there are very few verts to actually transfer.
				D3D12_HEAP_PROPERTIES heapProps;
				heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
				heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
				heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
				heapProps.CreationNodeMask = 1;
				heapProps.VisibleNodeMask = 1;

				D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
				heapDesc.NumDescriptors = 1;
				heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
				heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
				ThrowIfFailed(api->Device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&commandList->DescriptorHeap)));

				D3D12_RESOURCE_DESC uboResourceDesc;
				uboResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
				uboResourceDesc.Alignment = 0;
				uboResourceDesc.Width = (sizeof(s_Data.MvpMatrices) + 255) & ~255;
				uboResourceDesc.Height = 1;
				uboResourceDesc.DepthOrArraySize = 1;
				uboResourceDesc.MipLevels = 1;
				uboResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
				uboResourceDesc.SampleDesc.Count = 1;
				uboResourceDesc.SampleDesc.Quality = 0;
				uboResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
				uboResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

				ThrowIfFailed(api->Device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &uboResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&UniformBuffer)));
				commandList->DescriptorHeap->SetName(L"Constant Buffer Upload Resource Heap");

				D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
				cbvDesc.BufferLocation = UniformBuffer->GetGPUVirtualAddress();
				cbvDesc.SizeInBytes = (sizeof(s_Data.MvpMatrices) + 255) & ~255; // CB size is required to be 256-byte aligned.

				D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle(commandList->DescriptorHeap->GetCPUDescriptorHandleForHeapStart());
				cbvHandle.ptr = cbvHandle.ptr + api->Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 0;

				api->Device->CreateConstantBufferView(&cbvDesc, cbvHandle);

				// We do not intend to read from this resource on the CPU. (End is
				// less than or equal to begin)
				D3D12_RANGE readRange;
				readRange.Begin = 0;
				readRange.End = 0;

				ThrowIfFailed(UniformBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_MappedUniformBuffer)));
				memcpy(m_MappedUniformBuffer, &s_Data.MvpMatrices, sizeof(s_Data.MvpMatrices));
				UniformBuffer->Unmap(0, &readRange);
			}

			// Describe and create the graphics pipeline state object (PSO).
			D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
			psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
			psoDesc.pRootSignature = commandList->RootSignature;

			D3D12_SHADER_BYTECODE vsBytecode;
			D3D12_SHADER_BYTECODE psBytecode;

#ifdef COMPILESHADERS
			vsBytecode.pShaderBytecode = vertexShader->GetBufferPointer();
			vsBytecode.BytecodeLength = vertexShader->GetBufferSize();

			psBytecode.pShaderBytecode = pixelShader->GetBufferPointer();
			psBytecode.BytecodeLength = pixelShader->GetBufferSize();
#else
			vsBytecode.pShaderBytecode = vsBytecodeData.data();
			vsBytecode.BytecodeLength = vsBytecodeData.size();

			psBytecode.pShaderBytecode = fsBytecodeData.data();
			psBytecode.BytecodeLength = fsBytecodeData.size();
#endif

			psoDesc.VS = vsBytecode;
			psoDesc.PS = psBytecode;

			D3D12_RASTERIZER_DESC rasterDesc;
			rasterDesc.FillMode = D3D12_FILL_MODE_SOLID;
			rasterDesc.CullMode = D3D12_CULL_MODE_NONE; // Use D3D12_CULL_MODE_FRONT for clockwise culling, and D3D12_CULL_MODE_BACK for counter clock-wise!
			rasterDesc.FrontCounterClockwise = FALSE;
			rasterDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
			rasterDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
			rasterDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
			rasterDesc.DepthClipEnable = TRUE;
			rasterDesc.MultisampleEnable = FALSE;
			rasterDesc.AntialiasedLineEnable = FALSE;
			rasterDesc.ForcedSampleCount = 0;
			rasterDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

			psoDesc.RasterizerState = rasterDesc;

			D3D12_BLEND_DESC blendDesc;
			blendDesc.AlphaToCoverageEnable = FALSE;
			blendDesc.IndependentBlendEnable = FALSE;
			const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc = {
				FALSE,
				FALSE,
				D3D12_BLEND_ONE,
				D3D12_BLEND_ZERO,
				D3D12_BLEND_OP_ADD,
				D3D12_BLEND_ONE,
				D3D12_BLEND_ZERO,
				D3D12_BLEND_OP_ADD,
				D3D12_LOGIC_OP_NOOP,
				D3D12_COLOR_WRITE_ENABLE_ALL,
			};

			for (uint32_t i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
				blendDesc.RenderTarget[i] = defaultRenderTargetBlendDesc;

			psoDesc.BlendState = blendDesc;
			psoDesc.DepthStencilState.DepthEnable = FALSE;
			psoDesc.DepthStencilState.StencilEnable = FALSE;
			psoDesc.SampleMask = UINT_MAX;
			psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
			psoDesc.SampleDesc.Count = 1;

			try
			{
				ThrowIfFailed(api->Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&commandList->PipelineState)));
			}
			catch (std::exception e)
			{
				std::cout << "Failed to create Graphics Pipeline!";
			}

			if (vertexShader)
			{
				vertexShader->Release();
				vertexShader = nullptr;
			}

			if (pixelShader)
			{
				pixelShader->Release();
				pixelShader = nullptr;
			}
		}

		commandList->CreateCommands(api);

		// Command lists are created in the recording state, but there is nothing
		// to record yet. The main loop expects it to be closed, so close it now.
		ThrowIfFailed(commandList->CommandList->Close());

		// Create the vertex buffer.
		{
			const uint32_t vertexBufferSize = sizeof(s_Data.QuadVertexPositions);

			// Note: using upload heaps to transfer static data like vert buffers is
			// not recommended. Every time the GPU needs it, the upload heap will be
			// marshalled over. Please read up on Default Heap usage. An upload heap
			// is used here for code simplicity and because there are very few verts
			// to actually transfer.
			D3D12_HEAP_PROPERTIES heapProps;
			heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
			heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			heapProps.CreationNodeMask = 1;
			heapProps.VisibleNodeMask = 1;

			D3D12_RESOURCE_DESC vertexBufferResourceDesc;
			vertexBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			vertexBufferResourceDesc.Alignment = 0;
			vertexBufferResourceDesc.Width = vertexBufferSize;
			vertexBufferResourceDesc.Height = 1;
			vertexBufferResourceDesc.DepthOrArraySize = 1;
			vertexBufferResourceDesc.MipLevels = 1;
			vertexBufferResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
			vertexBufferResourceDesc.SampleDesc.Count = 1;
			vertexBufferResourceDesc.SampleDesc.Quality = 0;
			vertexBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			vertexBufferResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

			ThrowIfFailed(api->Device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &vertexBufferResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_VertexBuffer)));

			// Copy the triangle data to the vertex buffer.
			uint8_t* pVertexDataBegin;

			// We do not intend to read from this resource on the CPU.
			D3D12_RANGE readRange;
			readRange.Begin = 0;
			readRange.End = 0;

			ThrowIfFailed(m_VertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
			memcpy(pVertexDataBegin, s_Data.QuadVertexPositions, sizeof(s_Data.QuadVertexPositions));
			m_VertexBuffer->Unmap(0, nullptr);

			// Initialize the vertex buffer view.
			commandList->VertexBufferView.BufferLocation = m_VertexBuffer->GetGPUVirtualAddress();
			commandList->VertexBufferView.StrideInBytes = sizeof(QuadVertex);
			commandList->VertexBufferView.SizeInBytes = vertexBufferSize;
		}

		// Create the index buffer.
		{
			const uint32_t indexBufferSize = sizeof(s_Data.Indices);

			// Note: using upload heaps to transfer static data like vert buffers is
			// not recommended. Every time the GPU needs it, the upload heap will be
			// marshalled over. Please read up on Default Heap usage. An upload heap
			// is used here for code simplicity and because there are very few verts
			// to actually transfer.
			D3D12_HEAP_PROPERTIES heapProps;
			heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
			heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			heapProps.CreationNodeMask = 1;
			heapProps.VisibleNodeMask = 1;

			D3D12_RESOURCE_DESC vertexBufferResourceDesc;
			vertexBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			vertexBufferResourceDesc.Alignment = 0;
			vertexBufferResourceDesc.Width = indexBufferSize;
			vertexBufferResourceDesc.Height = 1;
			vertexBufferResourceDesc.DepthOrArraySize = 1;
			vertexBufferResourceDesc.MipLevels = 1;
			vertexBufferResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
			vertexBufferResourceDesc.SampleDesc.Count = 1;
			vertexBufferResourceDesc.SampleDesc.Quality = 0;
			vertexBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			vertexBufferResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

			ThrowIfFailed(api->Device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &vertexBufferResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_IndexBuffer)));

			// Copy the triangle data to the vertex buffer.
			uint8_t* pVertexDataBegin;

			// We do not intend to read from this resource on the CPU.
			D3D12_RANGE readRange;
			readRange.Begin = 0;
			readRange.End = 0;

			ThrowIfFailed(m_IndexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
			memcpy(pVertexDataBegin, s_Data.Indices, sizeof(s_Data.Indices));
			m_IndexBuffer->Unmap(0, nullptr);

			// Initialize the vertex buffer view.
			commandList->IndexBufferView.BufferLocation = m_IndexBuffer->GetGPUVirtualAddress();
			commandList->IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
			commandList->IndexBufferView.SizeInBytes = indexBufferSize;
		}

		// Create synchronization objects and wait until assets have been uploaded
		// to the GPU.
		{
			commandList->FenceValue = 1;

			// Create an event handle to use for frame synchronization.
			commandList->FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

			if (commandList->FenceEvent == nullptr)
				ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));

			// Wait for the command list to execute; we are reusing the same command
			// list in our main loop but for now, we just want to wait for setup to
			// complete before continuing.
			// Signal and increment the fence value.
			const uint64_t fence = commandList->FenceValue;
			ThrowIfFailed(api->CommandQueue->Signal(api->Fence, fence));
			commandList->FenceValue++;

			// Wait until the previous frame is finished.
			if (api->Fence->GetCompletedValue() < fence)
			{
				ThrowIfFailed(api->Fence->SetEventOnCompletion(fence, commandList->FenceEvent));
				WaitForSingleObject(commandList->FenceEvent, INFINITE);
			}

			commandList->FrameIndex = Swapchain->GetCurrentBackBufferIndex();
		}
	}

	void D3D12Renderer::DestroyResources(D3D12CommandList* commandList)
	{
		// Sync
		CloseHandle(commandList->FenceEvent);

		if (commandList->PipelineState)
		{
			commandList->PipelineState->Release();
			commandList->PipelineState = nullptr;
		}

		if (commandList->RootSignature)
		{
			commandList->RootSignature->Release();
			commandList->RootSignature = nullptr;
		}

		if (m_VertexBuffer)
		{
			m_VertexBuffer->Release();
			m_VertexBuffer = nullptr;
		}

		if (m_IndexBuffer)
		{
			m_IndexBuffer->Release();
			m_IndexBuffer = nullptr;
		}

		if (UniformBuffer)
		{
			UniformBuffer->Release();
			UniformBuffer = nullptr;
		}

		if (commandList->DescriptorHeap)
		{
			commandList->DescriptorHeap->Release();
			commandList->DescriptorHeap = nullptr;
		}
	}

	void D3D12Renderer::Render(D3D12API* api, D3D12CommandList* commandList)
	{
		// Frame limit set to 60 fps
		EndTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::milli>(EndTime - StartTime).count();

		//if (time < (1000.0f / 60.0f))
		//    return;

		StartTime = std::chrono::high_resolution_clock::now();

		{
			// Update Uniforms
			s_Data.MvpMatrices.Model = glm::rotate(s_Data.MvpMatrices.Model, 0.001f * time, glm::vec3(0.0f, 1.0f, 0.0f));

			D3D12_RANGE readRange;
			readRange.Begin = 0;
			readRange.End = 0;

			ThrowIfFailed(UniformBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_MappedUniformBuffer)));
			memcpy(m_MappedUniformBuffer, &s_Data.MvpMatrices, sizeof(s_Data.MvpMatrices));
			UniformBuffer->Unmap(0, &readRange);
		}

		// Record all the commands we need to render the scene into the command list.
		commandList->SetupCommands(api);

		// Execute the command list.
		ID3D12CommandList* ppCommandLists[] = { commandList->CommandList};
		api->CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
		Swapchain->Present(1, 0);

		// WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.

		// Signal and increment the fence value.
		const uint64_t fence = commandList->FenceValue;
		ThrowIfFailed(api->CommandQueue->Signal(api->Fence, fence));
		commandList->FenceValue++;

		// Wait until the previous frame is finished.
		if (api->Fence->GetCompletedValue() < fence)
		{
			ThrowIfFailed(api->Fence->SetEventOnCompletion(fence, commandList->FenceEvent));
			WaitForSingleObject(commandList->FenceEvent, INFINITE);
		}

		commandList->FrameIndex = Swapchain->GetCurrentBackBufferIndex();
	}

	void D3D12Renderer::UpdateMVPMatrices(int32_t width, int32_t height)
	{
		float zoom = 2.5f;

		s_Data.MvpMatrices.Projection = glm::perspective(45.0f, (float)width / (float)height, 0.01f, 1024.0f);
		s_Data.MvpMatrices.View = glm::translate(glm::identity<glm::mat4>(), glm::vec3(0.0f, 0.0f, zoom));
		s_Data.MvpMatrices.Model = glm::identity<glm::mat4>();
	}

	void D3D12Renderer::ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
			throw std::exception();
	}

}