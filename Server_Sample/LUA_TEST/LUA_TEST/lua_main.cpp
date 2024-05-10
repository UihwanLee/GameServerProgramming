#include <iostream>
#include "include/lua.hpp"

#pragma comment (lib, "lua54.lib")

int main()
{
	const char* program = "print \"Hello from Lua\"";

	lua_State* L = luaL_newstate();
	luaL_openlibs(L);
	luaL_loadbuffer(L, program, strlen(program), "line");
	int ret = lua_pcall(L, 0, 0, 0);
	if (0 != ret) {
		std::cout << "Error: " << lua_tostring(L, -1);
		lua_pop(L, 1);
	}
	lua_close(L);
}