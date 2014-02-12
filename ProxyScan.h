// ProxyScan.h                 placeholder for unloaded scans
// created 3/12/99             Matt Ginzton (magi@cs)


#ifndef _PROXYSCAN_H_
#define _PROXYSCAN_H_


#include "RigidScan.h"


class ProxyScan: public RigidScan
{
 public:
  // ProxyScan manipulation
  ProxyScan (const crope& proxyForFileName,
	     const Pnt3& min, const Pnt3& max);
  ~ProxyScan();

 public:
  // RigidScan methods:

  // Display
  virtual MeshTransport* mesh(bool         perVertex = true,
			      bool         stripped  = true,
			      ColorSource  color = colorNone,
			      int          colorSize = 3);

  virtual crope getInfo(void);
  virtual unsigned long byteSize() { return sizeof(ProxyScan); }
};


#endif // _PROXYSCAN_H_
