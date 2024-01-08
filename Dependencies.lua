-- Nutcrackz D3D12 Dependencies

IncludeDir = {}
IncludeDir["ImGui"] = "%{wks.location}/Nutcrackz/vendor/imgui"
IncludeDir["rtm"] = "%{wks.location}/Nutcrackz/vendor/rtmcpp/rtm/includes"
IncludeDir["rtmcpp"] = "%{wks.location}/Nutcrackz/vendor/rtmcpp/Include"

Library = {}

-- Windows
Library["WinSock"] = "Ws2_32.lib"
Library["WinMM"] = "Winmm.lib"
Library["WinVersion"] = "Version.lib"
Library["BCrypt"] = "Bcrypt.lib"
Library["Dbghelp"] = "Dbghelp.lib"