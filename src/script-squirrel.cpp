#include "script-squirrel.h"
#include <cstring>

#ifdef SCRIPT_SUPPORT

void registerStdLib(HSQUIRRELVM);

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
bool functionCall(HSQUIRRELVM v, const char *fn, const char *args, ...)
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
    } else
	return false;
    sq_settop(v, top);

    va_end(arglist);
    return true;
}

/*
 * functionRegister
 * Registers a function in Squirrel VM
 */
void functionRegister(HSQUIRRELVM v, SQFUNCTION f, const char *name)
{
    sq_pushroottable(v);
    sq_pushstring(v, name, -1);
    sq_newclosure(v, f, 0);
    sq_createslot(v, -3);
    sq_pop(v, 1);
}

/*
 * Test function called from Squirrel (modified form Squirrel docs)
 * Prints the type of all arguments.
 */
int testFunc(HSQUIRRELVM v)
{
    int nargs = sq_gettop(v);

    for (int n = 1; n <= nargs; n++) {
	printf("arg: %d is ", n);
	switch (sq_gettype(v, n))
	{
	case OT_NULL:
	    printf("null");
	    break;
	case OT_INTEGER:
	    printf("integer");
	    break;
	case OT_FLOAT:
	    printf("float");
	    break;
	case OT_STRING:
	    printf("string");
	    break;
	case OT_TABLE:
	    printf("table");
	    break;
	case OT_ARRAY:
	    printf("array");
	    break;
	case OT_USERDATA:
	    printf("userdata");
	    break;
	case OT_CLOSURE:
	    printf("closure");
	    break;
	case OT_NATIVECLOSURE:
	    printf("nativeclosure");
	    break;
	case OT_GENERATOR:
	    printf("generator");
	    break;
	case OT_USERPOINTER:
	    printf("userpointer");
	    break;
	default:
	    printf("unknown");
	}
    }
    printf("\n");
    sq_pushinteger(v, nargs);
    return 1;
}

/******/

ScriptSquirrel::ScriptSquirrel(const std::string &file) :
    Script(file)
{
    vm = sq_open(1024);  //1024 byte stack
    sqstd_seterrorhandlers(vm);
    sq_setprintfunc(vm, printfunc);

    sq_pushroottable(vm);
    if (!SQ_SUCCEEDED(sqstd_dofile(vm, _SC(scriptName.c_str()), 0, 1))) {
	std::cerr << "Error: ScriptSquirrel: could not execute " <<
	    scriptName << std::endl;
    } else {
        registerStdLib(vm);

	functionCall(vm, "", NULL);
	functionCall(vm, "init", NULL);
    }
}

ScriptSquirrel::~ScriptSquirrel()
{
    functionCall(vm, "destroy", NULL);

    sq_pop(vm, 1);
    sq_close(vm);
}

void ScriptSquirrel::update()
{
    functionCall(vm, "update", NULL);
}

bool ScriptSquirrel::execute(const std::string &functionName)
{
    return functionCall(vm, functionName.c_str(), NULL);
}

void ScriptSquirrel::message(char *msg)
{
    functionCall(vm, "message", "s", msg);
}

/******************************/
/*
 * Standard Library Functions
 */
int getName(HSQUIRRELVM v)
{
    sq_pushstring(v, "no name", -1);
    return 1;
}

int getX(HSQUIRRELVM v)
{
    sq_pushinteger(v, 0);
    return 1;
}

int getY(HSQUIRRELVM v)
{
    sq_pushinteger(v, 0);
    return 1;
}

int getMap(HSQUIRRELVM v)
{
    sq_pushstring(v, "map1.map", -1);
    return 1;
}

int getLevel(HSQUIRRELVM v)
{
    sq_pushinteger(v, 0);
    return 1;
}

int getHealth(HSQUIRRELVM v)
{
    sq_pushinteger(v, 0);
    return 1;
}

int getMaxHealth(HSQUIRRELVM v)
{
    sq_pushinteger(v, 0);
    return 1;
}

int getAttack(HSQUIRRELVM v)
{
    sq_pushinteger(v, 0);
    return 1;
}

int getDefense(HSQUIRRELVM v)
{
    sq_pushinteger(v, 0);
    return 1;
}

int getLuck(HSQUIRRELVM v)
{
    sq_pushinteger(v, 0);
    return 1;
}

int getVitality(HSQUIRRELVM v)
{
    sq_pushinteger(v, 0);
    return 1;
}

/*
 *  Register standard functions for game script
 */
void registerStdLib(HSQUIRRELVM v)
{
    functionRegister(v, getName, "getName");
    functionRegister(v, getX, "getX");
    functionRegister(v, getY, "getY");
    functionRegister(v, getMap, "getMap");
    functionRegister(v, getLevel, "getLevel");
    functionRegister(v, getHealth, "getHealth");
    functionRegister(v, getMaxHealth, "getMaxHealth");
    functionRegister(v, getAttack, "getAttack");
    functionRegister(v, getDefense, "getDefense");
    functionRegister(v, getLuck, "getLuck");
    functionRegister(v, getVitality, "getVitality");
}

#endif
