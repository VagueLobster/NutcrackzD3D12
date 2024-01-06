#pragma once

#include "Nutcrackz/Core/Core.hpp"

#include "InputAxis.hpp"

namespace Nutcrackz {

	struct ExternalInputChannel
	{
		float32_t Value;
		float32_t PreviousValue;

		bool HasRegisteredInput() const { return Value != 0.0f || PreviousValue != 0.0f; }
	};

}