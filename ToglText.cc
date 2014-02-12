// ToglText.cc            Wrapper for easy text output in a Togl widget
// created 4/30/99        magi@cs.stanford.edu


#include "ToglText.h"
#include "togl.h"


#include <hash_map.h>
struct eqtoglptr
{
  bool operator()(unsigned long t1, unsigned long t2) const
  {
    return (t1 == t2);
  }
};

typedef hash_map<unsigned long, GLuint, hash<unsigned long>,
  eqtoglptr> FontlistToglHash;

static FontlistToglHash fonthash;


GLuint InitForTogl (struct Togl* togl,
		    const char* font = TOGL_BITMAP_9_BY_15)
{
  GLuint fontbase = Togl_LoadBitmapFont (togl, font);
  return fontbase;
}


void DrawText (struct Togl* togl, char* string)
{
  Togl_MakeCurrent (togl);
  GLuint fontbase = fonthash[(unsigned long)togl];
  if (!fontbase) {
    fontbase = InitForTogl (togl);
    fonthash[(unsigned long)togl] = fontbase;
  }

  glListBase (fontbase);
  glCallLists (strlen (string), GL_BYTE, string);
}
