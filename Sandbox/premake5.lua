local GRDK = os.getenv("GRDKLatest");

project "Sandbox"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++latest"
	staticruntime "off"

	warnings "Extra"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"src/**.cpp",
		"src/**.hpp",
	}
	
	includedirs
	{
		"src",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.rtm}",
		"%{IncludeDir.rtmcpp}",
	}

	externalincludedirs
	{
		"../Nutcrackz/src/"
	}

	links
	{
		"Nutcrackz",
	}

	defines
	{
		"_CRT_SECURE_NO_WARNINGS",
		"RTMCPP_EXPORT="
	}

	filter { "system:windows" }
		libdirs
		{
			GRDK .. "/GameKit/Lib/amd64/",
			"../ThirdParty/DXC/lib/x64/"
		}

		links
		{
			"GameInput",
			"xgameruntime",
            "D3d12",
            "DXGI",
			"dxcompiler"
		}
		
	filter "configurations:Debug"
		defines "NZ_DEBUG"
		runtime "Debug"
		symbols "on"
		inlining ("Auto")
		editandcontinue "Off"

	filter "configurations:Release"
		defines "NZ_RELEASE"
		runtime "Release"
		optimize "on"
		inlining ("Auto")

	filter "configurations:Dist"
		kind "WindowedApp"
		defines "NZ_DIST"
		runtime "Release"
		optimize "on"
		inlining ("Auto")
