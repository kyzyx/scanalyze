/*

Header for PLY polygon files.

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

#ifndef __PLYPLUSPLUS_H__
#define __PLYPLUSPLUS_H__

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#ifndef WIN32
#include <zlib.h>
#endif

#include <vector.h>

#define PLY_ASCII      1        // ascii PLY file
#define PLY_BINARY_BE  2        // binary PLY file, big endian
#define PLY_BINARY_LE  3        // binary PLY file, little endian

#define PLY_OKAY    0           /* ply routine worked okay */
#define PLY_ERROR  -1           /* error in ply routine */

/* scalar data types supported by PLY format */

#define PLY_START_TYPE 0
#define PLY_CHAR       1
#define PLY_SHORT      2
#define PLY_INT        3
#define PLY_UCHAR      4
#define PLY_USHORT     5
#define PLY_UINT       6
#define PLY_FLOAT      7
#define PLY_DOUBLE     8
#define PLY_END_TYPE   9

#define  PLY_SCALAR  0
#define  PLY_LIST    1


struct PlyProperty {    /* description of a property */

  char *name;                   // property name
  int external_type;            // file's data type
  int internal_type;            // program's data type
  int offset;                   // offset bytes of prop in a struct

  int is_list;                  // 1 = list, 0 = scalar
  int count_external;           // file's count type
  int count_internal;           // program's count type
  int count_offset;             // offset byte for list count

  /*
  PlyProperty(void) {}

  PlyProperty(char *n, int et, int it, int o, int l,
	      int ce, int ci, int co)
    {
      name           = strdup(n);
      external_type  = et;
      internal_type  = it;
      offset         = o;
      is_list        = l;
      count_external = ce;
      count_internal = ci;
      count_offset   = co;
    }
  */

  PlyProperty &operator=(const PlyProperty &p)
    {
      name           = strdup(p.name);
      external_type  = p.external_type;
      internal_type  = p.internal_type;
      offset         = p.offset;
      is_list        = p.is_list;
      count_external = p.count_external;
      count_internal = p.count_internal;
      count_offset   = p.count_offset;
      return *this;
    }
};

struct PlyElement {     /* description of an element */
  char *name;                   // element name
  int num;                      // # of elements in this object
  int size;                     // size of element (bytes)
                                //  or -1 if variable
  vector<PlyProperty> props;    // list of properties in the file
  vector<int> store_prop;       // flags: property wanted by user?
  int other_offset;             // offset to un-asked-for props,
                                //  or -1 if none
  int other_size;               // size of other_props structure

  PlyElement(char *n) : num(0)
    { name = strdup(n); }

  PlyElement() : num (0)
	{ name = NULL; }
};

struct PlyOtherProp {   /* describes other properties in an element */
  char *name;                   // element name
  int size;                     // size of other_props
  vector<PlyProperty> props;    // list of properties in other_props
};

struct OtherData { /* for storing other_props for an other element */
  void *other_props;
};

struct OtherElem {     /* data for one "other" element */
  char *elem_name;             /* names of other elements */
  vector<OtherData *> other_data;// actual property data for the elements
  PlyOtherProp *other_props;   /* description of the property data */
};

struct PlyOtherElems {  /* "other" elements, not interpreted by user */
  vector<OtherElem> list;
};

class PlyFile {
private:

#ifndef WIN32
  gzFile gzfp;                // file pointer for reading
#endif
  FILE *fp;                   // file pointer for writing
  float m_version;            // version number of file
  int   m_file_type;          // ascii or binary
  vector<PlyElement> elems;   // list of elements
  vector<char *> comments;    // list of comments
  vector<char *> obj_info;    // list of object info items
  PlyElement *which_elem;     // the element we're now writing
  PlyOtherElems *other_elems; // "other" elements from a PLY file

  int  fbuf_size;
  char *fbuf;                 // buffer for reading files in blocks
  char *fb_s, *fb_e;          // start and end pointers to fbuf

  void fbuf_need(int n);

  char           read_char(void);
  unsigned char  read_uchar(void);
  short          read_short_BE(void);
  unsigned short read_ushort_BE(void);
  int            read_int_BE(void);
  unsigned int   read_uint_BE(void);
  float          read_float_BE(void);
  double         read_double_BE(void);
  short          read_short_LE(void);
  unsigned short read_ushort_LE(void);
  int            read_int_LE(void);
  unsigned int   read_uint_LE(void);
  float          read_float_LE(void);
  double         read_double_LE(void);

  int  to_int(char *item, int type);

  void get_and_store_binary_item(int type_ext,
				 int type_int,
				 char *item);
  void get_and_store_binary_list(int type_ext,
				 int type_int,
				 char *item,
				 int n);

  PlyElement *find_element(char *);
  void ascii_get_element(char *elem_ptr,
			 int use_provided_storage);
  void binary_get_element(char *elem_ptr,
			  int use_provided_storage);
  void write_scalar_type (int);
  void write_binary_item_BE(int, unsigned int, double, int);
  void write_binary_item_LE(int, unsigned int, double, int);
  void write_ascii_item(int, unsigned int, double, int);
  void add_element(char **, int);
  void add_property(char **, int);
  void add_comment(char *);
  void add_obj_info(char *);
  double get_item_value(char *, int);
  void get_ascii_item(char *, int, int *, unsigned int *,double *);
  void get_binary_item_BE(int, int *, unsigned int *, double *);
  void get_binary_item_LE(int, int *, unsigned int *, double *);
  char **get_words(int *, char **);
  PlyProperty *find_property(PlyElement *, char *, int *);
  void store_item(char *, int, int, unsigned int, double);
  void get_stored_item(void *, int, int *,unsigned int *,double *);
  void _put_element(void *, int static_strg);

public:

  PlyFile();
  ~PlyFile();

  int  open_for_writing(const char *filename, int nelems,
			char **elem_names, int file_type);
  void describe_element(char *, int, int, PlyProperty *);
  void describe_property(char *, PlyProperty *);
  void describe_other_elements (PlyOtherElems *);
  void describe_other_properties(PlyOtherProp *other, int offset);
  void element_count(char *, int);
  void header_complete(void);
  void put_element_setup(char *);
  void put_element(void *);
  void put_element_static_strg(void *);
  void put_comment(char *);
  void put_obj_info(char *);

  int  open_for_reading(const char *filename, int *nelems,
			char ***elem_names);
  PlyProperty **get_element_description(char *, int*, int*);
  void get_element_setup(char *, int, PlyProperty *);
  void get_property(char *, PlyProperty *);
  PlyOtherProp *get_other_properties(char *, int);
  void get_element(void *);
  void get_element_noalloc(void *);
  char **get_comments(int *);
  char **get_obj_info(int *);
  PlyOtherElems *get_other_element(char *, int);
  void put_other_elements(void);
  int is_valid_property(char *elem_name, char *prop_name);

  void close(void);

  float version(void)   { return m_version; }
  int   file_type(void) { return m_file_type; }
};


void copy_property(PlyProperty *, PlyProperty *);
int equal_strings(char *, char *);

int isPlyFile(char *filename);

#endif    //__PLYPLUSPLUS_H__
