# hashlink-exts
Backup of internal hashlink modules. Might be useful if you're brave.

# Documentation
Currently everything in here is undocumented except for the comments in the haxe files.

# Licenses
All contained module code uses the license of the library the module wraps around. At the time of writing, that means everything here is MIT licensed, but please check the module source license if you are unsure.

# Compiling
All modules depend on https://github.com/HaxeFoundation/hashlink and Haxe 4.2 (may work on 4.1, but don't bet on it). Build scripts for this repo have not yet been made available, but you might want to use `LUA_USE_POSIX` for `exlua` on Linux and `PHYSFS_SUPPORTS_DEFAULT=0` `PHYSFS_SUPPORTS_ZIP=1` for `exphysfs`.
