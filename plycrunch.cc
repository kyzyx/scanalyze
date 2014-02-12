/*
 * plycrunch.cc
 * Scanalyze module incorporating the old standalone plycrunch decimator.
 *
 * Created: Greg Turk, August 1994
 *
 *   Simplify a polygon model by collapsing multiple points together.  This
 *   is basically Jarek Rossignac's method of simplifying polygon models.
 *
 *   This code borrows heavily from "plyshared".
 *
 * Modified: Matt Ginzton, magi@cs, November 18 1998
 *   converted to c++, incorporated into scanalyze
 *
 */

#include <iostream>
#include <vector>
#include "Pnt3.h"



struct Vertex {
  Pnt3 pos;
  int count;                    /* number of vertices that contributed */
  int a,b,c;                    /* integer indices used in hash */
  int index;
  Vertex *shared;
  Vertex *next;
};


/* hash table for near neighbor searches */

#define PR1  17
#define PR2 101

static const int TABLE_SIZE[] = {
  5003,
  17011,
  53003,
  107021,
  233021,
  472019,
  600169,
  1111189,
  2222177,
  4444147,
  9374153,
  20123119,
  30123139,
  50123011,
  70123117,
  100123171,
  140123143,
  200123111,
  400123123,
  800123119,
};


struct Hash_Table {	        /* uniform spatial subdivision, with hash */
  int npoints;			/* number of points placed in table */
  Vertex **verts;		/* array of hash cells */
  int num_entries;		/* number of array elements in verts */
  float scale;			/* size of cell */
};


Hash_Table *init_table(int, float);
void add_to_hash (Vertex *, Hash_Table *, float);
void crunch_vertices (vector<Pnt3>& vtx, vector<int>& tris, float tolerance);
float compute_average_edge_length (const vector<Pnt3>& vtx,
				   const vector<int>& tris);



/******************************************************************************
External interface
******************************************************************************/

void plycrunch_simplify (const vector<Pnt3>& vtx, const vector<int>& tris,
			 vector<Pnt3>& outVtx, vector<int>& outTris,
			 int approxDesired)
{
  outVtx = vtx;
  outTris = tris;

#ifdef _ABI64
  cerr << "N64 version can't do plycrunch!" << endl;
#else
  float avg_edge_length;
  float tolerance;

  // attempt to guess tolerance based on desired output count
  avg_edge_length = compute_average_edge_length (vtx, tris);
  cout << "edge len: " << avg_edge_length << endl;

  // tolerance = edgelen * ratio: deletes approx. enough verts s.t.
  // outtris = intris / ratio^2
  // the 3 is arbitrary, by inspection
  float ratio = sqrt (tris.size() / (3.*approxDesired));
  tolerance = avg_edge_length * ratio;

  crunch_vertices(outVtx, outTris, tolerance);
#endif
}


/******************************************************************************
Figure out which vertices should be collapsed into one.
******************************************************************************/

void
crunch_vertices (vector<Pnt3>& vtx, vector<int>& tris, float tolerance)
{
#ifndef _ABI64
  int i,j;
  int jj;
  Hash_Table *table;
  float squared_dist;
  Vertex *vert;

  table = init_table (vtx.size(), tolerance);

  squared_dist = tolerance * tolerance;

  /* place all vertices in the hash table, and in the process */
  /* learn which ones should be collapsed */

  int nVerts = vtx.size();
  int nTris = tris.size();

  vector<Vertex> vlist (nVerts);
  for (i = 0; i < nVerts; i++) {
    vlist[i].count = 1;
    vlist[i].pos = vtx[i];
    add_to_hash (&vlist[i], table, squared_dist);
  }

  vector<bool> used (nTris / 3, true);

  /* compute average of all coordinates that contributed to */
  /* the vertices placed in the hash table */

  for (i = 0; i < nVerts; i++) {
    vert = &vlist[i];
    if (vert->shared == vert) {
      vert->pos /= vert->count;
    }
  }

  /* fix up the faces to point to the collapsed vertices */

  for (i = 0; i < nTris; i += 3) {

    /* change all indices to pointers to the collapsed vertices */
    for (j = 0; j < 3; j++)
      tris[i+j] = (int) (vlist[tris[i+j]].shared);

    /* collapse adjacent vertices in a face that are the same */
    for (j = 2; j >= 0; j--) {
      jj = (j+1) % 3;
      if (tris[i+j] == tris[i+jj]) {
	used[i/3] = false;
      }
    }
  }

  // recreate geometry/face array
  int nOutVerts = 0;
  for (i = 0; i < nVerts; i++) {
    if (vlist[i].shared == &vlist[i]) {
      vlist[i].index = nOutVerts++;
    }
  }

  cout << "Output mesh vertices: " << nOutVerts << endl;
  vtx.clear();
  vtx.reserve (nOutVerts);
  for (i = 0; i < nVerts; i++) {
    if (vlist[i].shared == &vlist[i])
      vtx.push_back (vlist[i].pos);
  }

  int nOutTris = 0;
  int nt = nTris / 3;
  for (i = 0; i < nt; i ++) {
    if (used[i])
      nOutTris += 3;
  }

  cout << "Output mesh tris: " << nOutTris << endl;
  vector<int> otris;
  otris.reserve (nOutTris);
  for (i = 0; i < nTris; i += 3) {
    if (used[i/3]) {
      otris.push_back (((Vertex*)tris[i])->index);
      otris.push_back (((Vertex*)tris[i+1])->index);
      otris.push_back (((Vertex*)tris[i+2])->index);
    }
  }

  tris = otris;
#endif
}


/******************************************************************************
Compute the average edge length.  Currently loops through faces and
visits all shared edges twice, giving them double wieghting with respect
to boundary edges.
******************************************************************************/

float
compute_average_edge_length (const vector<Pnt3>& vtx, const vector<int>& tris)
{
  float total_length = 0;

  int nTris = tris.size();
  if (!nTris)
    return 0;

  for (int i = 0; i < nTris; i += 3) {
    for (int j = 0; j < 3; j++) {
      int jj = (j+1) % 3;
      const Pnt3& v1 = vtx[tris[i+j]];
      const Pnt3& v2 = vtx[tris[i+jj]];

      float norm2 = (v1-v2).norm2();
      if (norm2) { // avoid sqrt(0)
	total_length += sqrtf (norm2);
      }
    }
  }

  return total_length/nTris;
}


/******************************************************************************
Add a vertex to it's hash table.

Entry:
  vert    - vertex to add
  table   - hash table to add to
  sq_dist - squared value of distance tolerance
******************************************************************************/

void
add_to_hash(Vertex *vert, Hash_Table *table, float sq_dist)
{
  int index;
  int a,b,c;
  float scale;
  Vertex *ptr;

  /* determine which cell the position lies within */

  scale = table->scale;
  a = floor (vert->pos[0] * scale);
  b = floor (vert->pos[1] * scale);
  c = floor (vert->pos[2] * scale);
  index = (a * PR1 + b * PR2 + c) % table->num_entries;
  if (index < 0)
    index += table->num_entries;

  /* examine all points hashed to this cell, looking for */
  /* a vertex to collapse with */

  for (ptr = table->verts[index]; ptr != NULL; ptr = ptr->next)
    if (a == ptr->a && b == ptr->b && c == ptr->c) {
      /* add to sums of coordinates (that later will be averaged) */
      ptr->pos += vert->pos;
      ptr->count++;
      vert->shared = ptr;
      return;
    }

  /* no match if we get here, so add new hash table entry */

  vert->next = table->verts[index];
  table->verts[index] = vert;
  vert->shared = vert;  /* self-reference as close match */
  vert->a = a;
  vert->b = b;
  vert->c = c;
}


/******************************************************************************
Initialize a uniform spatial subdivision table.  This structure divides
3-space into cubical cells and deposits points into their appropriate
cells.  It uses hashing to make the table a one-dimensional array.

Entry:
  nverts - number of vertices that will be placed in the table
  size   - size of a cell

Exit:
  returns pointer to hash table
******************************************************************************/

Hash_Table *
init_table(int nverts, float size)
{
  Hash_Table *table;
  float scale;

  /* allocate new hash table */

  table = (Hash_Table *) malloc (sizeof (Hash_Table));

  table->num_entries = 0;
  const int nTableSizes = sizeof(TABLE_SIZE) / sizeof(TABLE_SIZE[0]);
  for (int ii = 0; ii < nTableSizes; ii++) {
    if (nverts < TABLE_SIZE[ii]) {
      table->num_entries = TABLE_SIZE[ii];
      break;
    }
  }

  if (!table->num_entries) {
    cout << "Warning: hash table was not prepared for this many vertices"
	 << endl;
    table->num_entries = TABLE_SIZE[nTableSizes-1];
  }

  table->verts = (Vertex **) malloc (sizeof (Vertex *) * table->num_entries);

  /* set all table elements to NULL */
  for (int i = 0; i < table->num_entries; i++)
    table->verts[i] = NULL;

  /* place each point in table */

  scale = 1 / size;
  table->scale = scale;

  return (table);
}
