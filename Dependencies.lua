-- Nutcrackz D3D12 Dependencies

IncludeDir = {}
IncludeDir["ImGui"] = "%{wks.location}/Nutcrackz/vendor/imgui"
IncludeDir["glm"] = "%{wks.location}/Nutcrackz/vendor/glm"

Library = {}

-- Windows
Library["WinSock"] = "Ws2_32.lib"
Library["WinMM"] = "Winmm.lib"
Library["WinVersion"] = "Version.lib"
Library["BCrypt"] = "Bcrypt.lib"
Library["Dbghelp"] = "Dbghelp.lib"