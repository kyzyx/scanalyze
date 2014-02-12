/*

The interface routines for reading and writing PLY polygon files.

- Kari Pulli, April 1998

Modified from PLY library by
- Greg Turk, March 1994

A PLY file contains a single polygonal _object_.

An object is composed of lists of _elements_.
Typical elements are vertices, faces, edges and materials.

Each type of element for a given object has one or more
_properties_ associated with the element type.
For instance, a vertex element may have as properties three
floating-point values x,y,z and three unsigned
chars for red, green and blue.

---------------------------------------------------------------

Copyright (c) 1998 The Board of Trustees of The Leland Stanford
Junior University.  All rights reserved.

Permission to use, copy, modify and distribute this software and
its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice and this permission
notice appear in all copies of this software and that you do not
sell the software.

THE SOFTWARE IS PROVIDED "AS IS" AND WITHOUT WARRANTY OF ANY KIND,
EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.

*/

#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#ifdef linux
#    define __STL_THROW(x) throw x
#    include <stdexcept>
using std::length_error;
#else
#    include <stdexcept.h>
#endif
#include <assert.h>
#include "ply++.h"
#ifndef WIN32
#	include "zlib_fgets.h"
#endif


char *type_names[] = {
"invalid",
"char", "short", "int",
"uchar", "ushort", "uint",
"float", "double",
};

char *new_type_names[] = {
"invalid",
"int8", "int16", "int32",
"uint8", "uint16", "uint32",
"float32", "float64",
};

int ply_type_size[] = {
  0, 1, 2, 4, 1, 2, 4, 4, 8
};

#define NO_OTHER_PROPS  -1

#define DONT_STORE_PROP  0
#define STORE_PROP       1

#define OTHER_PROP       0
#define NAMED_PROP       1

#if defined(WIN32) || defined(i386)
#define NATIVE_ENDIANNES PLY_BINARY_LE
#else
#define NATIVE_ENDIANNES PLY_BINARY_LE
#endif


PlyFile::PlyFile()
  : fp(NULL)
{
  fbuf_size = 1024;
  fbuf = new char[fbuf_size];
  fb_s = fb_e = fbuf;
#ifndef _WIN32
  gzfp = NULL;
#endif
}

PlyFile::~PlyFile()
{
  delete[] fbuf;
  close();
}

// this is to be called to make sure the buffer has
// enough data for the next read(s)
void
PlyFile::fbuf_need(int n)
{
  int d = fb_e - fb_s; // how much is left
  if (d < n) {
    // not enough, get more
    if (n > fbuf_size) {
      // have to allocate more
      char *tmp = new char[n];
      if (d) {
	// there is some unread chars, move them to start
	memcpy(tmp, fb_s, d);
      }
      delete[] fbuf;
      fbuf = tmp;
      fbuf_size = n;
    } else {
      // have enough space
      if (d) {
	// there is some unread chars, move them to start
	memcpy(fbuf, fb_s, d);
      }
    }
#ifdef WIN32
    int i = fread(fbuf + d, 1, fbuf_size - d, fp);
#else
    int i = gzread(gzfp, fbuf+d, fbuf_size-d);
#endif
#ifdef __EXCEPTIONS
    if (!i) {
      __STL_THROW(length_error("ply_file_read"));
    }
#else
    assert(i);
#endif
    fb_s = fbuf;
    fb_e = fb_s + d + i;
  }
}

char
PlyFile::read_char(void)
{
  fbuf_need(1);
  return *(fb_s++);
}

unsigned char
PlyFile::read_uchar(void)
{
  fbuf_need(1);
  return *(fb_s++);
}

short
PlyFile::read_short_BE(void)
{
  fbuf_need(2);
  short s;
  char *p = (char *)&s;
  p[0] = fb_s[0];
  p[1] = fb_s[1];
  fb_s += 2;
  return s;
}

unsigned short
PlyFile::read_ushort_BE(void)
{
  fbuf_need(2);
  unsigned short s;
  char *p = (char *)&s;
  p[0] = fb_s[0];
  p[1] = fb_s[1];
  fb_s += 2;
  return s;
}

int
PlyFile::read_int_BE(void)
{
  fbuf_need(4);
  int s;
  char *p = (char *)&s;
  p[0] = fb_s[0];
  p[1] = fb_s[1];
  p[2] = fb_s[2];
  p[3] = fb_s[3];
  fb_s += 4;
  return s;
}

unsigned int
PlyFile::read_uint_BE(void)
{
  fbuf_need(4);
  unsigned int s;
  char *p = (char *)&s;
  p[0] = fb_s[0];
  p[1] = fb_s[1];
  p[2] = fb_s[2];
  p[3] = fb_s[3];
  fb_s += 4;
  return s;
}

float
PlyFile::read_float_BE(void)
{
  fbuf_need(4);
  float s;
  char *p = (char *)&s;
  p[0] = fb_s[0];
  p[1] = fb_s[1];
  p[2] = fb_s[2];
  p[3] = fb_s[3];
  fb_s += 4;
  return s;
}

double
PlyFile::read_double_BE(void)
{
  fbuf_need(8);
  double s;
  char *p = (char *)&s;
  p[0] = fb_s[0];
  p[1] = fb_s[1];
  p[2] = fb_s[2];
  p[3] = fb_s[3];
  p[4] = fb_s[4];
  p[5] = fb_s[5];
  p[6] = fb_s[6];
  p[7] = fb_s[7];
  fb_s += 8;
  return s;
}

short
PlyFile::read_short_LE(void)
{
  fbuf_need(2);
  short s;
  char *p = (char *)&s;
  p[0] = fb_s[1];
  p[1] = fb_s[0];
  fb_s += 2;
  return s;
}

unsigned short
PlyFile::read_ushort_LE(void)
{
  fbuf_need(2);
  unsigned short s;
  char *p = (char *)&s;
  p[0] = fb_s[1];
  p[1] = fb_s[0];
  fb_s += 2;
  return s;
}

int
PlyFile::read_int_LE(void)
{
  fbuf_need(4);
  int s;
  char *p = (char *)&s;
  p[0] = fb_s[3];
  p[1] = fb_s[2];
  p[2] = fb_s[1];
  p[3] = fb_s[0];
  fb_s += 4;
  return s;
}

unsigned int
PlyFile::read_uint_LE(void)
{
  fbuf_need(4);
  unsigned int s;
  char *p = (char *)&s;
  p[0] = fb_s[3];
  p[1] = fb_s[2];
  p[2] = fb_s[1];
  p[3] = fb_s[0];
  fb_s += 4;
  return s;
}

float
PlyFile::read_float_LE(void)
{
  fbuf_need(4);
  float s;
  char *p = (char *)&s;
  p[0] = fb_s[3];
  p[1] = fb_s[2];
  p[2] = fb_s[1];
  p[3] = fb_s[0];
  fb_s += 4;
  return s;
}

double
PlyFile::read_double_LE(void)
{
  fbuf_need(8);
  double s;
  char *p = (char *)&s;
  p[0] = fb_s[7];
  p[1] = fb_s[6];
  p[2] = fb_s[5];
  p[3] = fb_s[4];
  p[4] = fb_s[3];
  p[5] = fb_s[2];
  p[6] = fb_s[1];
  p[7] = fb_s[0];
  fb_s += 8;
  return s;
}


int
PlyFile::to_int(char *item, int type)
{
  switch (type) {
  case PLY_CHAR:   return *item;
  case PLY_UCHAR:  return *((unsigned char*)item);
  case PLY_SHORT:  return *((short*)item);
  case PLY_USHORT: return *((unsigned short*)item);
  case PLY_INT:    return *((int*)item);
  case PLY_UINT:   return *((unsigned int*)item);
  case PLY_FLOAT:  return *((float*)item);
  case PLY_DOUBLE: return *((double*)item);
  default:         return 0;
  }
}


/*************/
/*  Writing  */
/*************/


/*****************************************************************
Open a polygon file for writing.

Entry:
  filename   - name of file to read from
  nelems     - number of elements in object
  elem_names - list of element names
  file_type  - file type, either ascii or binary

Exit:
  returns 1 if success, 0 if error
******************************************************************/

int
PlyFile::open_for_writing(const char  *filename,
			  int    nelems,
			  char **elem_names,
			  int    file_type)
{
  /* tack on the extension .ply, if necessary */

  char *name = new char[(strlen(filename) + 5)];
  strcpy (name, filename);
  if (strlen (name) < 4 ||
      strcmp (name + strlen (name) - 4, ".ply") != 0)
      strcat (name, ".ply");

  /* open the file for writing */

#ifdef WIN32
  fp = fopen (name, "wb");
#else
  fp = fopen (name, "w");
#endif
  delete[] name;
  if (fp == NULL)
    return 0;

  /* create a record for this object */

  m_file_type  = file_type;
  m_version    = 1.0;
  other_elems  = NULL;

  /* tuck aside the names of the elements */
  for (int i = 0; i < nelems; i++) {
    elems.push_back(PlyElement(elem_names[i]));
  }

  return 1;
}


/*****************************************************************
Open a polygon file for reading.

Entry:
  filename - name of file to read from

Exit:
  nelems     - number of elements in object
  elem_names - list of element names
  returns 1 if success, 0 if error
******************************************************************/

int
PlyFile::open_for_reading(const char   *filename,
			  int    *nelems,
			  char ***elem_names)
{
  char *name = new char[(strlen (filename) + 5)];
  strcpy (name, filename);
#if 0
  /* tack on the extension .ply, if necessary */
  if (strlen (name) < 5 ||
      strcmp (name + strlen (name) - 4, ".ply"))
      strcat (name, ".ply");
#endif

  /* open the file for reading */

#ifdef WIN32
  fp = fopen (name, "rb");
#else
  gzfp = gzopen(name, "rb");
#endif
  delete[] name;
#ifdef WIN32
  if (fp == NULL)
#else
  if (gzfp == NULL)
#endif
    return 0;

  /* create the PlyFile data structure */

  int i,j;
  int nwords;
  char **words;
  char **elist;
  char *orig_line;

  /* create record for this object */

  comments.erase(comments.begin(), comments.end());
  obj_info.erase(obj_info.begin(), obj_info.end());
  other_elems = NULL;

  /* read and parse the file's header */

  words = get_words(&nwords, &orig_line);
  if (!words || !equal_strings (words[0], "ply"))
    return (NULL);

  while (words) {

    /* parse words */

    if (equal_strings (words[0], "format")) {
      if (nwords != 3)
        return (NULL);
      if (equal_strings (words[1], "ascii"))
        m_file_type = PLY_ASCII;
      else if (equal_strings (words[1], "binary_big_endian"))
        m_file_type = PLY_BINARY_BE;
      else if (equal_strings (words[1], "binary_little_endian"))
        m_file_type = PLY_BINARY_LE;
      else
        return 0;
      m_version = atof (words[2]);
    }
    else if (equal_strings (words[0], "element"))
      add_element (words, nwords);
    else if (equal_strings (words[0], "property"))
      add_property (words, nwords);
    else if (equal_strings (words[0], "comment"))
      add_comment (orig_line);
    else if (equal_strings (words[0], "obj_info"))
      add_obj_info (orig_line);
    else if (equal_strings (words[0], "end_header"))
      break;

    delete[] words;
    words = get_words(&nwords, &orig_line);
  }

  /* create tags for each property of each element, to be used */
  /* later to say whether or not to store each property for the user */

  for (i = 0; i < elems.size(); i++) {
    PlyElement &elem = elems[i];
    for (j = 0; j < elem.props.size(); j++)
      elem.store_prop.push_back(DONT_STORE_PROP);
    elem.other_offset = NO_OTHER_PROPS; /* no "other" props by default */
  }

  /* set return values about the elements */

  elist = new char *[elems.size()];
  for (i = 0; i < elems.size(); i++)
    elist[i] = strdup (elems[i].name);

  *elem_names = elist;
  *nelems = elems.size();

  return 1;
}



/*****************************************************************
Describe an element, including its properties and how many will be written
to the file.

Entry:
  elem_name - name of element that information is being specified about
  nelems    - number of elements of this type to be written
  nprops    - number of properties contained in the element
  prop_list - list of properties
******************************************************************/

void
PlyFile::describe_element(char *elem_name,
			  int   nelems,
			  int   nprops,
			  PlyProperty *prop_list)
{
  /* look for appropriate element */
  PlyElement *elem = find_element (elem_name);
  if (elem == NULL) {
    fprintf(stderr,"ply_describe_element: can't find element '%s'\n",elem_name);
    exit (-1);
  }

  elem->num = nelems;

  /* copy the list of properties */
  for (int i = 0; i < nprops; i++) {
    elem->props.push_back(prop_list[i]);
    elem->store_prop.push_back(NAMED_PROP);
  }
}


/*****************************************************************
Describe a property of an element.

Entry:
  elem_name - name of element that information is being specified about
  prop      - the new property
******************************************************************/

void
PlyFile::describe_property(char *elem_name,
			   PlyProperty *prop)
{
  /* look for appropriate element */
  PlyElement *elem = find_element (elem_name);
  if (elem == NULL) {
    fprintf(stderr,
	    "ply_describe_property: can't find element '%s'\n",
            elem_name);
    return;
  }

  /* copy the new property */
  elem->props.push_back(*prop);
  elem->store_prop.push_back(NAMED_PROP);
}

/*****************************************************************
State how many of a given element will be written.

Entry:
  elem_name - name of element that information is being specified about
  nelems    - number of elements of this type to be written
******************************************************************/

void
PlyFile::element_count(char *elem_name,
		       int nelems)
{
  // look for appropriate element
  PlyElement *elem = find_element(elem_name);
  if (elem == NULL) {
    fprintf(stderr,"ply_element_count: can't find element '%s'\n",
	    elem_name);
    exit (-1);
  }

  elem->num = nelems;
}


/*****************************************************************
Pass along a pointer to "other" elements that we want to save in
a given PLY file.  These other elements were presumably read
from another PLY file.

Entry:
  o_elems - info about other elements that we want to store
******************************************************************/

void
PlyFile::describe_other_elements(PlyOtherElems *o_elems)
{
  int i;
  OtherElem *other;

  /* ignore this call if there is no other element */
  if (o_elems == NULL)
    return;

  /* save pointer to this information */
  other_elems = o_elems;

  /* describe the other properties of this element */

  for (i = 0; i < o_elems->list.size(); i++) {
    other = &(o_elems->list[i]);
    element_count (other->elem_name, other->other_data.size());
    describe_other_properties (other->other_props,
			       offsetof(OtherData,other_props));
  }
}


/*****************************************************************
Describe what the "other" properties are that are to be stored, and where
they are in an element.
******************************************************************/

void
PlyFile::describe_other_properties(
  PlyOtherProp *other,
  int offset
)
{
  int i;
  PlyElement *elem;

  /* look for appropriate element */
  elem = find_element (other->name);
  if (elem == NULL) {
    fprintf(stderr, "ply_describe_other_properties: "
	    "can't find element '%s'\n", other->name);
    return;
  }

  /* copy the other properties */

  for (i = 0; i < other->props.size(); i++) {
    elem->props.push_back(other->props[i]);
    elem->store_prop.push_back(OTHER_PROP);
  }

  /* save other info about other properties */
  elem->other_size = other->size;
  elem->other_offset = offset;
}



/*****************************************************************
Signal that we've described everything a PLY file's header and that the
header should be written to the file.
******************************************************************/

void
PlyFile::header_complete(void)
{
  int i,j;

  fprintf (fp, "ply\n");

  switch (m_file_type) {
    case PLY_ASCII:
      fprintf (fp, "format ascii 1.0\n");
      break;
    case PLY_BINARY_BE:
      fprintf (fp, "format binary_big_endian 1.0\n");
      break;
    case PLY_BINARY_LE:
      fprintf (fp, "format binary_little_endian 1.0\n");
      break;
    default:
      fprintf (stderr, "ply_header_complete: bad file type = %d\n",
               m_file_type);
      exit (-1);
  }

  /* write out the comments */

  for (i = 0; i < comments.size(); i++)
    fprintf (fp, "comment %s\n", comments[i]);

  /* write out object information */

  for (i = 0; i < obj_info.size(); i++)
    fprintf (fp, "obj_info %s\n", obj_info[i]);

  /* write out information about each element */

  for (i = 0; i < elems.size(); i++) {

    PlyElement &elem = elems[i];
    fprintf (fp, "element %s %d\n", elem.name, elem.num);

    /* write out each property */
    for (j = 0; j < elem.props.size(); j++) {
      if (elem.props[j].is_list) {
        fprintf (fp, "property list ");
        write_scalar_type (elem.props[j].count_external);
        fprintf (fp, " ");
        write_scalar_type (elem.props[j].external_type);
        fprintf (fp, " %s\n", elem.props[j].name);
      }
      else {
        fprintf (fp, "property ");
        write_scalar_type (elem.props[j].external_type);
        fprintf (fp, " %s\n", elem.props[j].name);
      }
    }
  }

  fprintf (fp, "end_header\n");
}


/*****************************************************************
Specify which elements are going to be written.  This should be called
before a call to the routine ply_put_element().

Entry:
  elem_name - name of element we're talking about
******************************************************************/

void
PlyFile::put_element_setup(char *elem_name)
{
  PlyElement *elem = find_element (elem_name);
  if (elem == NULL) {
    fprintf(stderr, "ply_elements_setup: can't find element '%s'\n", elem_name);
    exit (-1);
  }

  which_elem = elem;
}


/*****************************************************************
Write an element to the file.  This routine assumes that we're
writing the type of element specified in the last call to the routine
ply_put_element_setup().

Entry:
  plyfile  - file identifier
  elem_ptr - pointer to the element
******************************************************************/

void
PlyFile::put_element_static_strg(void *elem_ptr)
{ _put_element(elem_ptr, 1); }

void
PlyFile::put_element(void *elem_ptr)
{ _put_element(elem_ptr, 0); }

void
PlyFile::_put_element(void *elem_ptr, int static_strg)
{
  int j,k;
  char *item;
  int list_count;
  int item_size;
  int int_val;
  unsigned int uint_val;
  double double_val;

  PlyElement *elem = which_elem;
  char *elem_data = (char *) elem_ptr;
  char **other_ptr = (char **)
    (((char *) elem_ptr) + elem->other_offset);

  // write out each property of the element
  for (j = 0; j < elem->props.size(); j++) {
    PlyProperty &prop = elem->props[j];
    if (elem->store_prop[j] == OTHER_PROP)
      elem_data = *other_ptr;
    else
      elem_data = (char *) elem_ptr;
    if (prop.is_list) {
      item = elem_data + prop.count_offset;
      get_stored_item ((void *) item, prop.count_internal,
		       &int_val, &uint_val, &double_val);
      if (m_file_type == PLY_ASCII) {
	write_ascii_item (int_val, uint_val, double_val,
			  prop.count_external);
      } else if (m_file_type == NATIVE_ENDIANNES) {
	write_binary_item_BE (int_val, uint_val, double_val,
			      prop.count_external);
      } else {
	write_binary_item_LE (int_val, uint_val, double_val,
			      prop.count_external);
      };
      list_count = uint_val;
      if (static_strg) {
	item = elem_data + prop.offset;
      } else {
	// get an array of pointers at data + offset
	char **item_ptr = (char **) (elem_data + prop.offset);
	// dereference (==take the only pointer there)
	item = item_ptr[0];
      }

      item_size = ply_type_size[prop.internal_type];
      if (m_file_type == PLY_ASCII) {
	for (k = 0; k < list_count; k++) {
	  get_stored_item ((void *) item, prop.internal_type,
			   &int_val, &uint_val, &double_val);
	  write_ascii_item (int_val, uint_val, double_val,
			    prop.external_type);
	  item += item_size;
	}
      } else if (m_file_type == NATIVE_ENDIANNES) {
	for (k = 0; k < list_count; k++) {
	  get_stored_item ((void *) item, prop.internal_type,
			   &int_val, &uint_val, &double_val);
	  write_binary_item_BE (int_val, uint_val, double_val,
				prop.external_type);
	  item += item_size;
	}
      } else {
	for (k = 0; k < list_count; k++) {
	  get_stored_item ((void *) item, prop.internal_type,
			   &int_val, &uint_val, &double_val);
	  write_binary_item_LE (int_val, uint_val, double_val,
				prop.external_type);
	  item += item_size;
	}
      }
    }
    else {
      item = elem_data + prop.offset;
      get_stored_item ((void *) item, prop.internal_type,
		       &int_val, &uint_val, &double_val);
      if (m_file_type == PLY_ASCII) {
	write_ascii_item (int_val, uint_val, double_val,
			  prop.external_type);
      } else if (m_file_type == NATIVE_ENDIANNES) {
	write_binary_item_BE (int_val, uint_val, double_val,
			      prop.external_type);
      } else {
	write_binary_item_LE (int_val, uint_val, double_val,
			      prop.external_type);
      };
    }
  }

  if (m_file_type == PLY_ASCII) fprintf (fp, "\n");
}


/*****************************************************************
Specify a comment that will be written in the header.

Entry:
  plyfile - file identifier
  comment - the comment to be written
******************************************************************/

void
PlyFile::put_comment(char *comment)
{
  // add comment to list
  comments.push_back (strdup (comment));
}


/*****************************************************************
Specify a piece of object information (arbitrary text) that will be written
in the header.

Entry:
  o_info - the text information to be written
******************************************************************/

void
PlyFile::put_obj_info(char *o_info)
{
  // add info to list
  obj_info.push_back(strdup(o_info));
}




/*************/
/*  Reading  */
/*************/



/*****************************************************************
Get information about a particular element.

Entry:
  elem_name - name of element to get information about

Exit:
  nelems   - number of elements of this type in the file
  nprops   - number of properties
  returns a list of properties, or NULL if the file doesn't contain that elem
******************************************************************/

PlyProperty **
PlyFile::get_element_description(char *elem_name,
				 int  *nelems,
				 int  *nprops)
{
  // find information about the element
  PlyElement *elem = find_element(elem_name);
  if (elem == NULL)
    return NULL;

  *nelems = elem->num;
  *nprops = elem->props.size();

  // make a copy of the element's property list
  PlyProperty **prop_list = new PlyProperty *[elem->props.size()];
  PlyProperty *prop;
  for (int i = 0; i < elem->props.size(); i++) {
    prop = new PlyProperty;
    *prop = elem->props[i];
    prop_list[i] = prop;
  }

  // return this duplicate property list
  return prop_list;
}


/*****************************************************************
Specify which properties of an element are to be returned.
This should be called before a call to the routine
ply_get_element().

Entry:
  elem_name - which element we're talking about
  nprops    - number of properties
  prop_list - list of properties
******************************************************************/

void
PlyFile::get_element_setup(char *elem_name,
			   int   nprops,
			   PlyProperty *prop_list)
{
  PlyProperty *prop;
  int index;

  // find information about the element
  PlyElement *elem = find_element(elem_name);
  which_elem = elem;

  // deposit the property info into the element's description
  for (int i = 0; i < nprops; i++) {

    // look for actual property
    prop = find_property(elem, prop_list[i].name, &index);
    if (prop == NULL) {
      cerr << "Warning:  Can't find property '"
	   << prop_list[i].name << "' in element '"
	   << elem_name << "'" << endl;
      continue;
    }

    // store its description
    prop->internal_type  = prop_list[i].internal_type;
    prop->offset         = prop_list[i].offset;
    prop->count_internal = prop_list[i].count_internal;
    prop->count_offset   = prop_list[i].count_offset;

    // specify that the user wants this property
    elem->store_prop[index] = STORE_PROP;
  }
}


/*****************************************************************
Specify a property of an element that is to be returned.
This should be called (usually multiple times) before a call to
the routine get_element().
This routine should be used in preference to the less flexible
old routine called get_element_setup().

Entry:
  elem_name - which element we're talking about
  prop      - property to add to those that will be returned
******************************************************************/

void
PlyFile::get_property(char *elem_name,
		      PlyProperty *prop)
{
  // find information about the element
  PlyElement *elem = find_element(elem_name);
  which_elem = elem;

  // deposit the property information into the element's description

  int index;
  PlyProperty *prop_ptr = find_property(elem, prop->name, &index);
  if (prop_ptr == NULL) {
    cerr << "Warning:  Can't find property '"
	 << prop->name << "' in element '"
	 << elem_name << "'" << endl;
    return;
  }
  prop_ptr->internal_type  = prop->internal_type;
  prop_ptr->offset         = prop->offset;
  prop_ptr->count_internal = prop->count_internal;
  prop_ptr->count_offset   = prop->count_offset;

  // specify that the user wants this property
  elem->store_prop[index] = STORE_PROP;
}


/*****************************************************************
Read one element from the file.
This routine assumes that we're reading
the type of element specified in the last call to the routine
get_element_setup().

Entry:
  elem_ptr - pointer to location where the element information should be put
******************************************************************/

void
PlyFile::get_element(void *elem_ptr)
{
  if (m_file_type == PLY_ASCII)
    ascii_get_element ((char *) elem_ptr, 0);
  else
    binary_get_element ((char *) elem_ptr, 0);
}


void
PlyFile::get_element_noalloc(void *elem_ptr)
{
  if (m_file_type == PLY_ASCII)
    ascii_get_element ((char *) elem_ptr, 1);
  else
    binary_get_element ((char *) elem_ptr, 1);
}


/*****************************************************************
Extract the comments from the header information of a PLY file.

Entry:

Exit:
  num_comments - number of comments returned
  returns a pointer to a list of comments
******************************************************************/

char **
PlyFile::get_comments(int *n_comments)
{
  *n_comments = comments.size();
  return &comments[0];
}


/*****************************************************************
Extract the object information (arbitrary text) from the header information
of a PLY file.

Entry:

Exit:
  num_obj_info - number of lines of text information returned
  returns a pointer to a list of object info lines
******************************************************************/

char **
PlyFile::get_obj_info(int *n_obj_info)
{
  *n_obj_info = obj_info.size();
  return &obj_info[0];
}


/*****************************************************************

Make ready for "other" properties of an element-- those properties that
the user has not explicitly asked for, but that are to be stashed away
in a special structure to be carried along with the element's other
information.

Entry:
  plyfile - file identifier
  elem    - element for which we want to save away other properties
******************************************************************/

void
setup_other_props(PlyFile *plyfile, PlyElement *elem)
{
  int i;
  int size = 0;
  int type_size;

  /* Examine each property in decreasing order of size. */
  /* We do this so that all data types will be aligned by */
  /* word, half-word, or whatever within the structure. */

  for (type_size = 8; type_size > 0; type_size /= 2) {

    // add up the space taken by each property,
    // and save this information away in the property descriptor

    for (i = 0; i < elem->props.size(); i++) {

      /* don't bother with properties we've been asked to store explicitly */
      if (elem->store_prop[i])
        continue;

      PlyProperty &prop = elem->props[i];

      /* internal types will be same as external */
      prop.internal_type = prop.external_type;
      prop.count_internal = prop.count_external;

      /* check list case */
      if (prop.is_list) {

        /* pointer to list */
        if (type_size == sizeof (void *)) {
          prop.offset = size;
          size += sizeof (void *);    /* always use size of a pointer here */
        }

        /* count of number of list elements */
        if (type_size == ply_type_size[prop.count_external]) {
          prop.count_offset = size;
          size += ply_type_size[prop.count_external];
        }
      }
      /* not list */
      else if (type_size == ply_type_size[prop.external_type]) {
        prop.offset = size;
        size += ply_type_size[prop.external_type];
      }
    }

  }

  /* save the size for the other_props structure */
  elem->other_size = size;
}


/*****************************************************************
Specify that we want the "other" properties of an element to be
tucked away within the user's structure.
The user needn't be concerned for how these properties are stored.

Entry:
  elem_name - name of element that we want to store other_props in
  offset    - offset to where other_props will be stored inside user's structure

Exit:
  returns pointer to structure containing description of other_props
******************************************************************/

PlyOtherProp *
PlyFile::get_other_properties(char *elem_name,
			      int   offset)
{
  int i;
  PlyElement *elem;
  PlyOtherProp *other;

  /* find information about the element */
  elem = find_element (elem_name);
  if (elem == NULL) {
    fprintf (stderr, "ply_get_other_properties: Can't find element '%s'\n",
             elem_name);
    return (NULL);
  }

  /* remember that this is the "current" element */
  which_elem = elem;

  /* save the offset to where to store the other_props */
  elem->other_offset = offset;

  /* place the appropriate pointers, etc. in the element's property list */
  setup_other_props (this, elem);

  /* create structure for describing other_props */
  other = new PlyOtherProp;
  other->name = strdup (elem_name);
  other->size = elem->other_size;

  /* save descriptions of each "other" property */
  for (i = 0; i < elem->props.size(); i++) {
    if (elem->store_prop[i])
      continue;
    other->props.push_back(elem->props[i]);
  }

  // set other_offset pointer appropriately if there are
  // NO other properties
  if (other->props.size() == 0) {
    elem->other_offset = NO_OTHER_PROPS;
  }

  /* return structure */
  return other;
}




/*************************/
/*  Other Element Stuff  */
/*************************/




/*****************************************************************
Grab all the data for an element that a user does not want to explicitly
read in.

Entry:
  elem_name  - name of element whose data is to be read in
  elem_count - number of instances of this element stored in the file

Exit:
  returns pointer to ALL the "other" element data for this PLY file
******************************************************************/

PlyOtherElems *
PlyFile::get_other_element(char *elem_name,
			   int   elem_count)
{
  int i;
  PlyElement *elem;
  PlyOtherElems *o_elems;
  OtherElem *other;

  /* look for appropriate element */
  elem = find_element (elem_name);
  if (elem == NULL) {
    fprintf (stderr,
             "ply_get_other_element: can't find element '%s'\n",
	     elem_name);
    exit (-1);
  }

  /* create room for the new "other" element, initializing the */
  /* other data structure if necessary */

  if (other_elems == NULL) {
    other_elems = new PlyOtherElems;
  }
  o_elems = other_elems;
  o_elems->list.push_back(OtherElem());
  other = &(o_elems->list.back());

  /* save name of element */
  other->elem_name = strdup (elem_name);

  /* set up for getting elements */
  other->other_props = get_other_properties(elem_name,
					    offsetof(OtherData,
						     other_props));

  /* grab all these elements */
  for (i = 0; i < elem_count; i++) {
    /* grab and element from the file */
    other->other_data.push_back(new OtherData);
    get_element ((void *) other->other_data[i]);
  }

  /* return pointer to the other elements data */
  return o_elems;
}


/*****************************************************************
Write out the "other" elements specified for this PLY file.

Entry:
******************************************************************/

void
PlyFile::put_other_elements (void)
{
  int i,j;
  OtherElem *other;

  /* make sure we have other elements to write */
  if (other_elems == NULL)
    return;

  /* write out the data for each "other" element */

  for (i = 0; i < other_elems->list.size(); i++) {

    other = &(other_elems->list[i]);
    put_element_setup (other->elem_name);

    /* write out each instance of the current element */
    for (j = 0; j < other->other_data.size(); j++)
      put_element ((void *) other->other_data[j]);
  }
}


/*******************/
/*  Miscellaneous  */
/*******************/



/*****************************************************************
Close a PLY file.

Entry:
******************************************************************/

void
PlyFile::close(void)
{
  if (fp) {
    fclose (fp);
    fp = NULL;
    fb_s = fb_e = fbuf;
  }
#ifndef WIN32
  if (gzfp) {
    gzclose(gzfp);
    gzfp = NULL;
  }
#endif
}


/*****************************************************************
Compare two strings.  Returns 1 if they are the same, 0 if not.
******************************************************************/

int
equal_strings(char *s1, char *s2)
{
  while (*s1 && *s2)
    if (*s1++ != *s2++)
      return (0);

  if (*s1 != *s2)
    return (0);
  else
    return (1);
}


/*****************************************************************
Find an element from the element list

Entry:
  element - name of element we're looking for

Exit:
  returns the element, or NULL if not found
******************************************************************/
PlyElement *
PlyFile::find_element(char *element)
{
  for (int i = 0; i < elems.size(); i++)
    if (equal_strings (element, elems[i].name))
      return &elems[i];

  return NULL;
}


/*****************************************************************
Find a property in the list of properties of a given element.

Entry:
  elem      - pointer to element in which we want to find the property
  prop_name - name of property to find

Exit:
  index - index to position in list
  returns a pointer to the property, or NULL if not found
******************************************************************/

PlyProperty *
PlyFile::find_property(PlyElement *elem,
		       char       *prop_name,
		       int        *index)
{
  for (int i = 0; i < elem->props.size(); i++)
    if (equal_strings (prop_name, elem->props[i].name)) {
      *index = i;
      return &elem->props[i];
    }

  *index = -1;
  return NULL;
}


/*****************************************************************
Read an element from an ascii file.

Entry:
  elem_ptr - pointer to element
******************************************************************/

void
PlyFile::ascii_get_element(char *elem_ptr,
			   int use_provided_storage)
{
  int j,k;
  int which_word;
  char *elem_data,*item;
  int int_val;
  unsigned int uint_val;
  double double_val;
  int list_count;
  int store_it;
  char **store_array;
  char *orig_line;
  char *other_data;
  int other_flag;

  /* the kind of element we're reading currently */
  PlyElement *elem = which_elem;

  /* do we need to setup for other_props? */

  if (elem->other_offset != NO_OTHER_PROPS) {
    char **ptr;
    other_flag = 1;
    /* make room for other_props */
    other_data = new char[elem->other_size];
    /* store pointer in user's structure to the other_props */
    ptr = (char **) (elem_ptr + elem->other_offset);
    *ptr = other_data;
  }
  else
    other_flag = 0;

  /* read in the element */

  int nwords;
  char **words = get_words(&nwords, &orig_line);
  if (words == NULL) {
    fprintf (stderr, "ply_get_element: unexpected end of file\n");
    exit (-1);
  }

  which_word = 0;

  for (j = 0; j < elem->props.size(); j++) {

    PlyProperty &prop = elem->props[j];
    store_it = (elem->store_prop[j] | other_flag);

    /* store either in the user's structure or in other_props */
    if (elem->store_prop[j])
      elem_data = elem_ptr;
    else
      elem_data = other_data;

    if (prop.is_list) {       /* a list */

      /* get and store the number of items in the list */
      get_ascii_item (words[which_word++], prop.count_external,
                      &int_val, &uint_val, &double_val);
      if (store_it) {
        item = elem_data + prop.count_offset;
        store_item(item, prop.count_internal, int_val, uint_val,
		   double_val);
      }

      /* allocate space for an array of items and store a ptr to the array */
      list_count = int_val;
      int item_size = ply_type_size[prop.internal_type];
      store_array = (char **) (elem_data + prop.offset);

      if (list_count == 0) {
        if (store_it)
          *store_array = NULL;
      }
      else {
        if (store_it) {
	  if (use_provided_storage) {
	    item = elem_data + prop.offset;
	  } else {
	    item = new char[item_size * list_count];
	    *store_array = item;
	  }
        }

        /* read items and store them into the array */
        for (k = 0; k < list_count; k++) {
          get_ascii_item (words[which_word++], prop.external_type,
                          &int_val, &uint_val, &double_val);
          if (store_it) {
            store_item (item, prop.internal_type,
                        int_val, uint_val, double_val);
            item += item_size;
          }
        }
      }

    }
    else {                     /* not a list */
      get_ascii_item (words[which_word++], prop.external_type,
                      &int_val, &uint_val, &double_val);
      if (store_it) {
        item = elem_data + prop.offset;
        store_item (item, prop.internal_type, int_val, uint_val, double_val);
      }
    }

  }

  delete[] words;
}

static void
inv_memcpy(char *to, const char *from, int size, int n)
{
  for (int i=0; i<n; i++) {
    int off_to   = size*i;
    int off_from = off_to + size - 1;
    for (int j=0; j<size; j++) {
      to[off_to+j] = from[off_from-j];
    }
  }
}

void
PlyFile::get_and_store_binary_item(int type_ext, int type_int,
				   char *item)
{
  if (type_ext == type_int) {
    // external type and internal type is the same, short
    // circuit
    fbuf_need(ply_type_size[type_ext]);
    if (m_file_type == NATIVE_ENDIANNES) {
      memcpy(item, fb_s, ply_type_size[type_ext]);
    } else {
      inv_memcpy(item, fb_s, ply_type_size[type_ext], 1);
    }
    fb_s += ply_type_size[type_ext];
  } else {
    // the general case
    int          int_val;
    unsigned int uint_val;
    double       double_val;
    if (m_file_type == NATIVE_ENDIANNES) {
      get_binary_item_BE(type_ext,
			 &int_val, &uint_val, &double_val);
    } else {
      get_binary_item_LE(type_ext,
			 &int_val, &uint_val, &double_val);
    }
    store_item(item, type_int, int_val, uint_val, double_val);
  }
}

void
PlyFile::get_and_store_binary_list(int type_ext, int type_int,
				   char *item, int n)
{
  if (type_ext == type_int) {
    // external type and internal type is the same, short
    // circuit
    int _n = n*ply_type_size[type_ext];
    fbuf_need(_n);
    if (m_file_type == NATIVE_ENDIANNES) {
      memcpy(item, fb_s, _n);
    } else {
      inv_memcpy(item, fb_s, ply_type_size[type_ext], n);
    }
    fb_s += _n;
  } else {
    // the general case
    int          int_val;
    unsigned int uint_val;
    double       double_val;
    int          step = ply_type_size[type_int];
    if (m_file_type == NATIVE_ENDIANNES) {
      for (int i=0; i<n; i++, item += step) {
	get_binary_item_BE(type_ext,
			   &int_val, &uint_val, &double_val);
	store_item(item, type_int, int_val, uint_val, double_val);
      }
    } else {
      for (int i=0; i<n; i++, item += step) {
	get_binary_item_LE(type_ext,
			   &int_val, &uint_val, &double_val);
	store_item(item, type_int, int_val, uint_val, double_val);
      }
    }
  }
}


/*****************************************************************
Read an element from a binary file.

Entry:
  elem_ptr - pointer to an element
******************************************************************/

void
PlyFile::binary_get_element(char *elem_ptr,
			    int use_provided_storage)
{
  int j,k;
  char *elem_data,*item;
  int int_val;
  unsigned int uint_val;
  double double_val;
  int list_count;
  char *other_data;
  int other_flag;

  // the kind of element we're reading currently
  PlyElement *elem = which_elem;

  // do we need to setup for other_props?

  if (elem->other_offset != NO_OTHER_PROPS) {
    other_flag = 1;
    // make room for other_props
    other_data = new char[elem->other_size];
    // store pointer in user's structure to the other_props
    char **ptr = (char **) (elem_ptr + elem->other_offset);
    *ptr = other_data;
  } else {
    other_flag = 0;
  }

  // read in a number of elements

  for (j = 0; j < elem->props.size(); j++) {

    PlyProperty &prop = elem->props[j];

    if (elem->store_prop[j] | other_flag) {
      // read and store data

      // store either in the user's structure or in other_props
      if (elem->store_prop[j])  elem_data = elem_ptr;
      else                      elem_data = other_data;

      if (prop.is_list) {       // a list

	// get and store the number of items in the list
	item = elem_data + prop.count_offset;
	get_and_store_binary_item(prop.count_external,
				  prop.count_internal,
				  item);
	list_count = to_int(item, prop.count_internal);
	if (list_count) {
	  // read items and store them into the array
	  if (!use_provided_storage) {
	    int item_size = ply_type_size[prop.internal_type];
	    char **tmp = (char **) (elem_data + prop.offset);
	    *tmp = item = new char[item_size * list_count];
	  } else {
	    item = elem_data + prop.offset;
	  }
	  get_and_store_binary_list(prop.external_type,
				    prop.internal_type,
				    item,
				    list_count);
	}
      } else {                     // not a list
	get_and_store_binary_item(prop.external_type,
				  prop.internal_type,
				  elem_data + prop.offset);
      }

    } else {
      // just read, don't store

      if (prop.is_list) {       // a list

	// get the number of items in the list
	get_binary_item_BE(prop.count_external,
			   &list_count, &uint_val, &double_val);

	if (list_count) {
	  // read items and store them into the array
	  for (k = 0; k < list_count; k++) {
	    get_binary_item_BE(prop.external_type,
			       &int_val, &uint_val, &double_val);
	  }
	}
      } else {                     // not a list
	get_binary_item_BE(prop.external_type,
			   &int_val, &uint_val, &double_val);
      }

    }
  }
}


/*****************************************************************
Write to a file the word that represents a PLY data type.

Entry:
  code - code for type
******************************************************************/

void
PlyFile::write_scalar_type (int code)
{
  /* make sure this is a valid code */

  if (code <= PLY_START_TYPE || code >= PLY_END_TYPE) {
    fprintf (stderr, "write_scalar_type: bad data code = %d\n",
	     code);
    exit (-1);
  }

  /* write the code to a file */

  fprintf (fp, "%s", type_names[code]);
}


/*****************************************************************
Get a text line from a file and break it up into words.

IMPORTANT: The calling routine call delete[] on the returned
pointer once finished with it.

Entry:

Exit:
  nwords    - number of words returned
  orig_line - the original line of characters
  returns a list of words from the line, or NULL if end-of-file
******************************************************************/

char **
PlyFile::get_words(int *nwords, char **orig_line)
{
#define BIG_STRING 4096
  static char str[BIG_STRING];
  static char str_copy[BIG_STRING];
  int max_words = 10;
  int num_words = 0;
  char *ptr,*ptr2;
  char *result;

  char **words = new char *[max_words];

  /* read in a line */
#ifdef WIN32
  result = fgets (str, BIG_STRING, fp);
#else
  result = gzgets (str, BIG_STRING, gzfp);
#endif
  if (result == NULL) {
    *nwords = 0;
    *orig_line = NULL;
    return NULL;
  }

  /* convert line-feed and tabs into spaces */
  /* (this guarentees that there will be a space before the */
  /*  null character at the end of the string) */

  str[BIG_STRING-2] = ' ';
  str[BIG_STRING-1] = '\0';

  for (ptr = str, ptr2 = str_copy; *ptr != '\0'; ptr++, ptr2++) {
    *ptr2 = *ptr;
    if (*ptr == '\t') {
      *ptr = ' ';
      *ptr2 = ' ';
    }
  #ifdef _WIN32	// reading ascii file in binary mode: handle \r\n
    else if (*ptr == '\r') {
      *ptr = ' ';
      *ptr2 = '\0';
    }
  #endif
    else if (*ptr == '\n') {
      *ptr = ' ';
      *ptr2 = '\0';
      break;
    }
  }

  /* find the words in the line */

  ptr = str;
  while (*ptr != '\0') {

    /* jump over leading spaces */
    while (*ptr == ' ')
      ptr++;

    /* break if we reach the end */
    if (*ptr == '\0')
      break;

    /* save pointer to beginning of word */
    if (num_words >= max_words) {
      max_words += 10;
      delete[] words;
      words = new char *[max_words];
    }
    words[num_words++] = ptr;

    /* jump over non-spaces */
    while (*ptr != ' ')
      ptr++;

    /* place a null character here to mark the end of the word */
    *ptr++ = '\0';
  }

  /* return the list of words */
  *nwords = num_words;
  *orig_line = str_copy;
  return words;
}


/*****************************************************************
Return the value of an item, given a pointer to it and its type.

Entry:
  item - pointer to item
  type - data type that "item" points to

Exit:
  returns a double-precision float that contains the value of the item
******************************************************************/

double
PlyFile::get_item_value(char *item, int type)
{
  unsigned char *puchar;
  char *pchar;
  short int *pshort;
  unsigned short int *pushort;
  int *pint;
  unsigned int *puint;
  float *pfloat;
  double *pdouble;
  int int_value;
  unsigned int uint_value;
  double double_value;

  switch (type) {
    case PLY_CHAR:
      pchar = (char *) item;
      int_value = *pchar;
      return ((double) int_value);
    case PLY_UCHAR:
      puchar = (unsigned char *) item;
      int_value = *puchar;
      return ((double) int_value);
    case PLY_SHORT:
      pshort = (short int *) item;
      int_value = *pshort;
      return ((double) int_value);
    case PLY_USHORT:
      pushort = (unsigned short int *) item;
      int_value = *pushort;
      return ((double) int_value);
    case PLY_INT:
      pint = (int *) item;
      int_value = *pint;
      return ((double) int_value);
    case PLY_UINT:
      puint = (unsigned int *) item;
      uint_value = *puint;
      return ((double) uint_value);
    case PLY_FLOAT:
      pfloat = (float *) item;
      double_value = *pfloat;
      return (double_value);
    case PLY_DOUBLE:
      pdouble = (double *) item;
      double_value = *pdouble;
      return (double_value);
    default:
      fprintf (stderr, "get_item_value: bad type = %d\n", type);
      exit (-1);
  }
  return 0.0; // remove warning (never gets here)
}


/*****************************************************************
Write out an item to a file as raw binary bytes.

Entry:
  int_val    - integer version of item
  uint_val   - unsigned integer version of item
  double_val - double-precision float version of item
  type       - data type to write out
******************************************************************/

void
PlyFile::write_binary_item_BE(int int_val,
			      unsigned int uint_val,
			      double double_val,
			      int type)
{
  unsigned char uchar_val;
  char char_val;
  unsigned short ushort_val;
  short short_val;
  float float_val;

  switch (type) {
    case PLY_CHAR:
      char_val = int_val;
      fwrite (&char_val, 1, 1, fp);
      break;
    case PLY_SHORT:
      short_val = int_val;
      fwrite (&short_val, 2, 1, fp);
      break;
    case PLY_INT:
      fwrite (&int_val, 4, 1, fp);
      break;
    case PLY_UCHAR:
      uchar_val = uint_val;
      fwrite (&uchar_val, 1, 1, fp);
      break;
    case PLY_USHORT:
      ushort_val = uint_val;
      fwrite (&ushort_val, 2, 1, fp);
      break;
    case PLY_UINT:
      fwrite (&uint_val, 4, 1, fp);
      break;
    case PLY_FLOAT:
      float_val = double_val;
      fwrite (&float_val, 4, 1, fp);
      break;
    case PLY_DOUBLE:
      fwrite (&double_val, 8, 1, fp);
      break;
    default:
      cerr << "write_binary_item_BE: bad type = "
	   << type << endl;
      exit (-1);
  }
}

static void
swap(short i, char *c)
{
  char *p = (char *)&i;
  c[0] = p[1];
  c[1] = p[2];
}

static void
swap(unsigned short i, char *c)
{
  char *p = (char *)&i;
  c[0] = p[1];
  c[1] = p[2];
}

static void
swap(int i, char *c)
{
  char *p = (char *)&i;
  c[0] = p[3];
  c[1] = p[2];
  c[2] = p[1];
  c[3] = p[0];
}

static void
swap(unsigned int i, char *c)
{
  char *p = (char *)&i;
  c[0] = p[3];
  c[1] = p[2];
  c[2] = p[1];
  c[3] = p[0];
}

static void
swap(float i, char *c)
{
  char *p = (char *)&i;
  c[0] = p[3];
  c[1] = p[2];
  c[2] = p[1];
  c[3] = p[0];
}

static void
swap(double i, char *c)
{
  char *p = (char *)&i;
  c[0] = p[7];
  c[1] = p[6];
  c[2] = p[5];
  c[3] = p[4];
  c[4] = p[3];
  c[5] = p[2];
  c[6] = p[1];
  c[7] = p[0];
}

void
PlyFile::write_binary_item_LE(int int_val,
			      unsigned int uint_val,
			      double double_val,
			      int type)
{
  unsigned char uchar_val;
  char char_val;
  char c[8];

  switch (type) {
    case PLY_CHAR:
      char_val = int_val;
      fwrite (&char_val, 1, 1, fp);
      break;
    case PLY_SHORT:
      swap(short(int_val), c);
      fwrite (c, 1, 2, fp);
      break;
    case PLY_INT:
      swap(int_val, c);
      fwrite (c, 1, 4, fp);
      break;
    case PLY_UCHAR:
      uchar_val = uint_val;
      fwrite (&uchar_val, 1, 1, fp);
      break;
    case PLY_USHORT:
      swap((unsigned short)int_val, c);
      fwrite (c, 1, 2, fp);
      break;
    case PLY_UINT:
      swap(uint_val, c);
      fwrite (c, 1, 4, fp);
      break;
    case PLY_FLOAT:
      swap(float(double_val), c);
      fwrite (c, 1, 4, fp);
      break;
    case PLY_DOUBLE:
      swap(double_val, c);
      fwrite (c, 1, 8, fp);
      break;
    default:
      cerr << "write_binary_item_LE: bad type = "
	   << type << endl;
      exit (-1);
  }
}


/*****************************************************************
Write out an item to a file as ascii characters.

Entry:
  int_val    - integer version of item
  uint_val   - unsigned integer version of item
  double_val - double-precision float version of item
  type       - data type to write out
******************************************************************/

void
PlyFile::write_ascii_item(
  int int_val,
  unsigned int uint_val,
  double double_val,
  int type
)
{
  switch (type) {
    case PLY_CHAR:
    case PLY_SHORT:
    case PLY_INT:
      fprintf (fp, "%d ", int_val);
      break;
    case PLY_UCHAR:
    case PLY_USHORT:
    case PLY_UINT:
      fprintf (fp, "%u ", uint_val);
      break;
    case PLY_FLOAT:
    case PLY_DOUBLE:
      fprintf (fp, "%g ", double_val);
      break;
    default:
      fprintf (stderr, "write_ascii_item: bad type = %d\n", type);
      exit (-1);
  }
}


/*****************************************************************
Get the value of an item that is in memory, and place the result
into an integer, an unsigned integer and a double.

Entry:
  ptr  - pointer to the item
  type - data type supposedly in the item

Exit:
  int_val    - integer value
  uint_val   - unsigned integer value
  double_val - double-precision floating point value
******************************************************************/

void
PlyFile::get_stored_item(void *ptr,
			 int type,
			 int *int_val,
			 unsigned int *uint_val,
			 double *double_val)
{
  switch (type) {
    case PLY_CHAR:
      *int_val = *((char *) ptr);
      *uint_val = *int_val;
      *double_val = *int_val;
      break;
    case PLY_UCHAR:
      *uint_val = *((unsigned char *) ptr);
      *int_val = *uint_val;
      *double_val = *uint_val;
      break;
    case PLY_SHORT:
      *int_val = *((short int *) ptr);
      *uint_val = *int_val;
      *double_val = *int_val;
      break;
    case PLY_USHORT:
      *uint_val = *((unsigned short int *) ptr);
      *int_val = *uint_val;
      *double_val = *uint_val;
      break;
    case PLY_INT:
      *int_val = *((int *) ptr);
      *uint_val = *int_val;
      *double_val = *int_val;
      break;
    case PLY_UINT:
      *uint_val = *((unsigned int *) ptr);
      *int_val = *uint_val;
      *double_val = *uint_val;
      break;
    case PLY_FLOAT:
      *double_val = *((float *) ptr);
      *int_val = *double_val;
      *uint_val = *double_val;
      break;
    case PLY_DOUBLE:
      *double_val = *((double *) ptr);
      *int_val = *double_val;
      *uint_val = *double_val;
      break;
    default:
      fprintf (stderr, "get_stored_item: bad type = %d\n", type);
      exit (-1);
  }
}


/*****************************************************************
Get the value of an item from a binary file, and place the result
into an integer, an unsigned integer and a double.

Entry:
  type - data type supposedly in the word

Exit:
  int_val    - integer value
  uint_val   - unsigned integer value
  double_val - double-precision floating point value
******************************************************************/

void
PlyFile::get_binary_item_BE(int type,
			    int *int_val,
			    unsigned int *uint_val,
			    double *double_val)
{
  switch (type) {
    case PLY_CHAR:
      *int_val = read_char();
      *double_val = *uint_val = *int_val;
      break;
    case PLY_UCHAR:
      *uint_val = read_uchar();
      *double_val = *int_val = *uint_val;
      break;
    case PLY_SHORT:
      *int_val = read_short_BE();
      *double_val = *uint_val = *int_val;
      break;
    case PLY_USHORT:
      *uint_val = read_ushort_BE();
      *double_val = *int_val = *uint_val;
      break;
    case PLY_INT:
      *int_val = read_int_BE();
      *double_val = *uint_val = *int_val;
      break;
    case PLY_UINT:
      *uint_val = read_uint_BE();
      *double_val = *int_val = *uint_val;
      break;
    case PLY_FLOAT:
      *double_val = read_float_BE();
      *uint_val = *int_val = *double_val;
      break;
    case PLY_DOUBLE:
      *double_val = read_double_BE();
      *uint_val = *int_val = *double_val;
      break;
    default:
      fprintf (stderr, "get_binary_item: bad type = %d\n", type);
      exit (-1);
  }
}

void
PlyFile::get_binary_item_LE(int type,
			    int *int_val,
			    unsigned int *uint_val,
			    double *double_val)
{
  switch (type) {
    case PLY_CHAR:
      *int_val = read_char();
      *double_val = *uint_val = *int_val;
      break;
    case PLY_UCHAR:
      *uint_val = read_uchar();
      *double_val = *int_val = *uint_val;
      break;
    case PLY_SHORT:
      *int_val = read_short_LE();
      *double_val = *uint_val = *int_val;
      break;
    case PLY_USHORT:
      *uint_val = read_ushort_LE();
      *double_val = *int_val = *uint_val;
      break;
    case PLY_INT:
      *int_val = read_int_LE();
      *double_val = *uint_val = *int_val;
      break;
    case PLY_UINT:
      *uint_val = read_uint_LE();
      *double_val = *int_val = *uint_val;
      break;
    case PLY_FLOAT:
      *double_val = read_float_LE();
      *uint_val = *int_val = *double_val;
      break;
    case PLY_DOUBLE:
      *double_val = read_double_LE();
      *uint_val = *int_val = *double_val;
      break;
    default:
      fprintf (stderr, "get_binary_item: bad type = %d\n", type);
      exit (-1);
  }
}


/*****************************************************************
Extract the value of an item from an ascii word, and place the result
into an integer, an unsigned integer and a double.

Entry:
  word - word to extract value from
  type - data type supposedly in the word

Exit:
  int_val    - integer value
  uint_val   - unsigned integer value
  double_val - double-precision floating point value
******************************************************************/

void
PlyFile::get_ascii_item(char *word,
			int type,
			int *int_val,
			unsigned int *uint_val,
			double *double_val)
{
  switch (type) {
    case PLY_CHAR:
    case PLY_UCHAR:
    case PLY_SHORT:
    case PLY_USHORT:
    case PLY_INT:
      *int_val = atoi (word);
      *uint_val = *int_val;
      *double_val = *int_val;
      break;

    case PLY_UINT:
      *uint_val = strtoul (word, (char **) NULL, 10);
      *int_val = *uint_val;
      *double_val = *uint_val;
      break;

    case PLY_FLOAT:
    case PLY_DOUBLE:
      *double_val = atof (word);
      *int_val = (int) *double_val;
      *uint_val = (unsigned int) *double_val;
      break;

    default:
      fprintf (stderr, "get_ascii_item: bad type = %d\n", type);
      exit (-1);
  }
}


/*****************************************************************
Store a value into a place being pointed to, guided by a data type.

Entry:
  item       - place to store value
  type       - data type
  int_val    - integer version of value
  uint_val   - unsigned integer version of value
  double_val - double version of value

Exit:
  item - pointer to stored value
******************************************************************/

void
PlyFile::store_item(char *item,
		    int type,
		    int int_val,
		    unsigned int uint_val,
		    double double_val)
{
  unsigned char *puchar;
  short int *pshort;
  unsigned short int *pushort;
  int *pint;
  unsigned int *puint;
  float *pfloat;
  double *pdouble;

  switch (type) {
    case PLY_CHAR:
      *item = int_val;
      break;
    case PLY_UCHAR:
      puchar = (unsigned char *) item;
      *puchar = uint_val;
      break;
    case PLY_SHORT:
      pshort = (short *) item;
      *pshort = int_val;
      break;
    case PLY_USHORT:
      pushort = (unsigned short *) item;
      *pushort = uint_val;
      break;
    case PLY_INT:
      pint = (int *) item;
      *pint = int_val;
      break;
    case PLY_UINT:
      puint = (unsigned int *) item;
      *puint = uint_val;
      break;
    case PLY_FLOAT:
      pfloat = (float *) item;
      *pfloat = double_val;
      break;
    case PLY_DOUBLE:
      pdouble = (double *) item;
      *pdouble = double_val;
      break;
    default:
      fprintf (stderr, "store_item: bad type = %d\n", type);
      exit (-1);
  }
}


/*****************************************************************
Add an element to a PLY file descriptor.

Entry:
  words   - list of words describing the element
  nwords  - number of words in the list
******************************************************************/

void
PlyFile::add_element (char **words, int nwords)
{
  /* create the new element */
  PlyElement elem(words[1]);
  elem.num = atoi (words[2]);

  /* add the new element to the object's list */
  elems.push_back(elem);
}


/*****************************************************************
Return the type of a property, given the name of the property.

Entry:
  name - name of property type

Exit:
  returns integer code for property, or 0 if not found
******************************************************************/

int
get_prop_type(char *type_name)
{
  int i;

  for (i = PLY_START_TYPE + 1; i < PLY_END_TYPE; i++)
    if (equal_strings (type_name, type_names[i]))
      return i;

  for (i = PLY_START_TYPE + 1; i < PLY_END_TYPE; i++)
    if (equal_strings (type_name, new_type_names[i]))
      return i;

  /* if we get here, we didn't find the type */
  return 0;
}


/*****************************************************************
Add a property to a PLY file descriptor.

Entry:
  words   - list of words describing the property
  nwords  - number of words in the list
******************************************************************/

void
PlyFile::add_property(char **words, int nwords)
{
  PlyProperty prop;

  if (equal_strings (words[1], "list")) {       /* is a list */
    prop.count_external = get_prop_type (words[2]);
    prop.external_type = get_prop_type (words[3]);
    prop.name = strdup (words[4]);
    prop.is_list = 1;
  }
  else {                                        /* not a list */
    prop.external_type = get_prop_type (words[1]);
    prop.name = strdup(words[2]);
    prop.is_list = 0;
  }

  // add this property to the list of properties of the
  // current element

  elems.back().props.push_back(prop);
}


/*****************************************************************
Add a comment to a PLY file descriptor.

Entry:
  line    - line containing comment
******************************************************************/

void
PlyFile::add_comment(char *line)
{
  int i;

  /* skip over "comment" and leading spaces and tabs */
  i = 7;
  while (line[i] == ' ' || line[i] == '\t')
    i++;

  put_comment (&line[i]);
}


/*****************************************************************
Add a some object information to a PLY file descriptor.

Entry:
  line    - line containing text info
******************************************************************/

void
PlyFile::add_obj_info(char *line)
{
  int i;

  /* skip over "obj_info" and leading spaces and tabs */
  i = 8;
  while (line[i] == ' ' || line[i] == '\t')
    i++;

  put_obj_info (&line[i]);
}


/*****************************************************************
Determine whether a property currently exists for an element.

Entry:
  elem_name - name of element that information is being
              specified about
  prop_name - name of property to find
******************************************************************/

int
PlyFile::is_valid_property(char *elem_name,
			   char *prop_name)
{
  // find information about the element
  PlyElement *elem = find_element (elem_name);
  which_elem = elem;
  int index;
  return (NULL != find_property(elem, prop_name, &index));
}


int
isPlyFile(char *filename)
{
  int nelems;
  char **elist;
  PlyFile ply;
  return ply.open_for_reading(filename, &nelems, &elist);
}
