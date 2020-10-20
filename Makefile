all:
	@haxe build.hxml
	@genie ninja
	@make -C scripts

clean:
	rm -rf bin
	rm -rf tmp
	rm -rf scripts/native
	rm scripts/Makefile

run: all
	@echo ""
	@exec ./bin/test

.PHONY: all clean run
