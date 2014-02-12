/////////////////////////////////////////////////////////////////////////////
//
// MCEdgeTable.h -- initializes edge table containing triangle locations
//
// Matt Ginzton, 2:26 PM 8/26/98
// adapted from code by Brian Curless, 10/24/95
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _EDGETABLE_H_
#define _EDGETABLE_H_


typedef int VertexIndex;
struct Triple {
  VertexIndex A;
  VertexIndex B;
  VertexIndex C;
};


struct EdgeTableEntry {
  bool edge[12];
  int Ntriangles;
  Triple *TriangleList;
};


class EdgeTable {
  public:
    EdgeTable();
    ~EdgeTable();

    EdgeTableEntry& operator[] (int index);

  private:
    EdgeTableEntry myEdgeTable[256];
};


#endif // _EDGETABLE_H_
