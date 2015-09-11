// Copyright �2015 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in "bss_util.h"

#ifndef __C_DYN_ARRAY_H__BSS__
#define __C_DYN_ARRAY_H__BSS__

#include "cArray.h"

namespace bss_util {
  // Dynamic array implemented using ArrayType (should only be used when constructors could potentially not be needed)
  template<class T, typename CType=unsigned int, ARRAY_TYPE ArrayType = CARRAY_SIMPLE, typename Alloc=StaticAllocPolicy<T>>
  class BSS_COMPILER_DLLEXPORT cDynArray : protected cArrayBase<T, CType, Alloc>, protected cArrayInternal<T, CType, ArrayType, Alloc>
  {
  protected:
    typedef cArrayBase<T, CType, Alloc> AT_;
    typedef typename AT_::CT_ CT_;
    typedef typename AT_::T_ T_;
    using AT_::_array;
    using AT_::_capacity;

  public:
    inline cDynArray(const cDynArray& copy) : AT_(copy._capacity), _length(copy._length) { _copy(_array, copy._array, _length); }
    inline cDynArray(cDynArray&& mov) : AT_(std::move(mov)), _length(mov._length) { mov._length = 0; }
    inline explicit cDynArray(CT_ capacity=0) : AT_(capacity), _length(0) {}
    inline cDynArray(const std::initializer_list<T> list) : AT_(list.size()), _length(0)
    {
      auto end = list.end();
      for(auto i = list.begin(); i != end && _length < _capacity; ++i)
        new(_array + (_length++)) T(*i);
    }
    inline ~cDynArray() { _setlength(_array, _length, 0); }
    BSS_FORCEINLINE CT_ Add(const T_& t) { _checksize(); new(_array + _length) T(t); return _length++; }
    BSS_FORCEINLINE CT_ Add(T_&& t) { _checksize(); new(_array + _length) T(std::move(t)); return _length++; }
    inline void Remove(CT_ index)
    {
      assert(index < _length);
      _move(_array, index, index + 1, _length - index - 1);
      _array[--_length].~T();
    }
    BSS_FORCEINLINE void RemoveLast() { Remove(_length - 1); }
    BSS_FORCEINLINE void Insert(const T_& t, CT_ index=0) { _insert(t, index); }
    BSS_FORCEINLINE void Insert(T_&& t, CT_ index=0) { _insert(std::move(t), index); }
    BSS_FORCEINLINE bool Empty() const { return !_length; }
    BSS_FORCEINLINE void Clear() { SetLength(0); }
    inline void SetLength(CT_ length)
    { 
      if(length < _length) _setlength(_array, _length, length);
      if(length > _capacity) _setcapacity(*this, length);
      if(length > _length) _setlength(_array, _length, length);
      _length = length;
    }
    inline void Reserve(CT_ capacity) { if(capacity>_capacity) _setcapacity(*this, capacity); }
    BSS_FORCEINLINE CT_ Length() const { return _length; }
    BSS_FORCEINLINE CT_ Capacity() const { return _capacity; }
    BSS_FORCEINLINE const T_& Front() const { assert(_length>0); return _array[0]; }
    BSS_FORCEINLINE const T_& Back() const { assert(_length>0); return _array[_length-1]; }
    BSS_FORCEINLINE T_& Front() { assert(_length>0); return _array[0]; }
    BSS_FORCEINLINE T_& Back() { assert(_length>0); return _array[_length-1]; }
    BSS_FORCEINLINE const T_* begin() const { return _array; }
    BSS_FORCEINLINE const T_* end() const { return _array+_length; }
    BSS_FORCEINLINE T_* begin() { return _array; }
    BSS_FORCEINLINE T_* end() { return _array+_length; }

    BSS_FORCEINLINE operator T_*() { return _array; }
    BSS_FORCEINLINE operator const T_*() const { return _array; }
    inline cDynArray& operator=(const cArray<T, CType, ArrayType, Alloc>& copy)
    { 
      _setlength(_array, _length, 0);
      if(copy._capacity > _capacity) 
        AT_::SetCapacityDiscard(copy._capacity);
      _copy(_array, copy._array, copy._capacity);
      _length = copy._capacity;
      return *this;
    }
    inline cDynArray& operator=(cArray<T, CType, ArrayType, Alloc>&& mov) { _setlength(_array, _length, 0); AT_::operator=(std::move(mov)); _length = mov._capacity; return *this; }
    inline cDynArray& operator=(const cDynArray& copy)
    {
      _setlength(_array, _length, 0);
      if(copy._length > _capacity)
        AT_::SetCapacityDiscard(copy._capacity);
      _copy(_array, copy._array, copy._length);
      _length = copy._length;
      return *this;
    }
    inline cDynArray& operator=(cDynArray&& mov) { _setlength(_array, _length, 0); AT_::operator=(std::move(mov)); _length = mov._length; return *this; }
    inline cDynArray& operator +=(const cDynArray& add)
    { 
      _setcapacity(*this, _length + add._length);
      _copy(_array + _length, add._array, add._length);
      _length+=add._length;
      return *this;
    }
    inline cDynArray operator +(const cDynArray& add) const
    {
      cDynArray r(_length + add._length);
      _copy(r._array, _array, _length);
      _copy(r._array + _length, add._array, add._length);
      r._length = _length + add._length;
      return r;
    }

  protected:
    template<typename U>
    inline void _insert(U && data, CType index)
    {
      _checksize();
      if(index < _length)
      {
        new(_array + _length) T(std::forward<U>(_array[_length - 1]));
        _move(_array, index + 1, index, _length - index - 1);
        _array[index] = std::forward<U>(data);
      }
      else
        new(_array + index) T(std::forward<U>(data));
      ++_length;
    }
    BSS_FORCEINLINE void _checksize()
    {
      if(_length >= _capacity)
        _setcapacity(*this, T_FBNEXT(_capacity));
      assert(_length<_capacity);
    }

    CT_ _length;
  };

  // A dynamic array that can dynamically adjust the size of each element
  template<typename CT_, typename Alloc=StaticAllocPolicy<unsigned char>>
  class BSS_COMPILER_DLLEXPORT cArbitraryArray : protected cArrayBase<unsigned char, CT_, Alloc>
  {
  protected:
    typedef cArrayBase<unsigned char, CT_, Alloc> AT_;
    using AT_::_array;
    using AT_::_capacity;

  public:
    inline cArbitraryArray(const cArbitraryArray& copy) : AT_(copy._capacity), _length(copy._length), _element(copy._element) { memcpy(_array, copy._array, _length*_element); }
    inline cArbitraryArray(cArbitraryArray&& mov) : AT_(std::move(mov)), _length(mov._length), _element(mov._element) {}
    inline cArbitraryArray(CT_ size=0, CT_ element=1): AT_(size*element), _length(0), _element(element) {}
    template<typename T>
    inline CT_ Add(const T& t)
    {
      if((_length*_element)>=_capacity) AT_::SetCapacity(T_FBNEXT(_length)*_element);
      memcpy(_array+(_length*_element), &t, bssmin(sizeof(T), _element));
      return _length++;
    }
    inline void Remove(CT_ index)
    {
      index *= _element;
      assert(_capacity>0 && index<_capacity);
      memmove(_array+index, _array+index+_element, _capacity-index-_element);
      --_length;
    }
    inline void RemoveLast() { --_length; }
    inline void SetElement(const void* newarray, CT_ element, CT_ num) // num is a count of how many elements are in the array
    {
      if(((unsigned char*)newarray)==_array) return;
      _element=element;
      _length=num;
      if((_length*_element)>_capacity) AT_::SetCapacityDiscard(_length*_element);
      if(_array)
        memcpy(_array, newarray, element*num);
    }
    template<typename T> // num is a count of how many elements are in the array
    BSS_FORCEINLINE void SetElement(const T* newarray, CT_ num) { SetElement(newarray, sizeof(T), num); }
    template<typename T, unsigned int NUM>
    BSS_FORCEINLINE void SetElement(const T(&newarray)[NUM]) { SetElement(newarray, sizeof(T), NUM); }
    void SetElement(CT_ element)
    {
      if(element==_element) return;
      _capacity=element*_length;
      unsigned char* narray = !_length?0:(unsigned char*)Alloc::allocate(_capacity);
      memset(narray, 0, _capacity);
      CT_ m=bssmin(element, _element);
      for(unsigned int i = 0; i < _length; ++i)
        memcpy(narray+(i*element), _array+(i*_element), m);
      _free(_array);
      _array=narray;
      _element=element;
      assert(element>0);
    }
    inline CT_ Element() const { return _element; }
    inline bool Empty() const { return !_length; }
    inline void Clear() { _length=0; }
    inline void SetLength(CT_ length) { _length=length; length*=_element; if(length>_capacity) AT_::SetCapacity(length); }
    inline CT_ Length() const { return _length; }
    // These are templatized accessors, but we don't do an assertion on if the size of T is the same as _element, because there are many cases where this isn't actually true.
    template<typename T> inline const T& Front() const { assert(_length>0); return _array[0]; }
    template<typename T> inline const T& Back() const { assert(_length>0); return _array[(_length-1)*_element]; }
    template<typename T> inline T& Front() { assert(_length>0); return _array[0]; }
    template<typename T> inline T& Back() { assert(_length>0); return _array[(_length-1)*_element]; }
    template<typename T> inline const T* begin() const { return _array; }
    template<typename T> inline const T* end() const { return _array+(_length*_element); }
    template<typename T> inline T* begin() { return _array; }
    template<typename T> inline T* end() { return _array+(_length*_element); }
    template<typename T> inline const T& Get(CT_ index) const { return *(T*)Get(index); }
    template<typename T> inline T& Get(CT_ index) { return *(T*)Get(index); }
    BSS_FORCEINLINE const void* Get(CT_ index) const { assert(index<_length); return (_array+(index*_element)); }
    BSS_FORCEINLINE void* Get(CT_ index) { assert(index<_length); return (_array+(index*_element)); }

    inline const void* operator[](CT_ index) const { return Get(index); }
    inline void* operator[](CT_ index) { return Get(index); }
    inline cArbitraryArray& operator=(const cArbitraryArray& copy) { SetCapacityDiscard(copy._capacity); memcpy(_array, copy._array, _capacity); _length = mov._length; _element = mov._element; return *this; }
    inline cArbitraryArray& operator=(cArbitraryArray&& mov) { AT_::operator=(std::move(mov)); _length = mov._length; _element = mov._element; return *this; }

  protected:
    CT_ _length; // Total length of the array in number of elements
    CT_ _element; // Size of each element
  };
}

#endif
