#include <iostream>
#include "include/lua.hpp"

#pragma comment (lib, "lua54.lib")

int main()
{
	const char* program = "print \"Hello from Lua\"";

	lua_State* L = luaL_newstate();
	luaL_openlibs(L);
	luaL_loadfile(L, "dragon.lua");
	int ret = lua_pcall(L, 0, 0, 0);
	if (0 != ret) {
		std::cout << "Error: " << lua_tostring(L, -1);
		lua_pop(L, 1);
	}

	/*lua_getglobal(L, "pos_x");
	lua_getglobal(L, "pos_y");
	int pos_x = lua_tonumber(L, -2);
	int pos_y = lua_tonumber(L, -1);
	lua_pop(L, 2);
	std::cout << "POS_X = " << pos_x << ", POS_Y = " << pos_y << std::endl;*/
	lua_getglobal(L, "plustwo");
	lua_pushnumber(L, 10);
	lua_pcall(L, 1, 1, 0);
	int res = lua_tonumber(L, -1);
	lua_pop(L, 1);
	std::cout << "Result = " << res << std::endl;
	
	lua_close(L);
}