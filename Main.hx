import exlua.Lua;

function main() {
	final lua = Lua.open();
	
	var cb = null;
	lua.new_table();
	lua.push_string("register");
	lua.push_closure(() -> cb = lua.check_function(1));
	lua.set_table(-3);
	lua.set_global("test");

	final msg = "hello from a lua function called by haxe from lua";
	lua.do_buffer("<unnamed>", 'test.register(function() print "$msg" end) ', (e) -> trace(e.message));

	if (cb != null) {
		final results = cb();
		for (result in results) {
			trace(result);
		}
	}
}
