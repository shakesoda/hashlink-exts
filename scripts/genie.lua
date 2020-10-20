solution "hashlink-ext"
configurations { "Native" }

local base = path.getabsolute("..")
targetdir(path.join(base, "bin"))
objdir(path.join(base, "tmp"))

project "lua" do
	local lua = require "lua"
	lua(base)
end

project "hl" do
	local hl = require "hashlink"
	hl(base)
end

project "test" do
	language "C++"
	kind "ConsoleApp"

	local defs = { "HL_MAKE=1", "LIBHL_STATIC=1" }
	defines(defs)

	local bin = path.join(base, "bin")
	local src = path.join(bin, "c")
	local hl = path.join(base, "hashlink")
	includedirs {
		src,
		path.join(hl, "src"),
	}
	local hlc = assert(io.open(path.join(src, "hlc.json")), "Unable to open hlc.json, build the hl first?")
	local encoded = hlc:read("*a")
	local json = require "dkjson"
	local data = json.decode(encoded)
	for i, v in ipairs(data.files) do
		data.files[i] = path.join(src, v)
	end
	files(data.files)

	do
		local exlua = path.join(base, "exlua")
		links "lua"
		files {
			path.join(path.join(exlua, "src"), "hl_lua.c"),
			path.join(path.join(exlua, "lua-5.4.1"), "src"),
		}
		includedirs {
			path.join(path.join(exlua, "lua-5.4.1"), "src"),
			path.join(hl, "src")
		}
	end

	flags {
		"OptimizeSpeed"
	}

	links "hl"
	configuration "linux" do
		links "pthread"
		configuration {}
	end
end
