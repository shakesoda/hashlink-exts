package exlua;

// abstract ExtDynamic<T>(Dynamic) from T to T {}
private typedef LuaState_Internal = hl.Abstract<"exlua_state">;

enum LuaValue {
	TNumber(number: Float);
	TInteger(number: Int);
	TBoolean(b: Bool);
	TString(str: String);
	TNil;
}

@:enum abstract LuaType(Int) {
	final TNone = -1;
	final TNil = 0;
	final TBoolean = 1;
	final TLightUserdata = 2;
	final TNumber = 3;
	final TString = 4;
	final TTable = 5;
	final TFunction = 6;
	final TUserdata = 7;
	final TThread = 8;
}

// wrapped this way to make it hard to use an invalid lua state
@:hlNative("exlua")
private extern class LuaInternal {
	static function stack_dump(L: LuaState_Internal, start: Int): Void;
	static function open_state(): LuaState_Internal;
	static function close_state(L: LuaState_Internal): Void;
	static function do_buffer(L: LuaState_Internal, name: String, str: String): Void;
	static function get_registry_index(): Int;
	static function ref(L: LuaState_Internal, idx: Int): Int;
	static function raw_get_i(L: LuaState_Internal, idx: Int, id: Int): Void;
	static function pcall(L: LuaState_Internal, args: Int, results: Int): Void;

	static function get_top(L: LuaState_Internal): Int;
	static function pop(L: LuaState_Internal, count: Int): Void;

	static function type(L: LuaState_Internal, idx: Int): LuaType;

	// stack manipulation
	static function push_value(L: LuaState_Internal, idx: Int): Void;
	static function push_number(L: LuaState_Internal, num: Float): Void;
	static function push_integer(L: LuaState_Internal, num: Int): Void;
	static function push_boolean(L: LuaState_Internal, b: Bool): Void;
	static function push_string(L: LuaState_Internal, str: String): Void;
	static function push_nil(L: LuaState_Internal): Void;
	static function push_closure(L: LuaState_Internal, fn: ()->Void): Void;

	static function check_number(L: LuaState_Internal, idx: Int): Float;
	static function check_integer(L: LuaState_Internal, idx: Int): Int;
	static function check_boolean(L: LuaState_Internal, idx: Int): Bool;
	static function check_string(L: LuaState_Internal, idx: Int): hl.Bytes;

	static function is_function(L: LuaState_Internal, idx: Int): Bool;

	static function new_table(L: LuaState_Internal, narr: Int, nrec: Int): Void;
	static function set_table(L: LuaState_Internal, idx: Int): Void;
	static function set_field(L: LuaState_Internal, idx: Int, key: String): Void;
	static function set_global(L: LuaState_Internal, name: String): Void;
}

abstract LuaState(LuaState_Internal) {
	public inline function stack_dump(start: Int = 0) {
		LuaInternal.stack_dump(this, start);
	}
	public inline function new() {
		this = LuaInternal.open_state();
	}
	public inline function close() {
		LuaInternal.close_state(this);
	}
	public inline function do_buffer(name: String, code: String, on_error: (e: haxe.Exception)->Void) {
		try {
			LuaInternal.do_buffer(this, name, code);
		}
		catch (e) {
			on_error(e);
		}
	}
	public inline function push_closure(fn: ()->Void) {
		LuaInternal.push_closure(this, fn);
	}
	public inline function ref(idx: Int): Int {
		return LuaInternal.ref(this, idx);
	}
	public inline function raw_get_i(idx: Int, id: Int): Void {
		LuaInternal.raw_get_i(this, idx, id);
	}
	public inline function pcall(args: Int, results: Int): Void {
		LuaInternal.pcall(this, args, results);
	}

	public inline function get_top() return LuaInternal.get_top(this);
	public inline function pop(count: Int) LuaInternal.pop(this, count);

	public inline function push_value(idx: Int) LuaInternal.push_value(this, idx);
	public inline function push_number(num: Float) LuaInternal.push_number(this, num);
	public inline function push_integer(num: Int) LuaInternal.push_integer(this, num);
	public inline function push_boolean(b: Bool) LuaInternal.push_boolean(this, b);
	public inline function push_string(str: String) LuaInternal.push_string(this, str);
	public inline function push_nil() LuaInternal.push_nil(this);

	public inline function check_number(idx: Int) return LuaInternal.check_number(this, idx);
	public inline function check_integer(idx: Int) return LuaInternal.check_integer(this, idx);
	public inline function check_boolean(idx: Int) return LuaInternal.check_boolean(this, idx);
	public inline function check_string(idx: Int) return @:privateAccess String.fromUCS2(LuaInternal.check_string(this, idx));

	/** note: does not currently support results **/
	public inline function check_function(idx: Int): (?args: Array<LuaValue>)->Array<LuaValue> {
		if (!LuaInternal.is_function(this, idx)) {
			throw new haxe.Exception('argument $idx must be a function');
		}
		LuaInternal.push_value(this, idx);
		// todo: figure out how to expose these in a way that can unref
		final cb = LuaInternal.ref(this, Lua.registry_index);
		return function(?args: Array<LuaValue>) {
			final top = LuaInternal.get_top(this);
			LuaInternal.raw_get_i(this, Lua.registry_index, cb);
			if (args != null && args.length > 0) {
				for (v in args) {
					switch (v) {
						case TNumber(number): LuaInternal.push_number(this, number);
						case TInteger(number): LuaInternal.push_integer(this, number);
						case TBoolean(b): LuaInternal.push_boolean(this, b);
						case TString(str): LuaInternal.push_string(this, str);
						case TNil: LuaInternal.push_nil(this);
					}
				}
			}
			LuaInternal.pcall(this, args.length, -1 /* LUA_MULTRET */);
			final count = LuaInternal.get_top(this) - top;
			final ret = [];
			for (idx in 0...count) {
				final i = idx + 1;
				// todo: tables, functions, coroutines, userdata, light userdata
				switch (LuaInternal.type(this, i)) {
					case TBoolean: ret.push(LuaValue.TBoolean(LuaInternal.check_boolean(this, i)));
					case TNumber: ret.push(LuaValue.TNumber(check_number(i)));
					case TNil: ret.push(LuaValue.TNil);
					case TString: ret.push(LuaValue.TString(check_string(i)));
					default: trace('can\'t handle a ${LuaInternal.type(this, i)}'); continue;
				}
			}
			LuaInternal.pop(this, count);
			return ret;
		}
	}

	public inline function is_function(idx: Int) return LuaInternal.is_function(this, idx);

	// new table -> push k, push v, set -3 until you're done, then setglobal/field to assign table name.
	public inline function new_table(narr: Int = 0, nrec: Int = 0) LuaInternal.new_table(this, narr, nrec);
	public inline function set_table(idx: Int) LuaInternal.set_table(this, idx);
	public inline function set_field(idx: Int, key: String) LuaInternal.set_field(this, idx, key);
	public inline function set_global(name: String) LuaInternal.set_global(this, name);

	public inline function new_table_auto(?arr: Array<LuaValue>, ?hsh: Map<LuaValue, LuaValue>) {
		final narr = arr != null ? arr.length : 0;
		LuaInternal.new_table(this, narr, 0);
	
		if (arr != null) {
			for (i in 0...arr.length) {
				LuaInternal.push_integer(this, i);
				final v = arr[i];
				switch (v) {
					case TNumber(number): LuaInternal.push_number(this, number);
					case TInteger(number): LuaInternal.push_integer(this, number);
					case TBoolean(b): LuaInternal.push_boolean(this, b);
					case TString(str): LuaInternal.push_string(this, str);
					case TNil: LuaInternal.push_nil(this);
				}
				LuaInternal.set_table(this, -3);
			}
		}
	
		if (hsh != null) {
			for (kv in hsh.keyValueIterator()) {
				final k = kv.key;
				final v = kv.value;
	
				switch (k) {
					case TNumber(number): LuaInternal.push_number(this, number);
					case TInteger(number): LuaInternal.push_integer(this, number);
					case TBoolean(b): LuaInternal.push_boolean(this, b);
					case TString(str): LuaInternal.push_string(this, str);
					case TNil: continue; // nil keys are illegal
				}
	
				switch (v) {
					case TNumber(number): LuaInternal.push_number(this, number);
					case TInteger(number): LuaInternal.push_integer(this, number);
					case TBoolean(b): LuaInternal.push_boolean(this, b);
					case TString(str): LuaInternal.push_string(this, str);
					case TNil: LuaInternal.push_nil(this);
				}
	
				LuaInternal.set_table(this, -3);
			}
		}
	}
}

abstract Lua(Void) {
	public static var registry_index(default, null): Int = 0;
	public static var globals_index(default, null): Int = 0;
	public static inline function open(): LuaState {
		// don't bother calling this all the time, it never changes.
		if (registry_index == 0) {
			registry_index = LuaInternal.get_registry_index();
		}
		return new LuaState();
	}
}
