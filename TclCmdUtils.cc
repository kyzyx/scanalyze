#include "plvGlobals.h"
#include "TclCmdUtils.h"


static bool
boolFromString (char* str)
{
  struct { char* name; bool val; } table[] = {
    { "true", true },
    { "false", false },
    { "yes", true },
    { "no", false },
    { "on", true },
    { "off", false }
  };
  #define nTable (sizeof(table) / sizeof(table[0]))

  for (int i = 0; i < nTable; i++) {
    if (!strcmp (table[i].name, str))
      return table[i].val;
  }

  return (atoi (str) > 0);
}


char* GetTclGlobal (char* name, char* def)
{
  char* val = Tcl_GetVar (g_tclInterp, name, TCL_GLOBAL_ONLY);
  if (!val)
    return def;

  return val;
}


bool  GetTclGlobalBool (char* name, bool def)
{
  char* val = GetTclGlobal (name);
  if (!val)
    return def; // == false

  return boolFromString (val);
}


bool SetVarFromBoolString (char* str, bool& out)
{
  // could have this error-check that given str is actually a bool value
  out = boolFromString (str);
  return true;
}


bool SetVarFromBoolArgListIndex (int argc, char** argv,
			int i, bool& var)
{
  if (i >= argc) {
    char buffer[1000];
    sprintf (buffer, "Missing argument to %s %s\n", argv[0], argv[i-1]);
    Tcl_SetResult (g_tclInterp, buffer, TCL_VOLATILE);
    return false;
  }

  if (!SetVarFromBoolString (argv[i], var)) {
    char buffer[1000];
    sprintf (buffer, "Bad argument '%s' to %s %s\n",
	     argv[i], argv[0], argv[i-1]);
    Tcl_SetResult (g_tclInterp, buffer, TCL_VOLATILE);
    return false;
  }

  return true;
}
