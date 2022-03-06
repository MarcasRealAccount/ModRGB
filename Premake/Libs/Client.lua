if not libs then libs = {} end
if not libs.Client then
	libs.Client = {
		name     = "",
		location = ""
	}
end

require("Common")

local Client = libs.Client

function Client:setup()
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

function Client:setupDep()
	links({ self.name })
	sysincludedirs({ self.location .. "/Inc/" })
	libs.Common:setupDep()
end