require("Premake/Common")

require("Premake/Libs/ReliableUDP")
require("Premake/Libs/ModRGB")

workspace("ModRGB")
	common:setConfigsAndPlatforms()

	common:addCoreDefines()

	cppdialect("C++20")
	rtti("Off")
	exceptionhandling("On")
	flags("MultiProcessorCompile")

	startproject("Tests")

	group("Libraries")
	project("ReliableUDP")
	    location("Libraries/ReliableUDP/")
	    warnings("Extra")
		libs.ReliableUDP:setup()
        common:addActions()
	
	project("ModRGB")
	    location("Libraries/ModRGB/")
	    warnings("Extra")
		libs.ModRGB:setup()
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