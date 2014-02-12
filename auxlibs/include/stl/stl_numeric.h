/*
 *
 * Copyright (c) 1994
 * Hewlett-Packard Company
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Hewlett-Packard Company makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 *
 * Copyright (c) 1996,1997
 * Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Silicon Graphics makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 */

/* NOTE: This is an internal header file, included by other STL headers.
 *   You should not attempt to use it directly.
 */


#ifndef __SGI_STL_INTERNAL_NUMERIC_H
#define __SGI_STL_INTERNAL_NUMERIC_H

__STL_BEGIN_NAMESPACE

template <class _InputIterator, class _T>
_T accumulate(_InputIterator __first, _InputIterator __last, _T __init)
{
  for ( ; __first != __last; ++__first)
    __init = __init + *__first;
  return __init;
}

template <class _InputIterator, class _T, class _BinaryOperation>
_T accumulate(_InputIterator __first, _InputIterator __last, _T __init,
              _BinaryOperation __binary_op)
{
  for ( ; __first != __last; ++__first)
    __init = __binary_op(__init, *__first);
  return __init;
}

template <class _InputIterator1, class _InputIterator2, class _T>
_T inner_product(_InputIterator1 __first1, _InputIterator1 __last1,
                 _InputIterator2 __first2, _T __init)
{
  for ( ; __first1 != __last1; ++__first1, ++__first2)
    __init = __init + (*__first1 * *__first2);
  return __init;
}

template <class _InputIterator1, class _InputIterator2, class _T,
          class _BinaryOperation1, class _BinaryOperation2>
_T inner_product(_InputIterator1 __first1, _InputIterator1 __last1,
                 _InputIterator2 __first2, _T __init,
                 _BinaryOperation1 __binary_op1,
                 _BinaryOperation2 __binary_op2)
{
  for ( ; __first1 != __last1; ++__first1, ++__first2)
    __init = __binary_op1(__init, __binary_op2(*__first1, *__first2));
  return __init;
}

template <class _InputIterator, class _OutputIterator, class _T>
_OutputIterator
__partial_sum(_InputIterator __first, _InputIterator __last,
              _OutputIterator __result, _T*)
{
  _T __value = *__first;
  while (++__first != __last) {
    __value = __value + *__first;
    *++__result = __value;
  }
  return ++__result;
}

template <class _InputIterator, class _OutputIterator>
_OutputIterator
partial_sum(_InputIterator __first, _InputIterator __last,
            _OutputIterator __result)
{
  if (__first == __last) return __result;
  *__result = *__first;
  return __partial_sum(__first, __last, __result, __VALUE_TYPE(__first));
}

template <class _InputIterator, class _OutputIterator, class _T,
          class _BinaryOperation>
_OutputIterator
__partial_sum(_InputIterator __first, _InputIterator __last,
              _OutputIterator __result, _T*, _BinaryOperation __binary_op)
{
  _T __value = *__first;
  while (++__first != __last) {
    __value = __binary_op(__value, *__first);
    *++__result = __value;
  }
  return ++__result;
}

template <class _InputIterator, class _OutputIterator, class _BinaryOperation>
_OutputIterator
partial_sum(_InputIterator __first, _InputIterator __last,
            _OutputIterator __result, _BinaryOperation __binary_op)
{
  if (__first == __last) return __result;
  *__result = *__first;
  return __partial_sum(__first, __last, __result, __VALUE_TYPE(__first),
                       __binary_op);
}

template <class _InputIterator, class _OutputIterator, class _T>
_OutputIterator
__adjacent_difference(_InputIterator __first, _InputIterator __last,
                      _OutputIterator __result, _T*)
{
  _T __value = *__first;
  while (++__first != __last) {
    _T __tmp = *__first;
    *++__result = __tmp - __value;
    __value = __tmp;
  }
  return ++__result;
}

template <class _InputIterator, class _OutputIterator>
_OutputIterator
adjacent_difference(_InputIterator __first,
                    _InputIterator __last, _OutputIterator __result)
{
  if (__first == __last) return __result;
  *__result = *__first;
  return __adjacent_difference(__first, __last, __result,
                               __VALUE_TYPE(__first));
}

template <class _InputIterator, class _OutputIterator, class _T,
          class _BinaryOperation>
_OutputIterator
__adjacent_difference(_InputIterator __first, _InputIterator __last,
                      _OutputIterator __result, _T*,
                      _BinaryOperation __binary_op) {
  _T __value = *__first;
  while (++__first != __last) {
    _T __tmp = *__first;
    *++__result = __binary_op(__tmp, __value);
    __value = __tmp;
  }
  return ++__result;
}

template <class _InputIterator, class _OutputIterator, class _BinaryOperation>
_OutputIterator
adjacent_difference(_InputIterator __first, _InputIterator __last,
                    _OutputIterator __result, _BinaryOperation __binary_op)
{
  if (__first == __last) return __result;
  *__result = *__first;
  return __adjacent_difference(__first, __last, __result,
                               __VALUE_TYPE(__first),
                               __binary_op);
}

// Returns __x ** __n, where __n >= 0.  _Note that "multiplication"
// is required to be associative, but not necessarily commutative.


template <class _T, class _Integer, class _MonoidOperation>
_T __power(_T __x, _Integer __n, _MonoidOperation __opr)
{
  if (__n == 0)
    return identity_element(__opr);
  else {
    while ((__n & 1) == 0) {
      __n >>= 1;
      __x = __opr(__x, __x);
    }

    _T __result = __x;
    __n >>= 1;
    while (__n != 0) {
      __x = __opr(__x, __x);
      if ((__n & 1) != 0)
        __result = __opr(__result, __x);
      __n >>= 1;
    }
    return __result;
  }
}

template <class _T, class _Integer>
inline _T __power(_T __x, _Integer __n)
{
  return __power(__x, __n, multiplies<_T>());
}

// Alias for the internal name __power.  Note that power is an extension,
// not part of the C++ standard.

template <class _T, class _Integer, class _MonoidOperation>
inline _T power(_T __x, _Integer __n, _MonoidOperation __opr)
{
  return __power(__x, __n, __opr);
}

template <class _T, class _Integer>
inline _T power(_T __x, _Integer __n)
{
  return __power(__x, __n);
}

// iota is not part of the C++ standard.  It is an extension.

template <class _ForwardIterator, class _T>
void
iota(_ForwardIterator __first, _ForwardIterator __last, _T __value)
{
  while (__first != __last)
    *__first++ = __value++;
}

__STL_END_NAMESPACE

#endif /* __SGI_STL_INTERNAL_NUMERIC_H */

// Local Variables:
// mode:C++
// End:
