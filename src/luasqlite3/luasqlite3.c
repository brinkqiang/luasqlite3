#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

const char* LIB_NAME = "_luasqlite3";

typedef struct sqlite_t {
	sqlite3 *db;
	int lua_callback;
	lua_State* l;
}sqlite_t;

static int _open(lua_State* L) {
	const char* filename = luaL_checkstring(L, 1);
	luaL_argcheck(L, filename != NULL, 1, "database name expected");
	sqlite_t* ud = (sqlite_t*)lua_newuserdata(L, sizeof(struct sqlite_t));
	int rc = sqlite3_open(filename, &ud->db);
	if (rc != 0) {
		luaL_error(L, "open sqlite db failed");
		return 0;
	}
	luaL_getmetatable(L, LIB_NAME);

	lua_setmetatable(L, -2);
	return 1;
}
static sqlite_t* checkarg(lua_State* L) {
	void *ud = luaL_checkudata(L, 1, LIB_NAME);
	luaL_argcheck(L, ud != NULL, 1, "sqlite_t expected");
	return (sqlite_t*)ud;
}

static int _exec_callback(void* data, int argc, char **argv, char **columnName) {
	int i = 0;
	sqlite_t *ud = (sqlite_t*)data;
	if (ud) {
		lua_rawgeti(ud->l, LUA_REGISTRYINDEX, ud->lua_callback);
		lua_newtable(ud->l);
		for (i=0; i<argc; i++) {
			lua_pushstring(ud->l, columnName[i]);
			lua_pushstring(ud->l, argv[i]);
			lua_settable(ud->l, -3);
		}
		lua_pcall(ud->l, 1, 0, 0);
	}
	return 0;
}
static int _sqlite3_exec (lua_State* L) {
	char *errMsg = 0;
	int rv = 0;
	sqlite_t* ud = checkarg(L);
	const char* sql = lua_tostring(L, 2);
	if (sql == NULL) {
		luaL_error(L, "sql is NULL");
		lua_pushinteger(L, -1);
		return 1;
	}
	int ltype = lua_type(L, 3);
	if (ltype == LUA_TFUNCTION) {
		ud->lua_callback = luaL_ref(L, LUA_REGISTRYINDEX);
		ud->l = L;
		rv = sqlite3_exec(ud->db, sql, _exec_callback, (void*)ud, &errMsg);
		if (rv != SQLITE_OK) {
			luaL_error(L, errMsg);
			sqlite3_free(errMsg);
			lua_pushinteger(L, rv);
			return 1;
		}
		luaL_unref(L, LUA_REGISTRYINDEX, ud->lua_callback);
		lua_pushinteger(L, rv);
		return 1;
	} else {
		rv = sqlite3_exec(ud->db, sql, NULL, (void*)ud, &errMsg);
		if (rv != SQLITE_OK) {
			luaL_error(L, errMsg);
			sqlite3_free(errMsg);
			lua_pushinteger(L, rv);
			return 1;
		}
		lua_pushinteger(L, rv);
		return 1;
	}
}

static int _exec_batch_callback(void* data, int argc, char **argv, char **columnName) {
	int i = 0;
	sqlite_t *ud = (sqlite_t*)data;
	if (ud) {
		lua_newtable(ud->l);
		for (i=0; i<argc; i++) {
			/* key */
			lua_pushstring(ud->l, columnName[i]); 
			/* value */
			lua_pushstring(ud->l, argv[i]);       
			lua_settable(ud->l, -3);
		}
		size_t size = lua_rawlen(ud->l, -2);
		lua_pushinteger(ud->l, size+1);
		lua_insert(ud->l, -2);  /* switch index -2 and -1*/
		lua_settable(ud->l, -3);
	}
	return 0;
}
static int _exec(lua_State* L) {
	char *errMsg = 0;
	int rv = 0;
	sqlite_t* ud = checkarg(L);
	const char* sql = lua_tostring(L, 2);
	if (sql == NULL) {
		luaL_error(L, "sql is NULL");
		lua_pushinteger(L, -1);
		return 1;
	}
	int ltype = lua_type(L, 3);
	if (ltype == LUA_TFUNCTION) {
		/*printf("LUA_TFUNCTION");*/
		ud->lua_callback = luaL_ref(L, LUA_REGISTRYINDEX);
		ud->l = L;
		/* push lua callback function to stack */
		lua_rawgeti(ud->l, LUA_REGISTRYINDEX, ud->lua_callback);
		/* push result table */
		lua_newtable(L);
		rv = sqlite3_exec(ud->db, sql, _exec_batch_callback, (void*)ud, &errMsg);
		if (rv != SQLITE_OK) {
			luaL_error(L, errMsg);
			sqlite3_free(errMsg);
			lua_pushinteger(L, rv);
			return 1;
		}
		/* invoke the lua callback function */
		lua_pcall(ud->l, 1, 0, 0);
		lua_pushinteger(L, rv);
		luaL_unref(L, LUA_REGISTRYINDEX, ud->lua_callback);
		return 1;
	} else {
		/*printf("not LUA_TFUNCTION\n");*/
		rv = sqlite3_exec(ud->db, sql, NULL, (void*)ud, &errMsg);
		if (rv != SQLITE_OK) {
			luaL_error(L, errMsg);
			sqlite3_free(errMsg);
			lua_pushinteger(L, rv);
			return 1;
		}
		/* invoke the lua callback function */
		lua_pushinteger(L, rv);
		return 1;
	}
}
static int _test(lua_State* L) {
	sqlite_t* ud = checkarg(L);	

	int lua_callback = luaL_ref(L, LUA_REGISTRYINDEX);
	if (lua_callback != LUA_REFNIL) {
		lua_rawgeti(L, LUA_REGISTRYINDEX, lua_callback);
		/**
		 * lua_newtable(L); // 创建一个table
		 * lua_pushstring(L, "intVal");  //key为intVal
		 * lua_pushinteger(L,1234);      //值为1234
		 * lua_settable(L, -3);          //写入table
		 */
		lua_newtable(L);
		int i = 0;
		for (i=1; i<10; i++) {
			lua_pushinteger(L, i);
			lua_pushstring(L, "hello");
			lua_settable(L, -3);
		}
		/* call with 1 argument and 0 result */
		lua_pcall(L, 1, 0, 0);
	}
	lua_pushstring(L, "test func success");
	return 1;
}

static int _gc(lua_State* L) {
	sqlite_t* ud = checkarg(L);	
	if (ud->db) {
		sqlite3_close(ud->db);
		ud->db = NULL;
		lua_pushstring(L, "gc success");
	} else {
		lua_pushstring(L, "gc failed");
	}
	return 1;
}

static const struct luaL_Reg lib_f[] = {
	{"open", _open},
	{NULL, NULL}
};
static const struct luaL_Reg lib_m[] = {
	{"test", _test},
	{"__gc", _gc},
	{"sqlite3_exec", _sqlite3_exec},
	{"exec", _exec},
	{NULL, NULL}
};
int luaopen_luasqlite3(lua_State *L) {
	/*create metatable of the array*/
	luaL_newmetatable(L, LIB_NAME);

	/* metatable.__index = metatable */
	lua_pushvalue(L, -1); /* duplicates the metatable */

	/**
	 * Does the equivalent to t[k] = v, where t is the value at the given index 
	 * and v is the value at the top of the stack. 
	 * This function pops the value from the stack. 
	 * As in Lua, this function may trigger a metamethod for the "newindex" event (see §2.4). 
	 */
	lua_setfield(L, -2, "__index");

	/**
	 * Registers all functions in the array l (see luaL_Reg) into the table on the top of the stack 
	 * (below optional upvalues, see next). 
	 */
	luaL_setfuncs(L, lib_m, 0);
	luaL_newlib(L, lib_f);
	return 1;
}

