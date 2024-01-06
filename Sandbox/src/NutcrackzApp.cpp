#include <Nutcrackz/Core/Application.hpp>
#include <Nutcrackz/Core/Window.hpp>
#include <Nutcrackz/Input/InputSystem.hpp>
#include <Nutcrackz/Input/InputAdapter.hpp>
#include <Nutcrackz/Input/InputAction.hpp>
#include <Nutcrackz/Input/InputCodes.hpp>
#include <Nutcrackz/Renderer/Renderer.hpp>
#include "Nutcrackz/UI/ImguiWindow.hpp"

#include <iostream>
#include <ranges>

#include <imgui.h>

using namespace Nutcrackz;

class NutcrackzApp final : public Application
{
protected:
	void OnLaunch() override
	{
		m_Window = m_WindowSystem->NewWindow("Input Testing");
		m_Window->D3D12Renderer = new Renderer(m_Window);

		//m_ImGuiWindow = new ImGuiWindow();
		//if (m_ImGuiWindow->Init(m_Window))
		//	m_InitializedImGui = true;

		m_Context = m_InputSystem.CreateContext();
		m_OtherContext = m_InputSystem.CreateContext();

		auto walkAction = m_InputSystem.RegisterAction({
			.AxisCount = 2,
			.AxisBindings = {
				{
					.Bindings = {
						{ { GenericKeyboard, KeyCode::D },  1.0f},
						{ { GenericKeyboard, KeyCode::A }, -1.0f},
					}
				},
				{
					.Bindings = {
						{ { GenericKeyboard, KeyCode::W },  1.0f},
						{ { GenericKeyboard, KeyCode::S }, -1.0f},
					}
				}
			},
			.ConsumeInputs = true
		});

		m_Context.BindAction(walkAction, [&](const InputReading& reading)
		{
			auto [x, y] = reading.Read<2>();
			//std::cout << "X: " << x << ", Y: " << y << "\n";
			std::cout << "Main Context\n";
		});

		m_OtherContext.BindAction(walkAction, [](const InputReading& reading)
		{
			auto [x, y] = reading.Read<2>();
			std::cout << "Other Context\n";
		});

		auto mouseAction = m_InputSystem.RegisterAction({
			.AxisCount = 1,
			.AxisBindings = {
				{
					.Bindings = {
						{ { GenericMouse, MouseCode::WheelScrollY },   1.0f },
					}
				}
			},
			.ConsumeInputs = true
		});

		m_Context.BindAction(mouseAction, [&](const InputReading& reading)
		{
			auto [x] = reading.Read<1>();
			std::cout << "Scroll Delta: " << x << "\n";
		});

		m_Context.Activate();
		m_OtherContext.Activate();
	}

	void OnUpdate() override
	{
		m_WindowSystem->Render(m_Window);

		if (m_InitializedImGui)
		{
			m_ImGuiWindow->Begin();
			{
				if (m_ShowDemoWindow)
					ImGui::ShowDemoWindow(&m_ShowDemoWindow);

				//if (ImGui::Begin("Test"))
				//{
				//	ImGui::Text("Fuck the world!");
				//
				//	ImGui::End();
				//}
			}
			m_ImGuiWindow->End(m_Window, m_Window->m_WindowSpec.Width, m_Window->m_WindowSpec.Height);
		}

		if (m_Window->IsClosed())
		{
			if (m_InitializedImGui)
				m_ImGuiWindow->Shutdown();

			m_Running = false;
		}
	}

private:
	Window* m_Window;
	InputContext m_Context, m_OtherContext;
	ImGuiWindow* m_ImGuiWindow = nullptr;

	bool m_ShowDemoWindow = true;
	bool m_InitializedImGui = false;
};

int main()
{
	AppRunner<NutcrackzApp>().Run();
	return 0;
}
