require("Premake/Common")

require("Premake/Libs/glfw")
require("Premake/Libs/glm")
require("Premake/Libs/imgui")
require("Premake/Libs/spdlog")
require("Premake/Libs/stb")
require("Premake/Libs/vma")
require("Premake/Libs/vulkan")
-- require("Premake/Libs/inipp")

workspace("ProRGB")
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
	project("Server")
		location("Libraries/Server/")
		warnings("Extra")

		kind("StaticLib")
		common:outDirs(true)

		common:addPCH("%{prj.location}/Source/PCH.cpp", "%{prj.location/Source/PCH.h")

        includedirs({
            "%{prj.location}/Include/",
            "%{prj.location}/Source/"
        })

		files({
			"%{prj.location}/Include/**",
			"%{prj.location}/Source/**"
		})
        removefiles({ "*.DS_Store" })

        common:addActions()

	project("Client")
		location("Libraries/Client/")
		warnings("Extra")

		kind("StaticLib")
		common:outDirs(true)

		common:addPCH("%{prj.location}/Source/PCH.cpp", "%{prj.location/Source/PCH.h")

		includedirs({
			"%{prj.location}/Include/",
			"%{prj.location}/Source/"
		})

        files({
            "%{prj.location}/Include/**",
            "%{prj.location}/Source/**"
        })
        removefiles({ "*.DS_Store" })

        common:addActions()

	project("Controller")
		location("Libraries/Controller/")
		warnings("Extra")

		kind("StaticLib")
		common:outDirs(true)

		common:addPCH("%{prj.location}/Source/PCH.cpp", "%{prj.location/Source/PCH.h")

        includedirs({
            "%{prj.location}/Include/",
            "%{prj.location}/Source/"
        })

		files({
            "%{prj.location}/Include/**",
            "%{prj.location}/Source/**"
        })
        removefiles({ "*.DS_Store" })

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

		common:addPCH("%{prj.location}/Source/PCH.cpp", "%{prj.location}/Source/PCH.h")

		includedirs({ "%{prj.location}/Source/" })

		if common.host == "windows" then
			linkoptions({ "/IGNORE:4099" })
		elseif common.host == "macosx" then
			linkoptions({ "-Wl,-rpath,'@executable_path'" })
		end

		libs.glfw:setupDep()
		libs.vma:setupDep()
		libs.imgui:setupDep()
		libs.stb:setupDep()
		libs.spdlog:setupDep()
		libs.vulkan:setupDep(false)
		libs.glm:setupDep()

		links({ "Controller" })
		sysincludedirs({ "%{wks.location}/Libraries/Controller/Include/" })

		files({ "%{prj.location}/Source/**" })
		removefiles({ "*.DS_Store" })

		common:addActions()
