// ToglHash.h            keeps track of Togl widgets by name for PlyView
// created 5/29/98       magi


#ifndef _toglhash_h_
#define _toglhash_h_

#include <tcl.h>
struct Togl;

class ToglHash
{
 public:
  ToglHash();
  ~ToglHash();

  struct Togl* FindTogl (char * name);
  void AddToHash (char* name, struct Togl* togl);

 private:
  Tcl_HashTable hashTable;
};


#endif //_toglhash_h_
