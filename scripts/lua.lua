return function(base)
	language "C++"
	kind "StaticLib"

	local lua_src = path.join(base, path.join("exlua", path.join("lua-5.4.1", "src")))
	files { path.join(lua_src, "*.c") }
	excludes {
		path.join(lua_src, "lua.c"),
		path.join(lua_src, "luac.c")
	}

	configuration "linux" do
		defines { "LUA_USE_POSIX" }
		configuration {}
	end
end
