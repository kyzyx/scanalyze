#include <tk.h>
#include <stdlib.h>
#include <malloc.h>
#ifdef WIN32
#  include <io.h>
#else
#  include <unistd.h>
#endif

#include <vector.h>
#include <string>
#include "plvScene.h"
#include "ply++.h"
#include "plvInit.h"
#include "plvCmds.h"
#include "plvGlobals.h"
#include "defines.h"
#include "togl.h"
#include "FileNameUtils.h"
#include "plvAnalyze.h"

#ifdef no_overlay_support
// so I can free the saved image used to emulate overlay planes
#include "plvDraw.h"
#endif

vector<char*> filenames;

#ifdef WIN32
EXTERN int		TkConsoleInit(Tcl_Interp *interp);
#endif

// BUGBUG - shouldn't need this here, but it helps with the compile.
int isPlyFile(char *filename);

static bool
isReadableFile (char* name)
{
  remove_trailing_slash(name);

  // if it's a ply file, we want it
  if (isPlyFile (name))
    return true;

  // if it's a set file, we want it
  if ( filename_has_ending( name, ".set" ) )
    return true;

  // if it's an *.sd file, we want it
  if ( filename_has_ending( name, ".sd" ) ||
       filename_has_ending( name, ".sd.gz" ) )
    return true;

  // ditto for modelmaker's cta and mms format
  if ( filename_has_ending(name, ".cta") ||
       filename_has_ending(name, ".mms") ||
       filename_has_ending(name, ".edges"))
    return true;

  // ditto for cyra's pts format
  if ( filename_has_ending (name, ".pts") )
    return true;

  // also read session files
  if (filename_has_ending (name, ".session"))
    return true;

  // and qsplat files
  if ( filename_has_ending( name, ".qs" ) )
    return true;

  // recognize group files
  if ( filename_has_ending( name, ".gp" ) )
    return true;


  // if it's a directory containing a same-named .ply or .set file,
  // we'll read that instead... so we want it
  char dir[PATH_MAX];
  strcpy (dir, name);
  if (dir[strlen(dir) - 1] == '/')
    dir[strlen(dir) - 1] = 0;

  char file[PATH_MAX];
  char* end = strrchr (dir, '/');
  if (end != NULL)
    end++;
  else
    end = dir;
  sprintf (file, "%s/%s.set", dir, end);
  if (0 == access (file, R_OK))
    return true;

  sprintf (file, "%s/%s.ply", dir, dir);
  if (isPlyFile (file))
    return true;

  sprintf (file, "%s/%s.ply.gz", dir, dir);
  if (isPlyFile (file))
    return true;

  return false;
}


char* find_token_end (char* begin)
{
  char* end = begin;
  while (end != NULL && *end != 0)
  {
    if (*end == ' ')	// space means found end.
      return end;

    if (*end == '\\')		// backslash skips over next char
      ++end;
    else if (*end == '"')	// quote skips until matching quote
      end = strchr (end + 1, '"');
    else if (*end == '{')	// brace skips until matching brace
      end = strchr (end + 1, '}');

    ++end;
  }

  return NULL;
}


/*
 *----------------------------------------------------------------
 *
 * main --
 *
 *	This is the main program for the application.
 *
 * Results:
 *	None: Tk_Main never returns here, so this procedure never
 *	returns either.
 *
 * Side effects:
 *	Whatever the application does.
 *
 *----------------------------------------------------------------
 */

int
main(int argc, char **argv)
{
   int i;

#ifdef _WIN32
   // need to explicitly glob files since shell didn't
   // BUGBUG: what if they use tcsh, bash, etc. which actually will?
   // probably won't hurt, can't think of any reason to have escaped wildcards that
   // need to come here and not get globbed.

   // replacement argc, argv
   vector<std::string*> argv_storage;

   Tcl_Interp* interp = Tcl_CreateInterp();

   for (i = 0; i < argc; i++)
   {
      char szGlobCmd[PATH_MAX];
      sprintf (szGlobCmd, "glob \"%s\"", argv[i]);
      if (TCL_OK == Tcl_Eval (interp, szGlobCmd))
      {
         char* base = interp->result;
	 while (base != NULL)
	 {
            char* end = find_token_end (base);
	    if (end)
	       *end = 0;

	    // Tcl's glob will surround spaced-out filenames with {}
	    char* base_end = base + strlen (base) - 1;
	    if (*base == '{' && *base_end == '}')
	    {
	       base++;
	       *base_end = 0;
	    }

	    argv_storage.push_back (new std::string (base));

	    base = end;
	    if (base != NULL)
	       ++base;
	 }
      }
      else  // glob failed -> not filename -> pass along verbatim
      {
         argv_storage.push_back (new std::string (argv[i]));
      }
   }

   Tcl_DeleteInterp (interp);

   // and update argc, argv
   argc = argv_storage.size();
   argv = new char* [ argc ];
   for (i = 0; i < argc; i++)
   {
      argv[i] = (char*)argv_storage[i]->c_str();
   }
#endif

   int lastarg = argc;
   // take all input files from the end of the command line back
   while (isReadableFile(argv[lastarg-1])) {
      lastarg--;
   }

   for (i = 0; i < argc - lastarg; i++) {
     filenames.push_back(argv[i + lastarg]);
   }
   argc =  lastarg;

   int iarg;
   for (iarg = 1; iarg < argc; iarg++)
       if (0 == strcmp(argv[iarg], "-noui")) {
	   g_bNoUI = true;
	   // if there is an argument to -noui, shift it to the beginning
	   if (argc > iarg+1 && argv[iarg+1][0] != '-') {
	       for (int jarg = iarg; jarg > 1; jarg--)
		   argv[jarg] = argv[jarg-1];
	       argv[1] = argv[iarg+1];
	       iarg++;
	   }
	   --argc;
	   for (; iarg < argc; iarg++)
	       argv[iarg] = argv[iarg+1];
       }

   for (iarg = 1; iarg < argc; iarg++)
       if (0 == strcmp(argv[iarg], "-useintensity")) {
	   g_bNoIntensity = false;
	   --argc;
	   for (; iarg < argc; iarg++)
	       argv[iarg] = argv[iarg+1];
       }

   if (g_bNoUI)
     Tcl_Main(argc, argv, Tcl_AppInit);
   else
     Tk_Main(argc, argv, Tcl_AppInit);

#ifdef no_overlay_support
   // free the fake overaly plane
   delete [] theRenderParams->savedImage;
#endif

   return 0;	      // Needed only to prevent compiler warning.
}


/*
 *----------------------------------------------------------------
 *
 * Tcl_AppInit --
 *
 *	This procedure performs application-specific initialization.
 *	Most applications, especially those that incorporate additional
 *	packages, will have their own version of this procedure.
 *
 * Results:
 *	Returns a standard Tcl completion code, and leaves an error
 *	message in interp->result if an error occurs.
 *
 * Side effects:
 *	Depends on the startup script.
 *
 *----------------------------------------------------------------
 */

int
Tcl_AppInit(Tcl_Interp *interp)
{
    /*
     * Call the init procedures for included packages.
     * Each call should look like this:
     *
     * if (Mod_Init(interp) == TCL_ERROR) {
     *     return TCL_ERROR;
     * }
     *
     * where "Mod" is the name of the module.
     */

puts ("In appInit.  Modules: ");
    if (Tcl_Init(interp) == TCL_ERROR)
      return TCL_ERROR;
puts ("Tcl...");

    if (!g_bNoUI) {
      if (Tk_Init(interp) == TCL_ERROR)
	return TCL_ERROR;

puts ("Tk...");

#ifdef WIN32
      /*
       * Set up a (GUI/Tk) console window.
       * Delete the following statement if you do not need that.
       * Then it'll use a Windows (WINOLDAP) text console.
       */
      puts ("(This console is soon to become useless!)");
      if (TkConsoleInit(interp) == TCL_ERROR) {
	   return TCL_ERROR;
      }
puts ("Console...");
#endif /* WIN32 */

      if (Togl_Init(interp) != TCL_OK) {
	fprintf(stderr, "Togl initialization failed: %s\n",
		interp->result);
	return TCL_ERROR;
      }
puts ("Togl...");
    }

puts ("about to plvinit");
    if (Plv_Init(interp) != TCL_OK) {
	fprintf(stderr, "Scanalyze initialization failed:\n\n%s\n",
		interp->result);
	fprintf(stderr, "\nThings will probably not work as expected.\n");
    }

    Tcl_Eval(interp, "update");

    // need to get z buffer minmaxes, since they're implementation-specific
    // and this is known to be a safe time to call this function (rendering
    // context is set up and mapped)
    storeMinMaxZBufferValues();

    // load any and all files specified on the command line...
    // disable redraw while this is going on, though, to avoid n^2 redraws
    Tcl_GlobalEval (interp, "redraw block");

    // build up list of filenames, and in a separate list, build up a list
    // of all group or .gp files. The .gp files are then loaded in after all
    // the files have been read, to avoid having to load files that would get
    // loaded anyway.
    if (filenames.size()) {
      crope filelist ("readfile");
      crope grouplist ("loadgroup");
      crope space (" ");
      crope inbrace ("{");
      crope outbrace ("}");
      for (int i = 0; i < filenames.size(); i++) {

	// hack to determine whether the filename extension is .gp
	if (strncmp(filenames[i] + strlen(filenames[i]) - 3, ".gp", 3) == 0) {
	  grouplist += space + inbrace + filenames[i] + outbrace;
	} else {
	  filelist += space + inbrace + filenames[i] + outbrace;
	}
      }

      if (strcmp (filelist.c_str(), "readfile") != 0) {
	if (TCL_OK != Tcl_Eval(interp, (char*)filelist.c_str())) {
	  char* err = Tcl_GetVar (interp, "errorInfo", TCL_GLOBAL_ONLY);
	  fprintf (stderr, "Some scans could not be loaded:\n%s\n", err);
	}
      }
      if (strcmp (grouplist.c_str(), "loadgroup") != 0) {
	Tcl_Eval(interp, (char *)grouplist.c_str());
	// now load in any groups
       }
    }
    // center camera on all meshes; set home to this view
    theScene->centerCamera();
    theScene->setHome();
    TbObj::clear_undo();

    // NOW, redraw
    Tcl_GlobalEval (interp, "redraw allow");
    Tcl_GlobalEval (interp, "redraw 1");

    // ~/.scanalyzerc is not placed in tcl_rcFileName, because then tcl
    // runs it automatically without returning any error information.  So
    // we explicitly source this file from plvInit(), and catch and
    // display any errors that occur.
#if 0
    /*
     * Specify a user-specific startup file to invoke if the
     * application is run interactively.
     * Typically the startup file is "~/.apprc"
     * where "app" is the name of the application.
     * If this line is deleted then no user-specific
     * startup file will be run under any conditions.
     */
    Tcl_SetVar(interp, "tcl_rcFileName", "~/.scanalyzerc", TCL_GLOBAL_ONLY);
#endif

    return TCL_OK;
}

