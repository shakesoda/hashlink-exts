return function(base)
	-- don't bother building the jit, doesn't work on arm64
	language "C++"
	kind "StaticLib"

	local hl = path.join(base, "hashlink")
	local src = path.join(hl, "src")
	local std = path.join(src, "std")
	includedirs {
		src,
		path.join(path.join(hl, "include"), "pcre")
	}
	files {
		path.join(src, "gc.c"),
		path.join(std, "array.c"),
		path.join(std, "buffer.c"),
		path.join(std, "bytes.c"),
		path.join(std, "cast.c"),
		path.join(std, "date.c"),
		path.join(std, "debug.c"),
		path.join(std, "error.c"),
		path.join(std, "file.c"),
		path.join(std, "fun.c"),
		path.join(std, "maps.c"),
		path.join(std, "math.c"),
		path.join(std, "obj.c"),
		path.join(std, "process.c"),
		path.join(std, "random.c"),
		path.join(std, "regexp.c"),
		path.join(std, "socket.c"),
		path.join(std, "string.c"),
		path.join(std, "sys.c"),
		path.join(std, "thread.c"),
		path.join(std, "track.c"),
		path.join(std, "types.c"),
		path.join(std, "ucs2.c"),
	}

	do
		-- internal pcre
		local src = path.join(hl, "include")
		includedirs {
			src
		}
		local prefix = path.join(path.join(src, "pcre"), "pcre")
		files {
			prefix .. "_chartables.c",
			prefix .. "_compile.c",
			prefix .. "_dfa_exec.c",
			prefix .. "_exec.c",
			prefix .. "_fullinfo.c",
			prefix .. "_globals.c",
			prefix .. "_newline.c",
			prefix .. "_string_utils.c",
			prefix .. "_tables.c",
			prefix .. "_ucd.c",
			prefix .. "_xclass.c",
			prefix .. "16_ord2utf16.c",
			prefix .. "16_valid_utf16.c",
		}
	end

	defines { "LIBHL_EXPORTS=1" }
	flags { "Optimize" }
end
