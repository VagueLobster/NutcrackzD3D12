#include "Application.hpp"
#include "Window.hpp"

#include "Nutcrackz/Input/InputSystemImpl.hpp"

namespace Nutcrackz {

	Application::Application()
	{
		m_WindowSystem = Unique<WindowSystem>::New();

		m_InputSystem = { new InputSystem::Impl() };
		m_InputSystem->Init();
	}

	void Application::Run()
	{
		OnLaunch();

		while (m_Running)
		{
			m_WindowSystem->PollEvents();
			m_InputSystem->Update();

			OnUpdate();
		}

		m_InputSystem->Shutdown();
	}

}