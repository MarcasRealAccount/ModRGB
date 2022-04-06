require("Premake/Common")

require("Premake/Libs/ModRGB")

require("Premake/ThirdParty/glfw")
require("Premake/ThirdParty/glm")
require("Premake/ThirdParty/imgui")
require("Premake/ThirdParty/spdlog")
require("Premake/ThirdParty/stb")
require("Premake/ThirdParty/vma")
require("Premake/ThirdParty/vulkan")

workspace("ModRGB")
	common:setConfigsAndPlatforms()

	common:addCoreDefines()

	cppdialect("C++20")
	rtti("Off")
	exceptionhandling("On")
	flags("MultiProcessorCompile")

	startproject("ControllerApp")

	group("Dependencies")
	project("GLFW")
		location("ThirdParty/GLFW/")
		warnings("Off")
		libs.glfw:setup()
		location("ThirdParty/")

	project("VMA")
		location("ThirdParty/VMA/")
		warnings("Off")
		libs.vma:setup()
		location("ThirdParty/")

	project("ImGUI")
		location("ThirdParty/ImGUI/")
		warnings("Off")
		libs.imgui:setup()
		location("ThirdParty/")

	project("STB")
		location("ThirdParty/STB/")
		warnings("Off")
		libs.stb:setup()
		location("ThirdParty/")

	project("Spdlog")
		location("ThirdParty/spdlog/")
		warnings("Off")
		libs.spdlog:setup()
		location("ThirdParty/")

	project("VulkanHeaders")
		location("")
		warnings("Off")
		libs.vulkan:setup()
		location("ThirdParty/")

	project("glm")
		location("ThirdParty/glm/")
		warnings("Off")
		libs.glm:setup()
		location("ThirdParty/")

	group("Libraries")
	project("ModRGB")
	    location("Library")
	    warnings("Extra")
		libs.ModRGB:setup()
        common:addActions()

	group("Apps")
	project("ControllerApp")
		location("Apps/ControllerApp/")
		warnings("Extra")

		common:outDirs()
		common:debugDir()

		filter("configurations:Debug")
			kind("ConsoleApp")

		filter("configurations:not Debug")
			kind("WindowedApp")

		filter({})

		common:addPCH("%{prj.location}/Src/PCH.cpp", "%{prj.location}/Src/PCH.h")

		includedirs({ "%{prj.location}/Src/" })

		if common.host == "windows" then
			linkoptions({ "/IGNORE:4099" })
		elseif common.host == "macosx" then
			linkoptions({ "-Wl,-rpath,'@executable_path'" })
		end

		libs.ModRGB:setupDep()
		libs.glfw:setupDep()
		libs.vma:setupDep()
		libs.imgui:setupDep()
		libs.stb:setupDep()
		libs.spdlog:setupDep()
		libs.vulkan:setupDep(false)
		libs.glm:setupDep()

		files({ "%{prj.location}/Src/**" })
		removefiles({ "*.DS_Store" })

		common:addActions()

	group("Tests")
	project("Tests")
		location("Tests/")
		warnings("Extra")

		common:outDirs()
		common:debugDir()

		kind("ConsoleApp")

		libs.ModRGB:setupDep()

		files({ "%{prj.location}/Src/**" })
		removefiles({ "*.DS_Store" })

		common:addActions()