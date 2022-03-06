if not libs then libs = {} end
if not libs.Server then
	libs.Server = {
		name     = "",
		location = ""
	}
end

require("Common")

local Server = libs.Server

function Server:setup()
	self.name     = common:projectName()
	self.location = common:projectLocation()

	kind("StaticLib")
	common:outDirs(true)

	common:addPCH("%{prj.location}/Src/PCH.cpp", "%{prj.location/Src/PCH.h")

	includedirs({
		"%{prj.location}/Inc/",
		"%{prj.location}/Src/"
	})

	libs.Common:setupDep()

	files({
		"%{prj.location}/Inc/**",
		"%{prj.location}/Src/**"
	})
	removefiles({ "*.DS_Store" })
end

function Server:setupDep()
	links({ self.name })
	sysincludedirs({ self.location .. "/Inc/" })
	libs.Common:setupDep()
end