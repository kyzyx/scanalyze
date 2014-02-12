/*
This is reference-counting code that I originally found in sample source
code distributed by SGI (/usr/share/src/dmedia/video/O2/lib/RefCount.{c++,h}).
As opposed to much of the other source code, it had no copyright statement.

 -- SMR
    Fri Mar 12 06:27:39 CET 1999
*/


////////////////////////////////////////////////////////////////////////
//
// File: RefCount.h
//
////////////////////////////////////////////////////////////////////////

#ifndef _REF_COUNT_H
#define _REF_COUNT_H

#include <assert.h>

#ifndef NULL
#define NULL 0
#endif

////////////////////////////////////////////////////////////////////////
//
// RefCount class
//
// This is a base class that should be inherited by all classes that
// are reference counted.
//
// When the LEAK_TEST variable is set, any ref-counted objects which
// are not deleted when a program exits will report memory leaks.
// This is implemented as a crude hack: when an object is created, at
// allocates one word of memory and then disguises the pointer so that
// it looks like a leak.  When the object is freed, it frees the
// memory.
//
//
////////////////////////////////////////////////////////////////////////

// #define LEAK_TEST

class RefCount
{
public:
    //
    // The constructor just needs to initialize the reference count
    // on the object.
    //

    RefCount()
	{
	    myRefCount = 0;
#ifdef LEAK_TEST
	    int* leak = new int;
	    myLeak = ~ long(leak);
#endif
	}

    //
    // Every base class needs a virtual destructor.
    //

    virtual ~RefCount();

    //
    // Reference counting stuff.
    // It should only be called from within the Ref
    // class.  Compiler didn't like:
    //     template <class Target> friend class Ref;
    //

    void incRefCount();
    void decRefCount();

private:
    int myRefCount;

#ifdef LEAK_TEST
    long myLeak;
#endif
};

////////////////////////////////////////////////////////////////////////
//
// Ref class
//
// This is a reference to an object that maintains a reference count.
// The target class must be derived from RefCount.
//
// To use Refs, first make a class that derives from RefCount, like
// this:
//
//	class Foo : public RefCount
// 	{
// 	public:
// 	    Foo();
// 	    ~Foo();
//	    void doSomething();
// 	};
//
// Any pointer to a Foo that represents ownership should use a Ref,
// which will increment the reference count as long as the pointer
// exists, and then decrement the reference count when the pointer
// goes away.  For example:
//
//	{
//	    Ref<Foo> myPtr = new Foo();
//	    myPtr->doSomething();
//	}
//
// When my pointer goes out of scope at the end of this block, the
// object will be deleted because there are no other references to it.
//
// References to a Foo from other classes should also use a Ref:
//
//	class Bar
//	{
//	public:
//	    Bar() { myFoo = new Foo(); }
//	private:
//	    Ref<Foo> myFoo;
//	};
//
// When an instance of the class Bar is deleted, the reference count
// on myFoo will be decremented, and the object freed if it is the
// last reference.
//
// When to use pointers and when to use Ref:
//
// 1) Use a Ref for class members (as in Bar, above).
//
// 2) Use a Ref for local variables. (Although it's safe to use a
//    pointer if you know that somebody else has a referenc to the
//    object, it's probably worth the cost to use a Ref.)
//
// 3) Use pointers in arguments to functions.  It's more efficient.
//    The caller must keep a reference for the duration of the call.
//    This means that you should not pass a newly created object as an
//    argument to a function; instead do the following:
//
//	{
//	    extern void aFunction( Foo* );
//	    Ref<Foo> x = new Foo();
//	    aFunction( x );
//	}
//
// 4) Return a Ref from a function when you are returning ownership of
//    the object, or if you *might* be returning ownership.
//
// 5) Return a pointer from a function when you are not returning
//    ownership.
//
////////////////////////////////////////////////////////////////////////

template <class Target>
class Ref
{
public:
    //
    // Be default, we point to NULL.
    //

    Ref()
	{
	    myTarget = NULL;
	}

    //
    // These two constructors just acquire ownership of an object by
    // bumping its reference count.  (incRefCount() and decRefCount()
    // handle the case when the pointer is NULL.)
    //

    Ref( Target* target )
	{
	    target->incRefCount();
	    myTarget = target;
	}

    Ref( const Ref& other )
	{
	    other.myTarget->incRefCount();
	    myTarget = other.myTarget;
	}

    //
    // Destroying this reference object drops the reference count.
    //

    ~Ref()
	{
	    myTarget->decRefCount();
	}

    //
    // A function for those rare cases when -> or casting is not right.
    //

    Target* getTarget()
	{
	    return myTarget;
	}

    //
    // Assignment adds a reference.  (The reference to the object we
    // used to have must be decremented second, to handle assignment
    // to self.)
    //

    Ref& operator =( const Ref& other )
	{
	    other.myTarget->incRefCount();
	    myTarget->decRefCount();
	    this->myTarget = other.myTarget;
	    return *this;
	}

    Ref& operator =( Target* target )
	{
	    target->incRefCount();
	    myTarget->decRefCount();
	    this->myTarget = target;
	    return *this;
	}

    //
    // We can cast to a Target*.  This is used when calling a function
    // that takes a Target*.
    //

    operator Target* () const
	{
	    return myTarget;
	}

    //
    // Overloading "->" allows a Ref to be used just like a pointer.
    //

    Target* operator ->() const
	{
	    return myTarget;
	}

private:
    //
    // This is the pointer to the object we are managing.
    //

    Target*	myTarget;
};

#endif // _REF_COUNT_H
