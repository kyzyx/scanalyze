//////////////////////////////////////////////////////////////////////
//
// OrganizingScan.h
//
// Matt Ginzton
// Tue May 30 16:41:09 PDT 2000
//
// Dummy scan/helper class for DisplayableOrganizingMesh
//
//////////////////////////////////////////////////////////////////////


#ifndef _ORGANIZING_SCAN_H_
#define _ORGANIZING_SCAN_H_

#include "RigidScan.h"


class OrganizingScan: public RigidScan
{
 public:
  OrganizingScan();

  virtual MeshTransport* mesh(bool         perVertex = true,
			      bool         stripped  = true,
			      ColorSource  color = colorNone,
			      int          colorSize = 3);

  virtual void computeBBox (void);
  virtual crope getInfo(void);

};


#endif // _ORGANIZING_SCAN_H_
