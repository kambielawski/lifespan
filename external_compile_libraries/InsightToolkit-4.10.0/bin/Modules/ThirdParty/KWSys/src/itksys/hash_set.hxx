/*============================================================================
  KWSys - Kitware System Library
  Copyright 2000-2009 Kitware, Inc., Insight Software Consortium

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/
/*
 * Copyright (c) 1996
 * Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Silicon Graphics makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
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
 */
#ifndef itksys_hash_set_hxx
#define itksys_hash_set_hxx

#include <itksys/hashtable.hxx>
#include <itksys/hash_fun.hxx>
#include <functional> // equal_to

#if defined(_MSC_VER)
# pragma warning (push)
# pragma warning (disable:4284)
# pragma warning (disable:4786)
#endif

#if defined(__sgi) && !defined(__GNUC__) && (_MIPS_SIM != _MIPS_SIM_ABI32)
# pragma set woff 1174
# pragma set woff 1375
#endif

namespace itksys
{

// identity is an extension: it is not part of the standard.
template <class _Tp>
struct _Identity : public std::unary_function<_Tp,_Tp>
{
  const _Tp& operator()(const _Tp& __x) const { return __x; }
};

// Forward declaration of equality operator; needed for friend declaration.

template <class _Value,
          class _HashFcn  = hash<_Value>,
          class _EqualKey = std::equal_to<_Value>,
          class _Alloc = std::allocator<char> >
class hash_set;

template <class _Value, class _HashFcn, class _EqualKey, class _Alloc>
bool
operator==(const hash_set<_Value,_HashFcn,_EqualKey,_Alloc>& __hs1,
           const hash_set<_Value,_HashFcn,_EqualKey,_Alloc>& __hs2);

template <class _Value, class _HashFcn, class _EqualKey, class _Alloc>
class hash_set
{
private:
  typedef hashtable<_Value, _Value, _HashFcn, _Identity<_Value>,
                    _EqualKey, _Alloc> _Ht;
  _Ht _M_ht;

public:
  typedef typename _Ht::key_type key_type;
  typedef typename _Ht::value_type value_type;
  typedef typename _Ht::hasher hasher;
  typedef typename _Ht::key_equal key_equal;

  typedef typename _Ht::size_type size_type;
  typedef typename _Ht::difference_type difference_type;
  typedef typename _Ht::const_pointer pointer;
  typedef typename _Ht::const_pointer const_pointer;
  typedef typename _Ht::const_reference reference;
  typedef typename _Ht::const_reference const_reference;

  typedef typename _Ht::const_iterator iterator;
  typedef typename _Ht::const_iterator const_iterator;

  typedef typename _Ht::allocator_type allocator_type;

  hasher hash_funct() const { return _M_ht.hash_funct(); }
  key_equal key_eq() const { return _M_ht.key_eq(); }
  allocator_type get_allocator() const { return _M_ht.get_allocator(); }

public:
  hash_set()
    : _M_ht(100, hasher(), key_equal(), allocator_type()) {}
  explicit hash_set(size_type __n)
    : _M_ht(__n, hasher(), key_equal(), allocator_type()) {}
  hash_set(size_type __n, const hasher& __hf)
    : _M_ht(__n, __hf, key_equal(), allocator_type()) {}
  hash_set(size_type __n, const hasher& __hf, const key_equal& __eql,
           const allocator_type& __a = allocator_type())
    : _M_ht(__n, __hf, __eql, __a) {}

  template <class _InputIterator>
  hash_set(_InputIterator __f, _InputIterator __l)
    : _M_ht(100, hasher(), key_equal(), allocator_type())
    { _M_ht.insert_unique(__f, __l); }
  template <class _InputIterator>
  hash_set(_InputIterator __f, _InputIterator __l, size_type __n)
    : _M_ht(__n, hasher(), key_equal(), allocator_type())
    { _M_ht.insert_unique(__f, __l); }
  template <class _InputIterator>
  hash_set(_InputIterator __f, _InputIterator __l, size_type __n,
           const hasher& __hf)
    : _M_ht(__n, __hf, key_equal(), allocator_type())
    { _M_ht.insert_unique(__f, __l); }
  template <class _InputIterator>
  hash_set(_InputIterator __f, _InputIterator __l, size_type __n,
           const hasher& __hf, const key_equal& __eql,
           const allocator_type& __a = allocator_type())
    : _M_ht(__n, __hf, __eql, __a)
    { _M_ht.insert_unique(__f, __l); }

public:
  size_type size() const { return _M_ht.size(); }
  size_type max_size() const { return _M_ht.max_size(); }
  bool empty() const { return _M_ht.empty(); }
  void swap(hash_set& __hs) { _M_ht.swap(__hs._M_ht); }

  friend bool operator==<>(const hash_set&,
                           const hash_set&);

  iterator begin() const { return _M_ht.begin(); }
  iterator end() const { return _M_ht.end(); }

public:
  std::pair<iterator, bool> insert(const value_type& __obj)
    {
      typedef typename _Ht::iterator _Ht_iterator;
      std::pair<_Ht_iterator, bool> __p = _M_ht.insert_unique(__obj);
      return std::pair<iterator,bool>(__p.first, __p.second);
    }
  template <class _InputIterator>
  void insert(_InputIterator __f, _InputIterator __l)
    { _M_ht.insert_unique(__f,__l); }
  std::pair<iterator, bool> insert_noresize(const value_type& __obj)
  {
    typedef typename _Ht::iterator _Ht_iterator;
    std::pair<_Ht_iterator, bool> __p =
      _M_ht.insert_unique_noresize(__obj);
    return std::pair<iterator, bool>(__p.first, __p.second);
  }

  iterator find(const key_type& __key) const { return _M_ht.find(__key); }

  size_type count(const key_type& __key) const { return _M_ht.count(__key); }

  std::pair<iterator, iterator> equal_range(const key_type& __key) const
    { return _M_ht.equal_range(__key); }

  size_type erase(const key_type& __key) {return _M_ht.erase(__key); }
  void erase(iterator __it) { _M_ht.erase(__it); }
  void erase(iterator __f, iterator __l) { _M_ht.erase(__f, __l); }
  void clear() { _M_ht.clear(); }

public:
  void resize(size_type __hint) { _M_ht.resize(__hint); }
  size_type bucket_count() const { return _M_ht.bucket_count(); }
  size_type max_bucket_count() const { return _M_ht.max_bucket_count(); }
  size_type elems_in_bucket(size_type __n) const
    { return _M_ht.elems_in_bucket(__n); }
};

template <class _Value, class _HashFcn, class _EqualKey, class _Alloc>
bool
operator==(const hash_set<_Value,_HashFcn,_EqualKey,_Alloc>& __hs1,
           const hash_set<_Value,_HashFcn,_EqualKey,_Alloc>& __hs2)
{
  return __hs1._M_ht == __hs2._M_ht;
}

template <class _Value, class _HashFcn, class _EqualKey, class _Alloc>
inline bool
operator!=(const hash_set<_Value,_HashFcn,_EqualKey,_Alloc>& __hs1,
           const hash_set<_Value,_HashFcn,_EqualKey,_Alloc>& __hs2) {
  return !(__hs1 == __hs2);
}

template <class _Val, class _HashFcn, class _EqualKey, class _Alloc>
inline void
swap(hash_set<_Val,_HashFcn,_EqualKey,_Alloc>& __hs1,
     hash_set<_Val,_HashFcn,_EqualKey,_Alloc>& __hs2)
{
  __hs1.swap(__hs2);
}

template <class _Value,
          class _HashFcn = hash<_Value>,
          class _EqualKey = std::equal_to<_Value>,
          class _Alloc = std::allocator<char> >
class hash_multiset;

template <class _Val, class _HashFcn, class _EqualKey, class _Alloc>
bool
operator==(const hash_multiset<_Val,_HashFcn,_EqualKey,_Alloc>& __hs1,
           const hash_multiset<_Val,_HashFcn,_EqualKey,_Alloc>& __hs2);


template <class _Value, class _HashFcn, class _EqualKey, class _Alloc>
class hash_multiset
{
private:
  typedef hashtable<_Value, _Value, _HashFcn, _Identity<_Value>,
                    _EqualKey, _Alloc> _Ht;
  _Ht _M_ht;

public:
  typedef typename _Ht::key_type key_type;
  typedef typename _Ht::value_type value_type;
  typedef typename _Ht::hasher hasher;
  typedef typename _Ht::key_equal key_equal;

  typedef typename _Ht::size_type size_type;
  typedef typename _Ht::difference_type difference_type;
  typedef typename _Ht::const_pointer pointer;
  typedef typename _Ht::const_pointer const_pointer;
  typedef typename _Ht::const_reference reference;
  typedef typename _Ht::const_reference const_reference;

  typedef typename _Ht::const_iterator iterator;
  typedef typename _Ht::const_iterator const_iterator;

  typedef typename _Ht::allocator_type allocator_type;

  hasher hash_funct() const { return _M_ht.hash_funct(); }
  key_equal key_eq() const { return _M_ht.key_eq(); }
  allocator_type get_allocator() const { return _M_ht.get_allocator(); }

public:
  hash_multiset()
    : _M_ht(100, hasher(), key_equal(), allocator_type()) {}
  explicit hash_multiset(size_type __n)
    : _M_ht(__n, hasher(), key_equal(), allocator_type()) {}
  hash_multiset(size_type __n, const hasher& __hf)
    : _M_ht(__n, __hf, key_equal(), allocator_type()) {}
  hash_multiset(size_type __n, const hasher& __hf, const key_equal& __eql,
                const allocator_type& __a = allocator_type())
    : _M_ht(__n, __hf, __eql, __a) {}

  template <class _InputIterator>
  hash_multiset(_InputIterator __f, _InputIterator __l)
    : _M_ht(100, hasher(), key_equal(), allocator_type())
    { _M_ht.insert_equal(__f, __l); }
  template <class _InputIterator>
  hash_multiset(_InputIterator __f, _InputIterator __l, size_type __n)
    : _M_ht(__n, hasher(), key_equal(), allocator_type())
    { _M_ht.insert_equal(__f, __l); }
  template <class _InputIterator>
  hash_multiset(_InputIterator __f, _InputIterator __l, size_type __n,
                const hasher& __hf)
    : _M_ht(__n, __hf, key_equal(), allocator_type())
    { _M_ht.insert_equal(__f, __l); }
  template <class _InputIterator>
  hash_multiset(_InputIterator __f, _InputIterator __l, size_type __n,
                const hasher& __hf, const key_equal& __eql,
                const allocator_type& __a = allocator_type())
    : _M_ht(__n, __hf, __eql, __a)
    { _M_ht.insert_equal(__f, __l); }

public:
  size_type size() const { return _M_ht.size(); }
  size_type max_size() const { return _M_ht.max_size(); }
  bool empty() const { return _M_ht.empty(); }
  void swap(hash_multiset& hs) { _M_ht.swap(hs._M_ht); }

  friend bool operator==<>(const hash_multiset&,
                           const hash_multiset&);

  iterator begin() const { return _M_ht.begin(); }
  iterator end() const { return _M_ht.end(); }

public:
  iterator insert(const value_type& __obj)
    { return _M_ht.insert_equal(__obj); }
  template <class _InputIterator>
  void insert(_InputIterator __f, _InputIterator __l)
    { _M_ht.insert_equal(__f,__l); }
  iterator insert_noresize(const value_type& __obj)
    { return _M_ht.insert_equal_noresize(__obj); }

  iterator find(const key_type& __key) const { return _M_ht.find(__key); }

  size_type count(const key_type& __key) const { return _M_ht.count(__key); }

  std::pair<iterator, iterator> equal_range(const key_type& __key) const
    { return _M_ht.equal_range(__key); }

  size_type erase(const key_type& __key) {return _M_ht.erase(__key); }
  void erase(iterator __it) { _M_ht.erase(__it); }
  void erase(iterator __f, iterator __l) { _M_ht.erase(__f, __l); }
  void clear() { _M_ht.clear(); }

public:
  void resize(size_type __hint) { _M_ht.resize(__hint); }
  size_type bucket_count() const { return _M_ht.bucket_count(); }
  size_type max_bucket_count() const { return _M_ht.max_bucket_count(); }
  size_type elems_in_bucket(size_type __n) const
    { return _M_ht.elems_in_bucket(__n); }
};

template <class _Val, class _HashFcn, class _EqualKey, class _Alloc>
bool
operator==(const hash_multiset<_Val,_HashFcn,_EqualKey,_Alloc>& __hs1,
           const hash_multiset<_Val,_HashFcn,_EqualKey,_Alloc>& __hs2)
{
  return __hs1._M_ht == __hs2._M_ht;
}

template <class _Val, class _HashFcn, class _EqualKey, class _Alloc>
inline bool
operator!=(const hash_multiset<_Val,_HashFcn,_EqualKey,_Alloc>& __hs1,
           const hash_multiset<_Val,_HashFcn,_EqualKey,_Alloc>& __hs2) {
  return !(__hs1 == __hs2);
}

template <class _Val, class _HashFcn, class _EqualKey, class _Alloc>
inline void
swap(hash_multiset<_Val,_HashFcn,_EqualKey,_Alloc>& __hs1,
     hash_multiset<_Val,_HashFcn,_EqualKey,_Alloc>& __hs2) {
  __hs1.swap(__hs2);
}

} // namespace itksys

#if defined(__sgi) && !defined(__GNUC__) && (_MIPS_SIM != _MIPS_SIM_ABI32)
# pragma reset woff 1174
# pragma reset woff 1375
#endif

#if defined(_MSC_VER)
# pragma warning (pop)
#endif

#endif
