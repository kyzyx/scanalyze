//############################################################
// ConnComp.h
// Kari Pulli
// Wed Feb 17 12:03:17 CET 1999
//
// A class for figuring out the connected components
//############################################################

#ifndef _CONNCOMP_H_
#define _CONNCOMP_H_

#include <vector>
#include <iostream>
#include <iomanip>


//#define TEST_CONNCOMP

class ConnComp {
private:

  vector<int>   label;
  int           max;
  int           next_group;


  void display(void)
    {
      for (int i=0; i<label.size(); i++) {
	cout << setw(3) << i << " : "
	     << setw(3) << label[i] << endl;
      }
    }

  int  get_label(int i)
    {
      if (label[i] != i)
	label[i] = get_label(label[i]);
      return label[i];
    }

public:

  ConnComp(int size)
    : max(-1), next_group(0), label(size)
    {
      for (int i=0; i<size; i++) label[i] = i;
    }

  ~ConnComp(void) {}

  void collapse(void)
    {
      next_group = 0;
      max = -1;
      for (int i=1; i<label.size(); i++) {
	label[i] = label[label[i]];
	if (label[i] > max) max = label[i];
      }
#ifdef TEST_CONNCOMP
      display();
#endif
    }

  void connect(int a, int b)
    {
      assert(a >= 0 && b >= 0);
      assert(a < label.size() && b < label.size());

      int la = get_label(a);
      int lb = get_label(b);
      if (la < lb) label[lb] = la;
      else         label[la] = lb;
#ifdef TEST_CONNCOMP
      display();
#endif
    }

  void restart_groups(void)
    {
      next_group = 0;
    }

  // fill the vector 'group' with all the elements that belong
  // to the next group
  // return a boolean that's false if all the groups have
  // been gotten already
  bool get_next_group(vector<int> &group)
    {
      if (next_group == 0)  collapse();

      if (next_group > max) return false;

      group.clear();
      while (next_group != label[next_group])
	next_group++;
      for (int i=0; i < label.size(); i++) {
        if (label[i] == next_group)
	  group.push_back(i);
      }

      next_group++;
      return true;
    }

  int num_groups (void)
    {
      return max;
    }
};


#endif


#ifdef TEST_CONNCOMP
void
main(void)
{
  ConnComp b(10);
  vector<int> g;
  while (1) {
    cout << "add or collapse or get next group" << endl;
    char txt[256];
    cin >> txt;
    if (txt[0] == 'a') {
      cout << "give two numbers" << endl;
      int a1, a2;
      cin >> a1 >> a2;
      b.connect(a1, a2);
    } else if (txt[0] == 'c') {
      b.collapse();
    } else {
      cout << b.get_next_group(g) << endl;
      cout << g.size() << endl;
      for (int i=0; i<g.size(); i++) {
	cout << g[i] << " ";
      }
      cout << endl;
    }
  }
}
#endif
