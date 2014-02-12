//############################################################
//
// ResolutionCtrl.h
//
// Kari Pulli
// Fri Jul  3 11:52:55 PDT 1998
//
//############################################################

#ifndef _RESOLUTIONCTRL_H_
#define _RESOLUTIONCTRL_H_

#include <vector.h>
#include <rope.h>

class ResolutionCtrl {
public:
  struct res_info {
    int   abs_resolution; // number of vertices
    bool  in_memory;      // true == in memory, false == in file
    bool  desired_in_mem;
    crope filename;

    res_info(void) : abs_resolution(0),
      in_memory(false), desired_in_mem(true) { }

    friend bool operator<(const res_info& r1, const res_info& r2)
      { // descending sort by number of vertices
	return r1.abs_resolution > r2.abs_resolution;
      };
  };

protected:
  vector<res_info> resolutions;

  int     curr_res;
  crope   name;
  crope   basename;
  crope   ending;

private:
  void    split_name(void);

public:

  ResolutionCtrl(void) : curr_res(0) { }

  //
  // Functions for name
  //
  void  set_name(const char  *n);
  void  set_name(const crope &n);
  crope get_name(void);
  crope get_basename(void);
  crope get_nameending(void);
  bool  has_ending(const char *end);

  //
  // Select a resolution
  //
  bool select_finest(void);
  bool select_coarsest(void);
  bool select_finer(void);
  bool select_coarser(void);
  bool select_by_count(int n);

  //
  // Create a new resolution (if it doesn't exist)
  //

  typedef enum { decQslim, decPlycrunch } Decimator;

  virtual int create_resolution_absolute(int budget = 0,
					  Decimator dec = decQslim);

  virtual bool delete_resolution (int abs_res);

  //
  // Find out about resolutions
  //
  int      num_resolutions(void) { return resolutions.size(); }
  void     existing_resolutions(vector<res_info> &res);
  res_info current_resolution(void);
  int      current_resolution_index (void);
  int          findResForLevel (int n);
  bool     set_load_desired (int nPolys, bool desired);

  //
  // Memory handling
  //
  virtual bool load_resolution(int i);
  virtual bool release_resolution(int nPolys);

 protected:
  int          findLevelForRes (int n);

  void         insert_resolution (int abs, crope filename,
				  bool in_mem = true,
				  bool desired_mem = true);

  virtual bool switchToResLevel (int iRes);
};

#endif
