local GRDK = os.getenv("GRDKLatest");

project "Nutcrackz"
	kind "StaticLib"
	language "C++"
	cppdialect "C++latest"
	staticruntime "off"
	
	warnings "Extra"
	
	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "nzpch.hpp"
	pchsource "src/nzpch.cpp"

	files
	{
		"src/nzpch.cpp",
        "src/nzpch.hpp",
	
		"src/**.hpp",
		"src/**.cpp"
	}

	includedirs
	{
		"src",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.glm}",
	}
	
	links
	{
		"ImGui",
	}

	defines
	{
		"_CRT_SECURE_NO_WARNINGS",
	}
	
	externalincludedirs
	{
		"src/"
	}
	
	forceincludes
	{
		"nzpch.hpp"
	}
	
	filter { "system:windows" }
        files
		{
            "src/Platform/Windows/**.cpp",
            "src/Platform/Windows/**.hpp",
        }

		externalincludedirs
		{
			GRDK .. "/GameKit/Include/",
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
		defines "NZ_DIST"
		runtime "Release"
		optimize "on"
		inlining ("Auto")