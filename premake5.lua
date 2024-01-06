include "Dependencies.lua"

workspace "Nutcrackz"
	conformancemode "On"
	--architecture "x86_64"
	startproject "Sandbox"

	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

	externalanglebrackets "On"
	externalwarnings "Off"
	warnings "Off"

	flags
	{
		"MultiProcessorCompile"
	}
	
	filter "language:C++ or language:C"
		architecture "x86_64"
		
	filter "system:windows"
		defines { "_CRT_SECURE_NO_WARNINGS" }

		disablewarnings {
            "4100", -- Unreferenced Formal Parameter
			"4201" -- Anonymous Struct
        }

		buildoptions {
			"/openmp:llvm"
		}

	filter "toolset:clang"
		disablewarnings {
			"unused-parameter",
		}

	filter "toolset:gcc"
		disablewarnings {
			"unused-parameter",
			"missing-field-initializers",
		}

    filter "action:vs*"
        linkoptions { "/ignore:4099" }
		buildoptions {
			"/Zc:preprocessor"
		}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

group "Dependencies"
	include "Nutcrackz/vendor/imgui"
group ""

group "Core"
	include "Nutcrackz"
group ""

group "Tools"
	include "Sandbox"
group ""
