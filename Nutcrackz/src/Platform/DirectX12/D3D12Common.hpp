#pragma once

#include "d3d12.h"
#include <D3Dcompiler.h>
#include <dxgi1_4.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#ifdef NZ_DEBUG
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif

#include "rtmcpp/Common.hpp"
#include "rtmcpp/Transforms.hpp"

//#define GLM_FORCE_LEFT_HANDED
//#include <glm/glm.hpp>
//#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>

#include <direct.h>

namespace Nutcrackz {

	static const uint32_t s_BackbufferCount = 2;

	// Uniform data
	struct MVPMatrices
	{
		rtmcpp::Mat4 Projection;
		rtmcpp::Mat4 Model;
		rtmcpp::Mat4 View;
	};

}