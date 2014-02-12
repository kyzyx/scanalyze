// GroupUI.h              manage interface to groupscan
// created 1/30/99        magi@cs


#ifndef _GROUPUI_H_
#define _GROUPUI_H_


DisplayableMesh* groupScans (vector<DisplayableMesh*>& scans,
			     char* nameToUse = NULL, bool bDirty = true);
vector<DisplayableMesh*> ungroupScans (DisplayableMesh* group);
bool     addToGroup (DisplayableMesh* group, DisplayableMesh* scan);
bool     removeFromGroup (DisplayableMesh* group, DisplayableMesh* scan);
char *getNextUnusedGroupName ();

#endif // _GROUPUI_H_
