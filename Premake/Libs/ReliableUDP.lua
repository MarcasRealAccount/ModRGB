if not libs then libs = {} end
if not libs.ReliableUDP then
	libs.ReliableUDP = {
		name     = "",
		location = ""
	}
end

local ReliableUDP = libs.ReliableUDP

function ReliableUDP:setup()
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
	
	if common.target == "windows" then
		links({ "Ws2_32" })
	end
end

function ReliableUDP:setupDep()
	links({ self.name })
	sysincludedirs({ self.location .. "/Inc/" })
end