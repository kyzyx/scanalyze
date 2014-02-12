#include <string>
#include <tk.h>
#include <stdlib.h>
#include "defines.h"
#include "plvGlobals.h"
#include "plvCmds.h"
#include "Timer.h"
#include "plvScene.h"
#include "plvDrawCmds.h"
#include "FileNameUtils.h"

// BUGBUG - shouldn't need this here, but it helps with the compile.
class Scene;

int
PlvParamCmd(ClientData clientData, Tcl_Interp *interp,
		int argc, char *argv[])
{
  if (argc == 1) {
    printf("Command: plv_param [-option value]\n");
    printf("  -warn <boolean> (%d)\n", Warn);
    printf("  -areanorms <int> (%d)\n", UseAreaWeightedNormals);
    printf("  -subsamp <int> (%d)\n", SubSampleBase);
    printf("  -numprocs <int> (%d)\n", NumProcs);
  }
  else {
    for (int i = 1; i < argc; i++) {
      if (!strcmp(argv[i], "-warn")) {
	i++;
	if (!strcmp(argv[i], "0") || !strcmp(argv[i], "off")) {
	  Warn = FALSE;
	}
	else if (!strcmp(argv[i], "1") || !strcmp(argv[i], "on")){
	  Warn = TRUE;
	}
	else {
	  interp->result = "bad arg to plv_param -warn";
	  return TCL_ERROR;
	}
      }
      else if (!strcmp(argv[i], "-areanorms")) {
	i++;
	UseAreaWeightedNormals = atoi(argv[i]);
      }
      else if (!strcmp(argv[i], "-subsamp")) {
	i++;
	SubSampleBase = atoi(argv[i]);
      }
      else if (!strcmp(argv[i], "-numprocs")) {
	i++;
	NumProcs = atoi(argv[i]);
      }
      else {
	interp->result = "bad args to plv_param";
	return TCL_ERROR;
      }
    }
  }

  return TCL_OK;
}



Tk_RestrictAction
updateWindowFilter (ClientData clientData, XEvent* eventPtr)
{
  Tk_Window parent = (Tk_Window)clientData;
  Window w = ((XAnyEvent*)eventPtr)->window;
  Display* d = ((XAnyEvent*)eventPtr)->display;
  Tk_Window child = Tk_IdToWindow (d, w);

  while (child != NULL) {
    //cerr << "checking " << Tk_PathName (child) << " : " << flush;

    if (child == parent) {
      //cerr << "yup" << endl;
      return TK_PROCESS_EVENT;
    }

    //cerr << "nope" << endl;
    child = Tk_Parent (child);
  }

  return TK_DEFER_EVENT;
}


int
PlvUpdateWindowCmd(ClientData clientData, Tcl_Interp *interp,
		   int argc, char *argv[])
{
  // NOTE, the idea of this was to process all pending events for a given
  // window and no others, but this doesn't work because most events
  // actually take shape as idle events before they do anything, and we
  // can't filter idle events, only window-system events.

  // But it's not entirely useless, because the builtin "update" command
  // only has two options, process everything or process only idle events,
  // and here we can process idle or window events without file events,
  // meaning clear the pipeline of all existing events including window
  // resizes, etc., without accepting new input (which comes in as file
  // events).

  if (argc < 2) {
    interp->result = "bad args";
    return TCL_ERROR;
  }

  Tk_Window tkwin = Tk_NameToWindow (interp, argv[1],
				     Tk_MainWindow (interp));
  if (!tkwin)
    return TCL_ERROR;  // interp->result already set

  // set up message filter, so we can be notified if messages arrive
  Tk_RestrictProc* old_filter_proc;
  ClientData old_filter_data;
  old_filter_proc = Tk_RestrictEvents (updateWindowFilter, tkwin,
				       &old_filter_data);

  //Tcl_Eval (interp, argv[2]);

  while (Tk_DoOneEvent (TK_WINDOW_EVENTS | TK_IDLE_EVENTS | TK_DONT_WAIT))
    ;

  // remove message filter
  Tk_RestrictEvents (old_filter_proc, old_filter_data, &old_filter_data);

  return TCL_OK;
}


int
SczGetSystemTickCountCmd (ClientData clientData, Tcl_Interp *interp,
			  int argc, char *argv[])
{
  unsigned int ticks = Timer::get_system_tick_count();
  char buf[20];
  sprintf (buf, "%u", ticks);
  Tcl_SetResult (interp, buf, TCL_VOLATILE);

  return TCL_OK;
}


int SczSessionCmd (ClientData clientData, Tcl_Interp *interp,
		   int argc, char *argv[])
{
  if (argc < 3) {
    interp->result = "bad # args";
    return TCL_ERROR;
  }

  if (!strcmp (argv[1], "load")) {
    if (!theScene->readSessionFile (argv[2])) {
      interp->result = "Session read failed.";
      return TCL_ERROR;
    }

  } else if (!strcmp (argv[1], "save")) {
    if (!theScene->writeSessionFile (argv[2])) {
      interp->result = "Session write failed.";
      return TCL_ERROR;
    }

  } else if (!strcmp (argv[1], "activate")) {
    DisplayableMesh* dm = FindMeshDisplayInfo (argv[2]);
    if (!dm) {
      interp->result = "Scan not in session";
      return TCL_ERROR;
    }

    if (argc != 4) {
      interp->result = "Bad # args";
      return TCL_ERROR;
    }
    bool active = atoi (argv[3]);

    if (!theScene->setScanLoadedStatus (dm, active)) {
      interp->result = "Unload/reload failed";
      return TCL_ERROR;
    }

  } else {
    interp->result = "bad subcommand";
    return TCL_ERROR;
  }

  return TCL_OK;
}


int
SczPseudoGroupCmd(ClientData clientData, Tcl_Interp *interp,
		  int argc, char *argv[])
{
  if (argc < 3) {
    interp->result = "bad # args";
    return TCL_ERROR;
  }

  // first create a directory
  if (portable_mkdir(argv[1], 00775)) {
    cerr << "Could not create directory " << argv[1] << endl;
    return TCL_ERROR;
  }

  // now, get the meshes, create soft links to them into
  // the directory we just created
  // argv[2] is a space separated list of mesh names
  char *start, *end;
  start = argv[2];
  bool done = false;
  while (!done) {
    end = start+1;
    while (*end != ' ' && *end != '\0') end++;
    if (*end == ' ') *end = '\0';
    else             done = true;

    // now start points to a null terminated string
    DisplayableMesh* dm = FindMeshDisplayInfo (start);
    if (dm) {
      RigidScan *rs = dm->getMeshData();
      if (rs) {
	{
	  char buf[256];
	  getcwd(buf,256);
	  std::string src(buf);
	  src += '/'; src += rs->get_name().c_str();
	  //cout << src << endl;
	  std::string tgt(argv[1]);
	  tgt += '/'; tgt += start; tgt += '.';
	  tgt += rs->get_nameending().c_str();
	  //cout << tgt << endl;
	  portable_symlink(src.c_str(), tgt.c_str());
	}
	{
	  char buf[256];
	  getcwd(buf,256);
	  std::string src(buf);
	  src += '/'; src += rs->get_basename().c_str(); src += ".xf";
	  //cout << src << endl;
	  std::string tgt(argv[1]);
	  tgt += '/'; tgt += start; tgt += ".xf";
	  //cout << tgt << endl;
	  portable_symlink(src.c_str(), tgt.c_str());
	}
      }
    }
    //cout << start << endl;

    if (!done) start = end+1;
  }

  return TCL_OK;
}
