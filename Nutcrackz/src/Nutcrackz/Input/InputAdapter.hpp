#pragma once

#include "InputDevice.hpp"

namespace Nutcrackz {

	struct InputAdapter : public Handle<InputAdapter>
	{
		static InputAdapter Create();
		void Destroy();

		void Update() const;

		const InputDevice GetDevice(uint32_t deviceIndex) const;
	};

}