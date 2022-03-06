if not libs then libs = {} end
if not libs.Controller then
	libs.Controller = {
		name     = "",
		location = ""
	}
end

require("Common")

local Controller = libs.Controller

function Controller:setup()
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

function Controller:setupDep()
	links({ self.name })
	sysincludedirs({ self.location .. "/Inc/" })
	libs.Common:setupDep()
end