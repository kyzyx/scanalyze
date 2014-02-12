//############################################################
// Median.h
// Kari Pulli
// 01/20/96
//############################################################

#ifndef _MEDIAN_H_
#define _MEDIAN_H_

#include <cassert>
#include <string.h>


template <class T>  // use with float or double
class Median {
private:
  T perc;
  T *data;
  int cnt;
  int size;
  int div_point;

  void divide(void)
    {
      int left, right, inleft, inright;
      T curr_med, temp;
      div_point = perc*(cnt-.5);

      left = 0;
      right = cnt-1;
      while (left < right) {
	curr_med = data[div_point];
	inleft  = left;
	inright = right;
	while (inleft <= inright) {
	  while (data[inleft]  < curr_med) inleft++;
	  while (data[inright] > curr_med) inright--;
	  if (inleft <= inright) {
	    temp = data[inleft];
	    data[inleft]  = data[inright];
	    data[inright] = temp;
	    inleft++;
	    inright--;
	  }
	}
	if (inright < div_point) left  = inleft;
	if (div_point < inleft)  right = inright;
      }
    }
public:
  Median(int p = 50, int s = 256) : perc(p/100.0), cnt(0), size(s)
    {
      data = new T[size];
      assert (data != NULL);
    }
  ~Median(void) { delete[] data; }
  void set_percentage(float p)
    { perc = p; }
  void zap(void)
    {
      delete[] data;
      data = new T[size = 256];
      assert (data != NULL);
      cnt = 0;
    }
  void clear()  { cnt = 0; }
  void reserve(int n)
    {
      if (size < n) {
	T *tmp = new T[n];
	assert (tmp != NULL);
	memcpy(tmp, data, size*sizeof(T));
	//for (int i=0; i<size; i++) tmp[i] = data[i];
	delete[] data; data = tmp;
	size=n;
      }
    }
  void operator +=(T f)
    {
      if (cnt==size) {
	T *tmp = new T[2*size];
	assert (tmp != NULL);
	memcpy(tmp, data, size*sizeof(T));
	delete[] data; data = tmp;
	size*=2;
      }
      data[cnt++] = f;
    }
  T find(void)
    {
      divide();
      return data[div_point];
    }
  T sum_until_median(void)
    {
      divide();
      T sum = 0.0;
      for (int i=0; i<=div_point; i++) sum += data[i];
      return sum;
    }
  T avrg_sum_until_median(void)
    {
      return sum_until_median()/T(div_point);
    }
  T inlier_percentage(T thr)
    {
      int cnt = 0;
      for (int i=0; i<size; i++) {
	if (data[i] < thr) cnt++;
      }
      return T(cnt)/T(size);
    }
};

#endif
