// ToglCache.h               cache togl frame buffer for fast redraws
// created 10/27/98          magi


#ifndef _TOGLCACHE_H_
#define _TOGLCACHE_H_


class DisplayCache
{
public:
  DisplayCache (struct Togl* togl);
  ~DisplayCache();

  bool Render();
  bool Invalidate();
  bool Update();
  bool Allocate();
  bool Free();
  void Enable (bool bEnable);

  static void InvalidateToglCache (struct Togl* togl);
  static void UpdateToglCache (struct Togl* togl);
  static void EnableToglCache (struct Togl* togl, bool bEnable);

private:
  struct Togl* mTogl;
  Togl_Callback* mOldDestroyProc;
  Togl_Callback* mOldReshapeProc;
  Togl_Callback* mOldDisplayProc;

  bool mEnabled;
  bool mValid;
  int mX;
  int mY;
  char* mData;
  char* mZData;

  static void DC_ToglDestroy (struct Togl* togl);
  static void DC_ToglReshape (struct Togl* togl);
  static void DC_ToglDisplay (struct Togl* togl);

  static void AddToglDC (struct Togl* togl, DisplayCache* dc);
  static DisplayCache* FindToglDC (struct Togl* togl);
};


#define TOGLCACHE_DEBUG 0

#endif // _TOGLCACHE_H_
