#define HL_NAME(n) exphysfs_##n

#include <hl.h>
#include <physfs.h>

static void spew_error() {
	PHYSFS_ErrorCode err = PHYSFS_getLastErrorCode();
	const char *msg = PHYSFS_getErrorByCode(err);
	hl_throw(hl_alloc_strbytes(hl_to_utf16(msg)));
}

HL_PRIM void HL_NAME(init)(vstring *argv0, vstring *org, vstring *app) {
	if (PHYSFS_isInit()) {
		return;
	}
	// i'm not sure this failure can happen on windows, but it's handled.
	if (!PHYSFS_init(argv0 ? hl_to_utf8(argv0->bytes) : NULL)) {
		spew_error();
		return;
	}

	if (!PHYSFS_setSaneConfig(hl_to_utf8(org->bytes), hl_to_utf8(app->bytes), NULL, 0, 1)) {
		spew_error();
		return;
	}
}

HL_PRIM void HL_NAME(mount)(vstring *real_path, vstring *vfs_path, bool append) {
	if (!PHYSFS_mount(hl_to_utf8(real_path->bytes), hl_to_utf8(vfs_path->bytes), append ? 1 : 0)) {
		spew_error();
		return;
	}
}

typedef struct _exphysfs_file exphysfs_file;
struct _exphysfs_file {
	void (*finalize)(exphysfs_file *);
	PHYSFS_File *handle;
	int mode;
};

HL_PRIM void HL_NAME(close)(exphysfs_file *file) {
	if (file->handle != NULL) {
		if (!PHYSFS_close(file->handle)) {
			spew_error();
			return;
		}
		file->handle = NULL;
		file->finalize = NULL;
	}
}

HL_PRIM int64 HL_NAME(get_length)(exphysfs_file *file) {
	if (file->handle != NULL) {
		PHYSFS_sint64 len = PHYSFS_fileLength(file->handle);
		if (len < 0) {
			spew_error();
			return 0;
		}
		return (int64)len;
	}
	return 0;
}

HL_PRIM exphysfs_file *HL_NAME(open)(vstring *path, int mode) {
	exphysfs_file *f = (exphysfs_file *)hl_gc_alloc_finalizer(sizeof(exphysfs_file));
	const char *path_utf8 = hl_to_utf8(path->bytes);
	f->mode = mode;
	switch (mode) {
	case 0: { // read
		f->handle = PHYSFS_openRead(path_utf8);
		break;
	}
	case 1: { // write
		f->handle = PHYSFS_openWrite(path_utf8);
		break;
	}
	case 2: { // append
		f->handle = PHYSFS_openAppend(path_utf8);
		break;
	}
	default: hl_assert(); break;
	}
	if (f->handle == NULL) {
		spew_error();
		return NULL;
	}
	f->finalize = HL_NAME(close);
	return f;
}

HL_PRIM vbyte *HL_NAME(read_all)(exphysfs_file *file) {
	if (file->handle != NULL) {
		if (file->mode != 0) {

			return NULL;
		}
		int64 len = HL_NAME(get_length)(file);
		vbyte *buf = hl_alloc_bytes((int)len);
		PHYSFS_sint64 result = PHYSFS_readBytes(file->handle, (void *)buf, (PHYSFS_uint64)len);
		if (!PHYSFS_seek(file->handle, 0)) {
			spew_error();
			return NULL;
		}
		if (result < 0) {
			spew_error();
			return NULL;
		}
		return buf;
	}
	return NULL;
}

HL_PRIM void HL_NAME(seek)(exphysfs_file *file, int64 pos) {
	if (file->handle != NULL) {
		if (!PHYSFS_seek(file->handle, (PHYSFS_uint64)pos)) {
			spew_error();
			return;
		}
	}
}

HL_PRIM int64 HL_NAME(tell)(exphysfs_file *file) {
	if (file->handle != NULL) {
		PHYSFS_sint64 pos = PHYSFS_tell(file->handle);
		if (pos < 0) {
			spew_error();
			return 0;
		}
		return (int64)pos;
	}
	return 0;
}

HL_PRIM bool HL_NAME(eof)(exphysfs_file *file) {
	if (file->handle != NULL) {
		// always report eof for writable files, since you can't read them
		// and as far as you're concerned that's the truth anyway.
		return file->mode != 0 || PHYSFS_eof(file->handle) != 0;
	}
	return true;
}

HL_PRIM vdynamic *HL_NAME(stat)(vstring *path) {
	PHYSFS_Stat info;
	if (!PHYSFS_stat(hl_to_utf8(path->bytes), &info)) {
		spew_error();
		return NULL;
	}

	vdynamic *ret = (vdynamic*)hl_alloc_dynobj();
	hl_dyn_seti(ret, hl_hash_utf8("read_only"), &hlt_bool, info.readonly);
	hl_dyn_seti(ret, hl_hash_utf8("file_type"), &hlt_i32, (int)info.filetype);
	// HL's I64 causes some compile issues, so cast to double and hope for the best.
	hl_dyn_setd(ret, hl_hash_utf8("file_size"), (double)info.filesize);
	hl_dyn_setd(ret, hl_hash_utf8("create_time"), (double)info.createtime);
	hl_dyn_setd(ret, hl_hash_utf8("access_time"), (double)info.accesstime);
	hl_dyn_setd(ret, hl_hash_utf8("mod_time"), (double)info.modtime);
	return ret;
}

HL_PRIM varray *HL_NAME(get_directory_items)(vstring *path) {
	char **rc = PHYSFS_enumerateFiles(hl_to_utf8(path->bytes));
	if (rc == NULL) {
		spew_error();
		return NULL;
	}
	int count = 0;
	for (char **i = rc; *i != NULL; i++) count++;

	varray *ret = hl_alloc_array(&hlt_bytes, count);
	int j = 0;
	for (char **i = rc; *i != NULL; i++) {
		hl_aptr(ret, uchar*)[j] = hl_to_utf16(*i);
		j++;
	}
	return ret;
}

HL_PRIM const uchar *HL_NAME(get_base_dir)() {
	return hl_to_utf16(PHYSFS_getBaseDir());
}

HL_PRIM const uchar *HL_NAME(get_write_dir)() {
	const char *write_dir = PHYSFS_getWriteDir();
	return write_dir ? hl_to_utf16(write_dir) : NULL;
}

HL_PRIM const uchar *HL_NAME(get_real_dir)(vstring *path) {
	const char *real_path = PHYSFS_getRealDir(hl_to_utf8(path->bytes));
	return real_path ? hl_to_utf16(real_path) : NULL;
}

#define _PHYSFSFILE _ABSTRACT(exphysfs_file)

DEFINE_PRIM(_VOID, init, _STRING _STRING _STRING);
DEFINE_PRIM(_VOID, mount, _STRING _STRING _BOOL);
DEFINE_PRIM(_PHYSFSFILE, open, _STRING _I32);
DEFINE_PRIM(_VOID, close, _PHYSFSFILE);
DEFINE_PRIM(_BYTES, read_all, _PHYSFSFILE);
DEFINE_PRIM(_I64, get_length, _PHYSFSFILE);

DEFINE_PRIM(_VOID, seek, _PHYSFSFILE _I64);
DEFINE_PRIM(_I64, tell, _PHYSFSFILE);
DEFINE_PRIM(_BOOL, eof, _PHYSFSFILE);

DEFINE_PRIM(_DYN, stat, _STRING);

DEFINE_PRIM(_BYTES, get_base_dir, _NO_ARG);
DEFINE_PRIM(_BYTES, get_write_dir, _NO_ARG);
DEFINE_PRIM(_BYTES, get_real_dir, _STRING);

DEFINE_PRIM(_ARR, get_directory_items, _STRING);
