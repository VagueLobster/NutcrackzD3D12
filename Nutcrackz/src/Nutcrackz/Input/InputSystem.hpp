#pragma once

#include "Nutcrackz/Core/Unique.hpp"

#include "InputAction.hpp"
#include "InputContext.hpp"
#include "ExternalInputChannel.hpp"

namespace Nutcrackz {

	struct InputSystem : Handle<InputSystem>
	{
		InputAction RegisterAction(const InputActionData& actionData);
		InputContext CreateContext();
	};

}