#include "script-squirrel.h"
#include <cstring>

/*
 * printfunc - Print function for Squirrel
 */
void printfunc(HSQUIRRELVM v, const SQChar *s, ...)
{
    va_list arglist;
    va_start(arglist, s);
    vprintf(s, arglist);
    va_end(arglist);
}

/*
 * functionCall - Call function with arguments
 * fn   - name of function
 * args - string with argument types.
 *        's' = String
 *        'i' = Integer
 *        'f' = Float
 */
void functionCall(HSQUIRRELVM v, char *fn, char *args, ...)
{
    int argCount = 0;
    va_list arglist;
    va_start(arglist, args);

    int top = sq_gettop(v); //save stack
    sq_pushroottable(v);    //pushes global table
    sq_pushstring(v, _SC(fn), -1);
    if (SQ_SUCCEEDED(sq_get(v, -2)))
    {
	sq_pushroottable(v); //push 'this'

	if (args != NULL)
	{
	    for (int i = 0; i < strlen(args); i++)
	    {
		switch (args[i])
		{
		case 'S':
		case 's':
		    //string
		    argCount++;
		    sq_pushstring(v, va_arg(arglist, char*), -1);
		    break;
		case 'I':
		case 'i':
		    //integer
		    argCount++;
		    sq_pushinteger(v, va_arg(arglist, int));
		    break;
		case 'F':
		case 'f':
		    //float
		    argCount++;
		    sq_pushfloat(v, va_arg(arglist, float));
		    break;
		}
	    }
	}
    
	sq_call(v, argCount + 1, 0);
    }
    sq_settop(v, top);

    va_end(arglist);
}

ScriptSquirrel::ScriptSquirrel(const std::string &file) :
    Script(file)
{
}

ScriptSquirrel::~ScriptSquirrel()
{
    sq_pop(vm, 1);
    sq_close(vm);
}

void ScriptSquirrel::init()
{
    vm = sq_open(2048);
    sqstd_seterrorhandlers(vm);
    sq_setprintfunc(vm, printfunc);

    sq_pushroottable(vm);
    sqstd_dofile(vm, _SC(scriptName.c_str()), 0, 1);
    if (SQ_SUCCEEDED(sqstd_dofile(vm, _SC("test.nut"), 0, 1)))
	functionCall(vm, "init", NULL);
}

void ScriptSquirrel::destroy()
{
    functionCall(vm, "destroy", NULL);
}

void ScriptSquirrel::update()
{
    functionCall(vm, "update", NULL);
}

void ScriptSquirrel::message(char *msg)
{
    functionCall(vm, "message", "s", msg);
}

