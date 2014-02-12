//############################################################
//
// DirEntries.h
//
// Kari Pulli
// Fri Apr  9 18:33:31 CDT 1999
//
// Read in directory entries.
// Can filter entries by the ending.
//
//############################################################


#ifndef _DIRENTRIES_H_
#define _DIRENTRIES_H_

#ifdef WIN32
#  include <io.h>
#  include "defines.h"
#else
#  include <dirent.h>
#endif

#include <string.h>
#include <vector.h>
//#include <rope.h>
#include <string>

class DirEntries {

  int         count;
  std::string m_path;
  vector<std::string> m_entry;

public:

  bool isdir;

  DirEntries(std::string dirname,
	     std::string end_filter = "")
    : count(0)
    {
#ifdef WIN32

      // Windows loop prologue
      struct _finddata_t fd;
      char sdfn[PATH_MAX];
      sprintf (sdfn, "%s/*", dirname.c_str());
      long fh = _findfirst (sdfn, &fd);
      isdir = (fh > 0);
      if (!isdir) return;
      while (1) {
	char *name = fd.name;

#else

      // Unix loop prologue
      DIR *dirp = opendir(dirname.c_str());
      isdir = (dirp != NULL);
      if (!isdir) return;
      struct dirent *direntp;
      while ((direntp = readdir(dirp)) != NULL) {
	char *name = direntp->d_name;

#endif

	// loop body, shared
	if (strcmp(name, ".")  != 0 && strcmp(name, "..") != 0) {
	  if (end_filter.size()) {
	    if (strcmp(&name[strlen(name)-end_filter.size()],
		       end_filter.c_str()) == 0)
	      {
		m_entry.push_back(std::string(name));
	      }
	  }
	}

#ifndef WIN32

      // Windows loop epilogue
      }
      closedir(dirp);
      m_path.assign(dirname);
      m_path += "/";

#else

      // Unix loop epilogue
	if (_findnext (fh, &fd))
	  break;
      }
      _findclose (fh);
      m_path.assign(dirname);
      m_path += "\\";

#endif

    }

  void next(void)
    {
      count++;
    }

  bool done(void)
    {
      return (count >= m_entry.size());
    }

  std::string &entry(void)
    {
      return m_entry[count];
    }

  std::string path(void)
    {
      return m_path + m_entry[count];
    }

  int size(void)
    {
      return m_entry.size();
    }
};



#if 0

#include <iostream.h>

void
main(int argc, char **argv)
{
  for (DirEntries de(argv[1], argc>2 ? argv[2] : NULL);
       !de.done(); de.next()) {
    cout << de.entry() << endl;
  }
}

#endif
#endif
