if not libs then libs = {} end
if not libs.ModRGB then
	libs.ModRGB = {
		name     = "",
		location = ""
	}
end

require("ReliableUDP")

local ModRGB = libs.ModRGB

function ModRGB:setup()
	self.name     = common:projectName()
	self.location = common:projectLocation()

	kind("StaticLib")
	common:outDirs(true)

	common:addPCH("%{prj.location}/Src/PCH.cpp", "%{prj.location/Src/PCH.h")

	includedirs({
		"%{prj.location}/Inc/",
		"%{prj.location}/Src/"
	})

	files({
		"%{prj.location}/Inc/**",
		"%{prj.location}/Src/**"
	})
	removefiles({ "*.DS_Store" })
	
	libs.ReliableUDP:setupDep()
end

function ModRGB:setupDep()
	links({ self.name })
	sysincludedirs({ self.location .. "/Inc/" })

	libs.ReliableUDP:setupDep()
end