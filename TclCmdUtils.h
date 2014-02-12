#ifndef _TCL_CMD_UTILS_H_
#define _TCL_CMD_UTILS_H_


char* GetTclGlobal (char* name, char* def = NULL);
bool  GetTclGlobalBool (char* name, bool def = false);

#define SetBoolFromArgIndex(index, var) \
   if (!SetVarFromBoolArgListIndex (argc, argv, index, var)) \
        return TCL_ERROR;

bool SetVarFromBoolArgListIndex (int argc, char** argv,
				 int index, bool& var);
bool SetVarFromBoolString (char* str, bool& out);


#endif // _TCL_CMD_UTILS_H_
