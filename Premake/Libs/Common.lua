if not libs then libs = {} end
if not libs.Common then
	libs.Common = {
		name     = "",
		location = ""
	}
end

local Common = libs.Common

function Common:setup()
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
end

function Common:setupDep()
	links({ self.name })
	sysincludedirs({ self.location .. "/Inc/" })
end