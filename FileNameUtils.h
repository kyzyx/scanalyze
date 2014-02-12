//############################################################
//
// FileNameUtils.h
//
// Kari Pulli
// Fri Dec 18 11:09:49 CET 1998
//
//############################################################


#ifndef _FILENAMEUTILS_H_
#define _FILENAMEUTILS_H_

#include <iostream.h>
#include <string.h>
#ifndef WIN32
#	include <unistd.h>
#else
#	include <io.h>
#	include <direct.h>
	// supply some missing definitions
#	define S_ISDIR(mode)    ((mode&S_IFMT) == S_IFDIR)
#	ifndef F_OK
#		define R_OK    004     /* Test for Read permission */
#		define W_OK    002     /* Test for Write permission */
#		define X_OK    001     /* Test for eXecute permission */
#		define F_OK    000     /* Test for existence of File */
#	endif
#endif
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

inline bool
get_filename_new_ending(const char* inName, const char* ext,
			char* outName)
{
  if (inName == NULL || inName[0] == 0)
    return false;

  strcpy (outName, inName);
  char* pExt = strrchr (outName, '.');

  if (pExt == NULL)
    pExt = outName + strlen (outName);

  strcpy (pExt, ext);
  return true;
}


inline bool
filename_has_ending(const char* filename, const char *ending)
{
  int lenf = strlen (filename);
  int lene = strlen (ending);
  if (lenf <= lene)
    return false;

  return (0 == strcmp (filename+lenf-lene, ending));
}


inline void
remove_trailing_slash(char *fname)
{
  int n = strlen(fname);
  if (fname[n-1] == '/') fname[n-1] = 0;
}



inline bool
check_file_access(const char *fname,
		  bool existence  = true,
		  bool readable   = true,
		  bool writable   = false,
		  bool executable = false,
		  bool directory  = false)
{
  int res = true;
  // first test the access
  int amode = 0;
  if (existence ) amode |= F_OK;
  if (readable  ) amode |= R_OK;
  if (writable  ) amode |= W_OK;
  if (executable) amode |= X_OK;

  if (access(fname, amode)) {
    cerr << "You don't have the requested access mode "
	 << "for " << fname << endl;
    perror(NULL);
    res = false;
  }
  if (res && directory) {
    struct stat st;
    if (stat(fname, &st)) {
      cerr << "Failed checking whether a directory for "
	   << fname << endl;
      perror(NULL);
      res = false;
    }
    if (!S_ISDIR(st.st_mode)) {
      cerr << fname << " is not a directory" << endl;
      res = false;
    }
  }
  return res;
}


inline bool
unlink_if_exists(const char *fname)
{
  if (access(fname, F_OK) == 0) {
    // file exists
    if (unlink(fname) != 0) {
      perror(fname);
      return false;
    }
  }
  return true;
}


inline int
portable_mkdir(const char* dname, int permissions = 00775)
{
#ifdef WIN32
	return mkdir (dname);
#else
	return mkdir (dname, permissions);
#endif
}

inline int
portable_symlink(const char *s1, const char *s2)
{
#ifndef WIN32
  return symlink(s1, s2);
#else
  cout << "portable_symlink not implemented on WIN32" << endl;
  return 0;
#endif
}

#endif
