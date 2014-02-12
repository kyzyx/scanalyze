////////////////////////////////////////////////////////////////////////
//
// File: RefCount.c++
//
////////////////////////////////////////////////////////////////////////

#include "RefCount.h"

////////
//
// Destructor
//
////////

RefCount::~RefCount(
    )
{
    assert( myRefCount == 0 );
#ifdef LEAK_TEST
    int* leak = (int*)(~ myLeak);
    delete leak;
#endif
}


////////
//
// incRefCount
//
////////

void RefCount::incRefCount()
{
    if ( this != NULL ) {
	myRefCount++;
    }
}

////////
//
// decRefCount
//
////////

void RefCount::decRefCount()
{
    if ( this != NULL ) {
	assert( myRefCount >= 0 );
	if ( --myRefCount == 0 ) {
	    delete this;
	}
    }
}

