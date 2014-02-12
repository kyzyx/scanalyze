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
 * Copyright (c) 1996-1998
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

#ifndef __SGI_STL_INTERNAL_ITERATOR_H
#define __SGI_STL_INTERNAL_ITERATOR_H

__STL_BEGIN_NAMESPACE

struct input_iterator_tag {};
struct output_iterator_tag {};
struct forward_iterator_tag : public input_iterator_tag {};
struct bidirectional_iterator_tag : public forward_iterator_tag {};
struct random_access_iterator_tag : public bidirectional_iterator_tag {};

// The base classes input_iterator, output_iterator, forward_iterator,
// bidirectional_iterator, and random_access_iterator are not part of
// the C++ standard.  (they have been replaced by struct iterator.)
// They are included for backward compatibility with the HP STL.

template <class _T, class _Distance> struct input_iterator {
  typedef input_iterator_tag  iterator_category;
  typedef _T                  value_type;
  typedef _Distance           difference_type;
  typedef _T*                 pointer;
  typedef _T&                 reference;
};

struct output_iterator {
  typedef output_iterator_tag iterator_category;
  typedef void                value_type;
  typedef void                difference_type;
  typedef void                pointer;
  typedef void                reference;
};

template <class _T, class _Distance> struct forward_iterator {
  typedef forward_iterator_tag  iterator_category;
  typedef _T                    value_type;
  typedef _Distance             difference_type;
  typedef _T*                   pointer;
  typedef _T&                   reference;
};


template <class _T, class _Distance> struct bidirectional_iterator {
  typedef bidirectional_iterator_tag  iterator_category;
  typedef _T                          value_type;
  typedef _Distance                   difference_type;
  typedef _T*                         pointer;
  typedef _T&                         reference;
};

template <class _T, class _Distance> struct random_access_iterator {
  typedef random_access_iterator_tag  iterator_category;
  typedef _T                          value_type;
  typedef _Distance                   difference_type;
  typedef _T*                         pointer;
  typedef _T&                         reference;
};

#ifdef __STL_USE_NAMESPACES
template <class _Category, class _T, class _Distance = ptrdiff_t,
          class _Pointer = _T*, class _Reference = _T&>
struct iterator {
  typedef _Category  iterator_category;
  typedef _T         value_type;
  typedef _Distance  difference_type;
  typedef _Pointer   pointer;
  typedef _Reference reference;
};
#endif /* __STL_USE_NAMESPACES */

#ifdef __STL_CLASS_PARTIAL_SPECIALIZATION

template <class _Iterator>
struct iterator_traits {
  typedef typename _Iterator::iterator_category iterator_category;
  typedef typename _Iterator::value_type        value_type;
  typedef typename _Iterator::difference_type   difference_type;
  typedef typename _Iterator::pointer           pointer;
  typedef typename _Iterator::reference         reference;
};

template <class _T>
struct iterator_traits<_T*> {
  typedef random_access_iterator_tag iterator_category;
  typedef _T                          value_type;
  typedef ptrdiff_t                   difference_type;
  typedef _T*                         pointer;
  typedef _T&                         reference;
};

template <class _T>
struct iterator_traits<const _T*> {
  typedef random_access_iterator_tag iterator_category;
  typedef _T                          value_type;
  typedef ptrdiff_t                   difference_type;
  typedef const _T*                   pointer;
  typedef const _T&                   reference;
};

// The overloaded functions iterator_category, distance_type, and
// value_type are not part of the C++ standard.  (They have been
// replaced by struct iterator_traits.)  They are included for
// backward compatibility with the HP STL.

// We introduce internal names for these functions.

template <class _Iter>
inline typename iterator_traits<_Iter>::iterator_category
__iterator_category(const _Iter&)
{
  typedef typename iterator_traits<_Iter>::iterator_category _Category;
  return _Category();
}

template <class _Iter>
inline typename iterator_traits<_Iter>::difference_type*
__distance_type(const _Iter&)
{
  return static_cast<typename iterator_traits<_Iter>::difference_type*>(0);
}

template <class _Iter>
inline typename iterator_traits<_Iter>::value_type*
__value_type(const _Iter&)
{
  return static_cast<typename iterator_traits<_Iter>::value_type*>(0);
}

template <class _Iter>
inline typename iterator_traits<_Iter>::iterator_category
iterator_category(const _Iter& __i) { return __iterator_category(__i); }


template <class _Iter>
inline typename iterator_traits<_Iter>::difference_type*
distance_type(const _Iter& __i) { return __distance_type(__i); }

template <class _Iter>
inline typename iterator_traits<_Iter>::value_type*
value_type(const _Iter& __i) { return __value_type(__i); }

#define __ITERATOR_CATEGORY(__i) __iterator_category(__i)
#define __DISTANCE_TYPE(__i)     __distance_type(__i)
#define __VALUE_TYPE(__i)        __value_type(__i)

#else /* __STL_CLASS_PARTIAL_SPECIALIZATION */

template <class _T, class _Distance>
inline input_iterator_tag
iterator_category(const input_iterator<_T, _Distance>&)
  { return input_iterator_tag(); }

inline output_iterator_tag iterator_category(const output_iterator&)
  { return output_iterator_tag(); }

template <class _T, class _Distance>
inline forward_iterator_tag
iterator_category(const forward_iterator<_T, _Distance>&)
  { return forward_iterator_tag(); }

template <class _T, class _Distance>
inline bidirectional_iterator_tag
iterator_category(const bidirectional_iterator<_T, _Distance>&)
  { return bidirectional_iterator_tag(); }

template <class _T, class _Distance>
inline random_access_iterator_tag
iterator_category(const random_access_iterator<_T, _Distance>&)
  { return random_access_iterator_tag(); }

template <class _T>
inline random_access_iterator_tag iterator_category(const _T*)
  { return random_access_iterator_tag(); }

template <class _T, class _Distance>
inline _T* value_type(const input_iterator<_T, _Distance>&)
  { return (_T*)(0); }

template <class _T, class _Distance>
inline _T* value_type(const forward_iterator<_T, _Distance>&)
  { return (_T*)(0); }

template <class _T, class _Distance>
inline _T* value_type(const bidirectional_iterator<_T, _Distance>&)
  { return (_T*)(0); }

template <class _T, class _Distance>
inline _T* value_type(const random_access_iterator<_T, _Distance>&)
  { return (_T*)(0); }

template <class _T>
inline _T* value_type(const _T*) { return (_T*)(0); }

template <class _T, class _Distance>
inline _Distance* distance_type(const input_iterator<_T, _Distance>&)
{
  return (_Distance*)(0);
}

template <class _T, class _Distance>
inline _Distance* distance_type(const forward_iterator<_T, _Distance>&)
{
  return (_Distance*)(0);
}

template <class _T, class _Distance>
inline _Distance*
distance_type(const bidirectional_iterator<_T, _Distance>&)
{
  return (_Distance*)(0);
}

template <class _T, class _Distance>
inline _Distance*
distance_type(const random_access_iterator<_T, _Distance>&)
{
  return (_Distance*)(0);
}

template <class _T>
inline ptrdiff_t* distance_type(const _T*) { return (ptrdiff_t*)(0); }

// Without partial specialization we can't use iterator_traits, so
// we must keep the old iterator query functions around.

#define __ITERATOR_CATEGORY(__i) iterator_category(__i)
#define __DISTANCE_TYPE(__i)     distance_type(__i)
#define __VALUE_TYPE(__i)        value_type(__i)

#endif /* __STL_CLASS_PARTIAL_SPECIALIZATION */

template <class _InputIterator, class _Distance>
inline void __distance(_InputIterator __first, _InputIterator __last,
                       _Distance& __n, input_iterator_tag)
{
  while (__first != __last) { ++__first; ++__n; }
}

template <class _RandomAccessIterator, class _Distance>
inline void __distance(_RandomAccessIterator __first,
                       _RandomAccessIterator __last,
                       _Distance& __n, random_access_iterator_tag)
{
  __n += __last - __first;
}

template <class _InputIterator, class _Distance>
inline void distance(_InputIterator __first,
                     _InputIterator __last, _Distance& __n)
{
  __distance(__first, __last, __n, iterator_category(__first));
}

#ifdef __STL_CLASS_PARTIAL_SPECIALIZATION

template <class _InputIterator>
inline iterator_traits<_InputIterator>::difference_type
__distance(_InputIterator __first, _InputIterator __last, input_iterator_tag)
{
  iterator_traits<_InputIterator>::difference_type __n = 0;
  while (__first != __last) {
    ++__first; ++__n;
  }
  return __n;
}

template <class _RandomAccessIterator>
inline iterator_traits<_RandomAccessIterator>::difference_type
__distance(_RandomAccessIterator __first, _RandomAccessIterator __last,
           random_access_iterator_tag) {
  return __last - __first;
}

template <class _InputIterator>
inline iterator_traits<_InputIterator>::difference_type
distance(_InputIterator __first, _InputIterator __last) {
  typedef typename iterator_traits<_InputIterator>::iterator_category
    _Category;
  return __distance(__first, __last, _Category());
}

#endif /* __STL_CLASS_PARTIAL_SPECIALIZATION */

template <class _InputIter, class _Distance>
inline void __advance(_InputIter& __i, _Distance __n, input_iterator_tag) {
  while (__n--) ++__i;
}

#if defined(__sgi) && !defined(__GNUC__) && (_MIPS_SIM != _MIPS_SIM_ABI32)
#pragma set woff 1183
#endif

template <class _BidirectionalIterator, class _Distance>
inline void __advance(_BidirectionalIterator& __i, _Distance __n,
                      bidirectional_iterator_tag) {
  if (__n >= 0)
    while (__n--) ++__i;
  else
    while (__n++) --__i;
}

#if defined(__sgi) && !defined(__GNUC__) && (_MIPS_SIM != _MIPS_SIM_ABI32)
#pragma reset woff 1183
#endif

template <class _RandomAccessIterator, class _Distance>
inline void __advance(_RandomAccessIterator& __i, _Distance __n,
                      random_access_iterator_tag) {
  __i += __n;
}

template <class _InputIterator, class _Distance>
inline void advance(_InputIterator& __i, _Distance __n) {
  __advance(__i, __n, iterator_category(__i));
}

template <class _Container>
class back_insert_iterator {
protected:
  _Container* container;
public:
  typedef _Container          container_type;
  typedef output_iterator_tag iterator_category;
  typedef void                value_type;
  typedef void                difference_type;
  typedef void                pointer;
  typedef void                reference;

  explicit back_insert_iterator(_Container& __x) : container(&__x) {}
  back_insert_iterator<_Container>&
  operator=(const typename _Container::value_type& __value) {
    container->push_back(__value);
    return *this;
  }
  back_insert_iterator<_Container>& operator*() { return *this; }
  back_insert_iterator<_Container>& operator++() { return *this; }
  back_insert_iterator<_Container>& operator++(int) { return *this; }
};

#ifndef __STL_CLASS_PARTIAL_SPECIALIZATION

template <class _Container>
inline output_iterator_tag
iterator_category(const back_insert_iterator<_Container>&)
{
  return output_iterator_tag();
}

#endif /* __STL_CLASS_PARTIAL_SPECIALIZATION */

template <class _Container>
inline back_insert_iterator<_Container> back_inserter(_Container& __x) {
  return back_insert_iterator<_Container>(__x);
}

template <class _Container>
class front_insert_iterator {
protected:
  _Container* container;
public:
  typedef _Container          container_type;
  typedef output_iterator_tag iterator_category;
  typedef void                value_type;
  typedef void                difference_type;
  typedef void                pointer;
  typedef void                reference;

  explicit front_insert_iterator(_Container& __x) : container(&__x) {}
  front_insert_iterator<_Container>&
  operator=(const typename _Container::value_type& __value) {
    container->push_front(__value);
    return *this;
  }
  front_insert_iterator<_Container>& operator*() { return *this; }
  front_insert_iterator<_Container>& operator++() { return *this; }
  front_insert_iterator<_Container>& operator++(int) { return *this; }
};

#ifndef __STL_CLASS_PARTIAL_SPECIALIZATION

template <class _Container>
inline output_iterator_tag
iterator_category(const front_insert_iterator<_Container>&)
{
  return output_iterator_tag();
}

#endif /* __STL_CLASS_PARTIAL_SPECIALIZATION */

template <class _Container>
inline front_insert_iterator<_Container> front_inserter(_Container& __x) {
  return front_insert_iterator<_Container>(__x);
}

template <class _Container>
class insert_iterator {
protected:
  _Container* container;
  typename _Container::iterator iter;
public:
  typedef _Container          container_type;
  typedef output_iterator_tag iterator_category;
  typedef void                value_type;
  typedef void                difference_type;
  typedef void                pointer;
  typedef void                reference;

  insert_iterator(_Container& __x, typename _Container::iterator __i)
    : container(&__x), iter(__i) {}
  insert_iterator<_Container>&
  operator=(const typename _Container::value_type& __value) {
    iter = container->insert(iter, __value);
    ++iter;
    return *this;
  }
  insert_iterator<_Container>& operator*() { return *this; }
  insert_iterator<_Container>& operator++() { return *this; }
  insert_iterator<_Container>& operator++(int) { return *this; }
};

#ifndef __STL_CLASS_PARTIAL_SPECIALIZATION

template <class _Container>
inline output_iterator_tag
iterator_category(const insert_iterator<_Container>&)
{
  return output_iterator_tag();
}

#endif /* __STL_CLASS_PARTIAL_SPECIALIZATION */

template <class _Container, class _Iterator>
inline
insert_iterator<_Container> inserter(_Container& __x, _Iterator __i)
{
  typedef typename _Container::iterator __iter;
  return insert_iterator<_Container>(__x, __iter(__i));
}

#ifndef __STL_LIMITED_DEFAULT_TEMPLATES
template <class _BidirectionalIterator, class _T, class _Reference = _T&,
          class _Distance = ptrdiff_t>
#else
template <class _BidirectionalIterator, class _T, class _Reference,
          class _Distance>
#endif
class reverse_bidirectional_iterator {
  typedef reverse_bidirectional_iterator<_BidirectionalIterator, _T,
                                         _Reference, _Distance>  _Self;
protected:
  _BidirectionalIterator _M_current;
public:
  typedef bidirectional_iterator_tag iterator_category;
  typedef _T                          value_type;
  typedef _Distance                   difference_type;
  typedef _T*                         pointer;
  typedef _Reference                  reference;

  reverse_bidirectional_iterator() {}
  explicit reverse_bidirectional_iterator(_BidirectionalIterator __x)
    : _M_current(__x) {}
  _BidirectionalIterator base() const { return _M_current; }
  _Reference operator*() const {
    _BidirectionalIterator __tmp = _M_current;
    return *--__tmp;
  }
#ifndef __SGI_STL_NO_ARROW_OPERATOR
  pointer operator->() const { return &(operator*()); }
#endif /* __SGI_STL_NO_ARROW_OPERATOR */
  _Self& operator++() {
    --_M_current;
    return *this;
  }
  _Self operator++(int) {
    _Self __tmp = *this;
    --_M_current;
    return __tmp;
  }
  _Self& operator--() {
    ++_M_current;
    return *this;
  }
  _Self operator--(int) {
    _Self __tmp = *this;
    ++_M_current;
    return __tmp;
  }
};

#ifndef __STL_CLASS_PARTIAL_SPECIALIZATION

template <class _BidirectionalIterator, class _T, class _Reference,
          class _Distance>
inline bidirectional_iterator_tag
iterator_category(const reverse_bidirectional_iterator<_BidirectionalIterator,
                                                       _T, _Reference,
                                                       _Distance>&)
{
  return bidirectional_iterator_tag();
}

template <class _BidirectionalIterator, class _T, class _Reference,
          class _Distance>
inline _T*
value_type(const reverse_bidirectional_iterator<_BidirectionalIterator, _T,
                                               _Reference, _Distance>&)
{
  return (_T*) 0;
}

template <class _BidirectionalIterator, class _T, class _Reference,
          class _Distance>
inline _Distance*
distance_type(const reverse_bidirectional_iterator<_BidirectionalIterator, _T,
                                                   _Reference, _Distance>&)
{
  return (_Distance*) 0;
}

#endif /* __STL_CLASS_PARTIAL_SPECIALIZATION */

template <class _BiIter, class _T, class _Ref,
          class _Distance>
inline bool operator==(
    const reverse_bidirectional_iterator<_BiIter, _T, _Ref, _Distance>& __x,
    const reverse_bidirectional_iterator<_BiIter, _T, _Ref, _Distance>& __y)
{
  return __x.base() == __y.base();
}

#ifdef __STL_CLASS_PARTIAL_SPECIALIZATION

// This is the new version of reverse_iterator, as defined in the
//  draft C++ standard.  It relies on the iterator_traits template,
//  which in turn relies on partial specialization.  The class
//  reverse_bidirectional_iterator is no longer part of the draft
//  standard, but it is retained for backward compatibility.

template <class _Iterator>
class reverse_iterator
{
protected:
  _Iterator _M_current;
public:
  typedef typename iterator_traits<_Iterator>::iterator_category
          iterator_category;
  typedef typename iterator_traits<_Iterator>::value_type
          value_type;
  typedef typename iterator_traits<_Iterator>::difference_type
          difference_type;
  typedef typename iterator_traits<_Iterator>::pointer
          pointer;
  typedef typename iterator_traits<_Iterator>::reference
          reference;

  typedef _Iterator iterator_type;
  typedef reverse_iterator<_Iterator> _Self;

public:
  reverse_iterator() {}
  explicit reverse_iterator(iterator_type __x) : _M_current(__x) {}

  reverse_iterator(const _Self& __x) : _M_current(__x._M_current) {}
#ifdef __STL_MEMBER_TEMPLATES
  template <class _Iter>
  reverse_iterator(const reverse_iterator<_Iter>& __x)
    : _M_current(__x.base()) {}
#endif /* __STL_MEMBER_TEMPLATES */

  iterator_type base() const { return _M_current; }
  reference operator*() const {
    _Iterator __tmp = _M_current;
    return *--__tmp;
  }
#ifndef __SGI_STL_NO_ARROW_OPERATOR
  pointer operator->() const { return &(operator*()); }
#endif /* __SGI_STL_NO_ARROW_OPERATOR */

  _Self& operator++() {
    --_M_current;
    return *this;
  }
  _Self operator++(int) {
    _Self __tmp = *this;
    --_M_current;
    return __tmp;
  }
  _Self& operator--() {
    ++_M_current;
    return *this;
  }
  _Self operator--(int) {
    _Self __tmp = *this;
    ++_M_current;
    return __tmp;
  }

  _Self operator+(difference_type __n) const {
    return _Self(_M_current - __n);
  }
  _Self& operator+=(difference_type __n) {
    _M_current -= __n;
    return *this;
  }
  _Self operator-(difference_type __n) const {
    return _Self(_M_current + __n);
  }
  _Self& operator-=(difference_type __n) {
    _M_current += __n;
    return *this;
  }
  reference operator[](difference_type __n) const { return *(*this + __n); }
};

template <class _Iterator>
inline bool operator==(const reverse_iterator<_Iterator>& __x,
                       const reverse_iterator<_Iterator>& __y) {
  return __x.base() == __y.base();
}

template <class _Iterator>
inline bool operator<(const reverse_iterator<_Iterator>& __x,
                      const reverse_iterator<_Iterator>& __y) {
  return __y.base() < __x.base();
}

template <class _Iterator>
inline typename reverse_iterator<_Iterator>::difference_type
operator-(const reverse_iterator<_Iterator>& __x,
          const reverse_iterator<_Iterator>& __y) {
  return __y.base() - __x.base();
}

template <class _Iterator>
inline reverse_iterator<_Iterator>
operator+(reverse_iterator<_Iterator>::difference_type __n,
          const reverse_iterator<_Iterator>& __x) {
  return reverse_iterator<_Iterator>(__x.base() - __n);
}

#else /* __STL_CLASS_PARTIAL_SPECIALIZATION */

// This is the old version of reverse_iterator, as found in the original
//  HP STL.  It does not use partial specialization.

#ifndef __STL_LIMITED_DEFAULT_TEMPLATES
template <class _RandomAccessIterator, class _T, class _Reference = _T&,
          class _Distance = ptrdiff_t>
#else
template <class _RandomAccessIterator, class _T, class _Reference,
          class _Distance>
#endif
class reverse_iterator {
  typedef reverse_iterator<_RandomAccessIterator, _T, _Reference, _Distance>
        _Self;
protected:
  _RandomAccessIterator _M_current;
public:
  typedef random_access_iterator_tag iterator_category;
  typedef _T                          value_type;
  typedef _Distance                   difference_type;
  typedef _T*                         pointer;
  typedef _Reference                  reference;

  reverse_iterator() {}
  explicit reverse_iterator(_RandomAccessIterator __x) : _M_current(__x) {}
  _RandomAccessIterator base() const { return _M_current; }
  _Reference operator*() const { return *(_M_current - 1); }
#ifndef __SGI_STL_NO_ARROW_OPERATOR
  pointer operator->() const { return &(operator*()); }
#endif /* __SGI_STL_NO_ARROW_OPERATOR */
  _Self& operator++() {
    --_M_current;
    return *this;
  }
  _Self operator++(int) {
    _Self __tmp = *this;
    --_M_current;
    return __tmp;
  }
  _Self& operator--() {
    ++_M_current;
    return *this;
  }
  _Self operator--(int) {
    _Self __tmp = *this;
    ++_M_current;
    return __tmp;
  }
  _Self operator+(_Distance __n) const {
    return _Self(_M_current - __n);
  }
  _Self& operator+=(_Distance __n) {
    _M_current -= __n;
    return *this;
  }
  _Self operator-(_Distance __n) const {
    return _Self(_M_current + __n);
  }
  _Self& operator-=(_Distance __n) {
    _M_current += __n;
    return *this;
  }
  _Reference operator[](_Distance __n) const { return *(*this + __n); }
};

template <class _RandomAccessIterator, class _T,
          class _Reference, class _Distance>
inline random_access_iterator_tag
iterator_category(const reverse_iterator<_RandomAccessIterator, _T,
                                         _Reference, _Distance>&)
{
  return random_access_iterator_tag();
}

template <class _RandomAccessIterator, class _T,
          class _Reference, class _Distance>
inline _T* value_type(const reverse_iterator<_RandomAccessIterator, _T,
                                            _Reference, _Distance>&)
{
  return (_T*) 0;
}

template <class _RandomAccessIterator, class _T,
          class _Reference, class _Distance>
inline _Distance*
distance_type(const reverse_iterator<_RandomAccessIterator,
                                     _T, _Reference, _Distance>&)
{
  return (_Distance*) 0;
}


template <class _RandomAccessIterator, class _T,
          class _Reference, class _Distance>
inline bool
operator==(const reverse_iterator<_RandomAccessIterator, _T,
                                  _Reference, _Distance>& __x,
           const reverse_iterator<_RandomAccessIterator, _T,
                                  _Reference, _Distance>& __y)
{
  return __x.base() == __y.base();
}

template <class _RandomAccessIterator, class _T,
          class _Reference, class _Distance>
inline bool
operator<(const reverse_iterator<_RandomAccessIterator, _T,
                                 _Reference, _Distance>& __x,
          const reverse_iterator<_RandomAccessIterator, _T,
                                 _Reference, _Distance>& __y)
{
  return __y.base() < __x.base();
}

template <class _RandomAccessIterator, class _T,
          class _Reference, class _Distance>
inline _Distance
operator-(const reverse_iterator<_RandomAccessIterator, _T,
                                 _Reference, _Distance>& __x,
          const reverse_iterator<_RandomAccessIterator, _T,
                                 _Reference, _Distance>& __y)
{
  return __y.base() - __x.base();
}

template <class _RandAccIter, class _T, class _Ref, class _Dist>
inline reverse_iterator<_RandAccIter, _T, _Ref, _Dist>
operator+(_Dist __n,
          const reverse_iterator<_RandAccIter, _T, _Ref, _Dist>& __x)
{
  return reverse_iterator<_RandAccIter, _T, _Ref, _Dist>(__x.base() - __n);
}

#endif /* __STL_CLASS_PARTIAL_SPECIALIZATION */

// When we have templatized iostreams, istream_iterator and ostream_iterator
// must be rewritten.

template <class _T, class _Dist = ptrdiff_t>
class istream_iterator {
  friend bool operator== __STL_NULL_TMPL_ARGS (const istream_iterator&,
                                               const istream_iterator&);
protected:
  istream* _M_stream;
  _T _M_value;
  bool _M_end_marker;
  void _M_read() {
    _M_end_marker = (*_M_stream) ? true : false;
    if (_M_end_marker) *_M_stream >> _M_value;
    _M_end_marker = (*_M_stream) ? true : false;
  }
public:
  typedef input_iterator_tag  iterator_category;
  typedef _T                  value_type;
  typedef _Dist               difference_type;
  typedef const _T*           pointer;
  typedef const _T&           reference;

  istream_iterator() : _M_stream(&cin), _M_end_marker(false) {}
  istream_iterator(istream& __s) : _M_stream(&__s) { _M_read(); }
  reference operator*() const { return _M_value; }
#ifndef __SGI_STL_NO_ARROW_OPERATOR
  pointer operator->() const { return &(operator*()); }
#endif /* __SGI_STL_NO_ARROW_OPERATOR */
  istream_iterator<_T, _Dist>& operator++() {
    _M_read();
    return *this;
  }
  istream_iterator<_T, _Dist> operator++(int)  {
    istream_iterator<_T, _Dist> __tmp = *this;
    _M_read();
    return __tmp;
  }
};

#ifndef __STL_CLASS_PARTIAL_SPECIALIZATION

template <class _T, class _Dist>
inline input_iterator_tag
iterator_category(const istream_iterator<_T, _Dist>&)
{
  return input_iterator_tag();
}

template <class _T, class _Dist>
inline _T*
value_type(const istream_iterator<_T, _Dist>&) { return (_T*) 0; }

template <class _T, class _Dist>
inline _Dist*
distance_type(const istream_iterator<_T, _Dist>&) { return (_Dist*)0; }

#endif /* __STL_CLASS_PARTIAL_SPECIALIZATION */

template <class _T, class _Distance>
inline bool operator==(const istream_iterator<_T, _Distance>& __x,
                       const istream_iterator<_T, _Distance>& __y) {
  return (__x._M_stream == __y._M_stream &&
          __x._M_end_marker == __y._M_end_marker) ||
         __x._M_end_marker == false && __y._M_end_marker == false;
}

template <class _T>
class ostream_iterator {
protected:
  ostream* _M_stream;
  const char* _M_string;
public:
  typedef output_iterator_tag iterator_category;
  typedef void                value_type;
  typedef void                difference_type;
  typedef void                pointer;
  typedef void                reference;

  ostream_iterator(ostream& __s) : _M_stream(&__s), _M_string(0) {}
  ostream_iterator(ostream& __s, const char* __c)
    : _M_stream(&__s), _M_string(__c)  {}
  ostream_iterator<_T>& operator=(const _T& __value) {
    *_M_stream << __value;
    if (_M_string) *_M_stream << _M_string;
    return *this;
  }
  ostream_iterator<_T>& operator*() { return *this; }
  ostream_iterator<_T>& operator++() { return *this; }
  ostream_iterator<_T>& operator++(int) { return *this; }
};

#ifndef __STL_CLASS_PARTIAL_SPECIALIZATION

template <class _T>
inline output_iterator_tag
iterator_category(const ostream_iterator<_T>&) {
  return output_iterator_tag();
}

#endif /* __STL_CLASS_PARTIAL_SPECIALIZATION */

__STL_END_NAMESPACE

#endif /* __SGI_STL_INTERNAL_ITERATOR_H */

// Local Variables:
// mode:C++
// End:
