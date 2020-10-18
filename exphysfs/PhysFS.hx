package exphysfs;

private typedef PhysFS_File_Internal = hl.Abstract<"exphysfs_file">;

@:hlNative("exphysfs")
abstract PhysFS_File({size: Int, handle: PhysFS_File_Internal}) {
	@:native("open") static function _open(path: String, mode: OpenMode): PhysFS_File_Internal { return null; }
	public static inline function open(path: String, mode: OpenMode) {
		return new PhysFS_File(_open(path, mode));
	}

	@:native("get_length") private static function _get_length(handle: PhysFS_File_Internal): hl.I64 { return 0; }
	private inline function get_length() { return _get_length(this.handle); }

	/** if this is all you want to know then don't bother opening the file, just use stat! **/
	public var length(get, never): hl.I64;
	
	@:native("eof") static function _eof(handle: PhysFS_File_Internal): Bool { return true; }
	private inline function get_eof() {
		return _eof(this.handle);
	}
	public var eof(get, never): Bool;

	inline function new(handle: PhysFS_File_Internal) {
		this = {
			handle: handle,
			size: _get_length(handle)
		}
	}

	@:native("close") static function _close(handle: PhysFS_File_Internal): Void {}

	/** you can close files yourself, but they do get cleaned up by GC eventually regardless. **/
	public inline function close() {
		_close(this.handle);
	}

	@:native("seek") static function _seek(handle: PhysFS_File_Internal, pos: hl.I64): Void {}
	public inline function seek(pos: hl.I64) {
		_seek(this.handle, pos);
	}

	@:native("tell") static function _tell(handle: PhysFS_File_Internal): hl.I64 { return 0; }
	public inline function tell() {
		return _tell(this.handle);
	}

	@:native("read_all")
	static function _read_all(handle: PhysFS_File_Internal): hl.Bytes { return null; }

	/** reads everything then seeks back to 0 **/
	public inline function read_all() {
		return _read_all(this.handle).toBytes(this.size);
	}
}

@:enum abstract OpenMode(Int) {
	final Read = 0;
	final Write = 1;
	final Append = 2;
}

@:enum abstract FileType(Int) {
	final Regular = 0;
	final Directory = 1;
	final Symlink = 2;
	/** something completely different like a device **/
	final Other = 3;
}

typedef Stat = {
	read_only: Bool,
	file_type: Int,
	// these should all be I64, but HL/C is bugged with I64 + Dynamic
	file_size: Float,
	create_time: Float,
	access_time: Float,
	mod_time: Float,
}

@:hlNative("exphysfs")
abstract PhysFS(Void) {
	@:native("init") static function _init(self: String, org: String, app: String): Void {}

	public static inline function init(org: String, app: String) {
		_init(Sys.programPath(), org, app);
	}

	public static function mount(real_path: String, vfs_path: String, append: Bool): Void {}

	@:native("stat") static function _stat(filepath: String): Dynamic { return null; }
	public static inline function stat(filepath: String): Null<Stat> {
		final s = _stat(filepath);
		if (s != null) {
			return {
				read_only: s.read_only,
				file_type: s.file_type,
				file_size: s.file_size,
				create_time: s.create_time,
				access_time: s.access_time,
				mod_time: s.mod_time,
			}
		}
		return null;
	}

	@:native("get_directory_items") static function _get_directory_items(path: String) : hl.NativeArray<hl.Bytes> { return null; }

	public static inline function get_directory_items(path: String): Array<String> {
		var arr = _get_directory_items(path);
		return [ for (v in arr) @:privateAccess String.fromUCS2(v) ];
	}

	// this approach doesn't work on hl/jit (fine on hl/c)
	// @:native("get_directory_items") static function _get_directory_items(path: String, cb: (filename: hl.Bytes)->Void): Void {}

	@:native("get_base_dir") static function _get_base_dir(): hl.Bytes { return null; };
	public static inline function get_base_dir() return @:privateAccess String.fromUCS2(_get_base_dir());

	@:native("get_write_dir") static function _get_write_dir(): hl.Bytes { return null; };
	public static inline function get_write_dir() return @:privateAccess String.fromUCS2(_get_write_dir());

	@:native("get_real_dir") static function _get_real_dir(filepath: String): hl.Bytes { return null; };
	public static inline function get_real_dir(filepath: String) return @:privateAccess String.fromUCS2(_get_real_dir(filepath));

	public static inline function open(path: String, mode: OpenMode) {
		return PhysFS_File.open(path, mode);
	}
}
