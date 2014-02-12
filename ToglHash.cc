// ToglHash.cc           keeps track of Togl widgets by name for PlyView
// created 5/29/98       magi


#include "ToglHash.h"
#include "togl.h"


//global exported via plvGlobals.h
ToglHash toglHash;



ToglHash::ToglHash()
{
  Tcl_InitHashTable (&hashTable, TCL_STRING_KEYS);
}


ToglHash::~ToglHash()
{
  Tcl_DeleteHashTable (&hashTable);
}


struct Togl* ToglHash::FindTogl (char* name)
{
  //printf ("Looking for togl '%s'\n", name);

  Tcl_HashEntry* hash = Tcl_FindHashEntry (&hashTable, name);
  if (hash == NULL)
    return NULL;
  else
    return (struct Togl*)Tcl_GetHashValue (hash);
}


void ToglHash::AddToHash (char* name, struct Togl* togl)
{
  //printf ("Togl created, name '%s', ptr %x\n", name, togl);

  int iNew;
  Tcl_HashEntry* hash = Tcl_CreateHashEntry (&hashTable, name, &iNew);
  if (hash != NULL)
    Tcl_SetHashValue (hash, togl);
}
