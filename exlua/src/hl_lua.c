#define HL_NAME(n) exlua_##n

#include <hl.h>
#include <stdio.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

typedef struct _exlua_state exlua_state;
struct _exlua_state {
	void (*finalize)(exlua_state *);
	lua_State *L;
};

// you might want to wrap long running ops with these
//hl_blocking(true); hl_blocking(false);

HL_PRIM void HL_NAME(close_state)(exlua_state *s) {
	if (s->L != NULL) {
		// dump registry
		//int t = 0;
		//lua_pushnil(s->L);  // first key
		//lua_gettable(s->L, LUA_REGISTRYINDEX);
		//printf("%d\n", lua_gettop(s->L));
		/*
		while (lua_next(s->L, t) != 0) {
			// uses 'key' (at index -2) and 'value' (at index -1)
			printf("%s - %s\n",
				lua_typename(s->L, lua_type(s->L, -2)),
				lua_typename(s->L, lua_type(s->L, -1)));
			// removes 'value'; keeps 'key' for next iteration
			lua_pop(s->L, 1);
		}
		*/
		lua_close(s->L);
		s->L = NULL;
		s->finalize = NULL;
	}
}

HL_PRIM int HL_NAME(ref)(exlua_state *s, int idx) {
	if (s->L != NULL) {
		return luaL_ref(s->L, idx);
	}
	return 0;
}

HL_PRIM int HL_NAME(get_registry_index)() {
	return LUA_REGISTRYINDEX;
}

HL_PRIM void HL_NAME(raw_get_i)(exlua_state *s, int idx, int id) {
	if (s->L != NULL) {
		lua_rawgeti(s->L, idx, id);
	}
}

HL_PRIM void HL_NAME(pcall)(exlua_state *s, int args, int results) {
	if (s->L != NULL) {
		int err = lua_pcall(s->L, args, results, 0);
		if (err) {
			vdynamic *msg = hl_alloc_strbytes(hl_to_utf16(lua_tostring(s->L, -1)));
			lua_pop(s->L, 1);
			hl_throw(msg);
		}
	}
}

HL_PRIM exlua_state *HL_NAME(open_state)() {
	exlua_state *s = (exlua_state *)hl_gc_alloc_finalizer(sizeof(exlua_state));
	s->L = luaL_newstate();
	if (s->L != NULL) {
		s->finalize = HL_NAME(close_state);
		// this reduces overhead when loading a lot of objects which aren't garbage.
		// source: http://lua-users.org/lists/lua-l/2008-07/msg00690.html
		lua_gc(s->L, LUA_GCSTOP, 0);
		luaL_openlibs(s->L);
		lua_gc(s->L, LUA_GCRESTART, 0);
	}
	return s;
}

HL_PRIM void HL_NAME(do_buffer)(exlua_state *s, vstring *name, vstring *str) {
	if (str->length > 0 && s->L != NULL) {
		const char *code = hl_to_utf8(str->bytes);
		int err = luaL_loadbuffer(s->L, code, str->length, hl_to_utf8(name->bytes));
		err = err || lua_pcall(s->L, 0, 0, 0);
		if (err) {
			vdynamic *msg = hl_alloc_strbytes(hl_to_utf16(lua_tostring(s->L, -1)));
			lua_pop(s->L, 1);
			hl_throw(msg);
		}
	}
}

static int call_hx_closure(lua_State *L) {
	vclosure *fn = (vclosure*)lua_topointer(L, lua_upvalueindex(1));
	hl_dyn_call(fn, NULL, 0);
	return 0;
}

HL_PRIM void HL_NAME(push_closure)(exlua_state *s, vclosure *fn) {
	if (s->L != NULL) {
		hl_add_root(&fn);
		lua_pushlightuserdata(s->L, fn);
		lua_pushcclosure(s->L, &call_hx_closure, 1);
	}
}

HL_PRIM int HL_NAME(type)(exlua_state *s, int idx) {
	if (s->L != NULL) {
		return lua_type(s->L, idx);
	}
	return LUA_TNONE;
}

HL_PRIM int HL_NAME(get_top)(exlua_state *s) {
	if (s->L != NULL) {
		return lua_gettop(s->L);
	}
	return 0;
}

HL_PRIM void HL_NAME(pop)(exlua_state *s, int count) {
	if (s->L != NULL) {
		lua_pop(s->L, count);
	}
}

HL_PRIM void HL_NAME(push_value)(exlua_state *s, int i) {
	if (s->L != NULL) {
		lua_pushvalue(s->L, i);
	}
}

HL_PRIM void HL_NAME(push_number)(exlua_state *s, double num) {
	if (s->L != NULL) {
		lua_pushnumber(s->L, num);
	}
}

HL_PRIM void HL_NAME(push_integer)(exlua_state *s, int num) {
	if (s->L != NULL) {
		lua_pushinteger(s->L, num);
	}
}

HL_PRIM void HL_NAME(push_boolean)(exlua_state *s, bool b) {
	if (s->L != NULL) {
		lua_pushboolean(s->L, b);
	}
}

HL_PRIM void HL_NAME(push_string)(exlua_state *s, vstring *str) {
	if (s->L != NULL) {
		lua_pushlstring(s->L, hl_to_utf8(str->bytes), str->length);
	}
}

HL_PRIM void HL_NAME(push_nil)(exlua_state *s) {
	if (s->L != NULL) {
		lua_pushnil(s->L);
	}
}

HL_PRIM double HL_NAME(check_number)(exlua_state *s, int idx) {
	if (s->L != NULL) {
		if (lua_isnumber(s->L, idx)) {
			return (double)lua_tonumber(s->L, idx);
		}
		hl_throw(hl_alloc_strbytes(USTR("argument #%d must be a number"), idx));
	}
	return 0.0;
}

HL_PRIM int HL_NAME(check_integer)(exlua_state *s, int idx) {
	if (s->L != NULL) {
		if (lua_isinteger(s->L, idx)) {
			return (int)lua_tointeger(s->L, idx);
		}
		hl_throw(hl_alloc_strbytes(USTR("argument #%d must be an integer"), idx));
	}
	return 0;
}

HL_PRIM bool HL_NAME(check_boolean)(exlua_state *s, int idx) {
	if (s->L != NULL) {
		if (lua_isboolean(s->L, idx)) {
			return (bool)lua_toboolean(s->L, idx);
		}
		hl_throw(hl_alloc_strbytes(USTR("argument #%d must be a boolean"), idx));
	}
	return false;
}

HL_PRIM const uchar *HL_NAME(check_string)(exlua_state *s, int idx) {
	if (s->L != NULL) {
		if (lua_isstring(s->L, idx)) {
			return hl_to_utf16(lua_tostring(s->L, idx));
		}
		hl_throw(hl_alloc_strbytes(USTR("argument #%d must be a string"), idx));
	}
	return NULL;
}

HL_PRIM bool HL_NAME(is_function)(exlua_state *s, int idx) {
	if (s->L != NULL) {
		return (bool)lua_isfunction(s->L, idx);
	}
	return false;
}

HL_PRIM void HL_NAME(new_table)(exlua_state *s, int narr, int nrec) {
	if (s->L != NULL) {
		lua_createtable(s->L, narr, nrec);
	}
}

HL_PRIM void HL_NAME(set_table)(exlua_state *s, int idx) {
	if (s->L != NULL) {
		lua_settable(s->L, idx);
	}
}

HL_PRIM void HL_NAME(set_field)(exlua_state *s, int idx, vstring *key) {
	if (s->L != NULL) {
		lua_setfield(s->L, idx, hl_to_utf8(key->bytes));
	}
}

HL_PRIM void HL_NAME(set_global)(exlua_state *s, vstring *name) {
	if (s->L != NULL) {
		lua_setglobal(s->L, hl_to_utf8(name->bytes));
	}
}

static size_t luaM_objlen(lua_State *L, int ndx) {
#if LUA_VERSION_NUM == 501
	return lua_objlen(L, ndx);
#else
	return lua_rawlen(L, ndx);
#endif
}

static int luaM_table_insert(lua_State *L, int tindex, int vindex, int pos) {
	if (tindex < 0) {
		tindex = lua_gettop(L) + 1 + tindex;
	}
	if (vindex < 0) {
		vindex = lua_gettop(L) + 1 + vindex;
	}

	if (pos == -1) {
		lua_pushvalue(L, vindex);
		lua_rawseti(L, tindex, (lua_Integer)luaM_objlen(L, tindex) + 1);
		return 0;
	}
	else if (pos < 0) {
		pos = (int)luaM_objlen(L, tindex) + 1 + pos;
	}

	for (int i = (int)luaM_objlen(L, tindex) + 1; i > pos; i--) {
		lua_rawgeti(L, tindex, (lua_Integer)i - 1);
		lua_rawseti(L, tindex, i);
	}

	lua_pushvalue(L, vindex);
	lua_rawseti(L, tindex, pos);
	return 0;
}

static int luaM_register_searcher(lua_State *L, lua_CFunction f, int pos) {
	// Add the package loader to the package.loaders table.
	lua_getglobal(L, "package");

	if (lua_isnil(L, -1)) {
		return luaL_error(L, "Can't register searcher: package table does not exist.");
	}

	lua_getfield(L, -1, "loaders");

	// Lua 5.2 renamed package.loaders to package.searchers.
	if (lua_isnil(L, -1)) {
		lua_pop(L, 1);
		lua_getfield(L, -1, "searchers");
	}

	if (lua_isnil(L, -1)) {
		return luaL_error(L, "Can't register searcher: package.loaders table does not exist.");
	}

	lua_pushcfunction(L, f);
	luaM_table_insert(L, -2, -1, pos);
	lua_pop(L, 3);
	return 0;
}

HL_PRIM void HL_NAME(stack_dump)(exlua_state *s, int start) {
	int i = lua_gettop(s->L) - start;
	int m = i;
	printf("------------- Lua Stack Dump ------------------------\n");
	if (i <= 0) {
		printf("(empty)\n");
	}
	while (i > 0) {
		int t = lua_type(s->L, i);
		int n = -(m - i + 1);
		switch (t) {
			case LUA_TSTRING: {
				printf("%d (%d):`%s'\n", i, n, lua_tostring(s->L, i));
				break;
			}
			case LUA_TBOOLEAN: {
				printf("%d (%d): %s\n", i, n, lua_toboolean(s->L, i) ? "true" : "false");
				break;
			}
			case LUA_TNUMBER: {
				printf("%d (%d): %g\n", i, n, lua_tonumber(s->L, i));
				break;
			}
			default: {
				printf("%d (%d): %s\n", i, n, lua_typename(s->L, t));
				break;
			}
		}
		i--;
	}
	printf("------------- Lua Stack Dump Finished ---------------\n");
}

#define _LUASTATE _ABSTRACT(exlua_state)

DEFINE_PRIM(_LUASTATE, open_state, _NO_ARG);
DEFINE_PRIM(_VOID, close_state, _LUASTATE);
DEFINE_PRIM(_VOID, do_buffer, _LUASTATE _STRING _STRING);

DEFINE_PRIM(_I32, type, _LUASTATE _I32);

DEFINE_PRIM(_I32, get_registry_index, _NO_ARG);
DEFINE_PRIM(_VOID, push_value, _LUASTATE _I32);
DEFINE_PRIM(_VOID, raw_get_i, _LUASTATE _I32 _I32);
DEFINE_PRIM(_I32, ref, _LUASTATE _I32);
DEFINE_PRIM(_VOID, pcall, _LUASTATE _I32 _I32);

DEFINE_PRIM(_I32, get_top, _LUASTATE);
DEFINE_PRIM(_VOID, pop, _LUASTATE _I32);

DEFINE_PRIM(_VOID, push_number, _LUASTATE _F64);
DEFINE_PRIM(_VOID, push_integer, _LUASTATE _I32);
DEFINE_PRIM(_VOID, push_boolean, _LUASTATE _BOOL);
DEFINE_PRIM(_VOID, push_string, _LUASTATE _STRING);
DEFINE_PRIM(_VOID, push_nil, _LUASTATE);
DEFINE_PRIM(_VOID, push_closure, _LUASTATE _FUN(_VOID, _NO_ARG));

DEFINE_PRIM(_F64, check_number, _LUASTATE _I32);
DEFINE_PRIM(_I32, check_integer, _LUASTATE _I32);
DEFINE_PRIM(_BOOL, check_boolean, _LUASTATE _I32);
DEFINE_PRIM(_BYTES, check_string, _LUASTATE _I32);

DEFINE_PRIM(_BOOL, is_function, _LUASTATE _I32);

DEFINE_PRIM(_VOID, new_table, _LUASTATE _I32 _I32);
DEFINE_PRIM(_VOID, set_table, _LUASTATE _I32);
DEFINE_PRIM(_VOID, set_field, _LUASTATE _I32 _STRING);
DEFINE_PRIM(_VOID, set_global, _LUASTATE _STRING);

DEFINE_PRIM(_VOID, stack_dump, _LUASTATE _I32);
