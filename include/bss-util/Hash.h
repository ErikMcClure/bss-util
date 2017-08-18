// Copyright ©2017 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in "bss_util.h"

#ifndef __HASH_H__BSS__
#define __HASH_H__BSS__

#include "khash.h"
#include "bss_util.h"
#include "Array.h"
#include "Str.h"
#include <wchar.h>
#include <utility>
#include <iterator>

namespace bss {
  template<class Engine>
  class Serializer;

  namespace internal {
    template<class T, bool value>
    struct _HashBaseGET { typedef T GET; static BSS_FORCEINLINE GET F(T& s) { return s; } };

    template<class T>
    struct _HashBaseGET<T, false> { typedef T* GET; static BSS_FORCEINLINE GET F(T& s) { return &s; } };

    template<>
    struct _HashBaseGET<Str, false> { typedef const char* GET; static BSS_FORCEINLINE GET F(Str& s) { return s.c_str(); } };

#ifdef BSS_PLATFORM_WIN32
    template<>
    struct _HashBaseGET<StrW, false> { typedef const wchar_t* GET; static BSS_FORCEINLINE GET F(StrW& s) { return s.c_str(); } };
#endif
  }

  template<class Key, class Data, khint_t(*__hash_func)(const Key&), bool(*__hash_equal)(const Key&, const Key&), ARRAY_TYPE ArrayType = ARRAY_SIMPLE, typename Alloc = StaticAllocPolicy<char>>
  class HashBase
  {
  public:
    static constexpr bool IsMap = !std::is_void<Data>::value;
    typedef Key KEY;
    typedef Data DATA;
    typedef typename std::conditional<IsMap, Data, char>::type FakeData;
    typedef typename internal::_HashBaseGET<FakeData, std::is_integral<FakeData>::value | std::is_pointer<FakeData>::value | std::is_member_pointer<FakeData>::value>::GET GET;

    HashBase(const HashBase& copy)
    {
      bssFill(*this, 0);
      operator=(copy);
    }
    HashBase(HashBase&& mov)
    {
      memcpy(this, &mov, sizeof(HashBase));
      bssFill(mov, 0);
    }
    explicit HashBase(khint_t n_buckets = 0)
    {
      bssFill(*this, 0);
      if(n_buckets > 0)
        _resize(n_buckets);
    }
    ~HashBase()
    {
      Clear(); // calls all destructors.
      if(flags) 
        Alloc::deallocate((char*)flags);
      if(keys) 
        Alloc::deallocate((char*)keys);
      if(vals) 
        Alloc::deallocate((char*)vals);
    }

    template<bool U = IsMap>
    inline typename std::enable_if<U, khiter_t>::type Insert(const Key& key, const FakeData& value) { return _insert<const Key&, const Data&>(key, value); }
    template<bool U = IsMap>
    inline typename std::enable_if<U, khiter_t>::type Insert(const Key& key, FakeData&& value) { return _insert<const Key&, Data&&>(key, std::move(value)); }
    template<bool U = IsMap>
    inline typename std::enable_if<U, khiter_t>::type Insert(Key&& key, const FakeData& value) { return _insert<Key&&, const Data&>(std::move(key), value); }
    template<bool U = IsMap>
    inline typename std::enable_if<U, khiter_t>::type Insert(Key&& key, FakeData&& value) { return _insert<Key&&, Data&&>(std::move(key), std::move(value)); }
    template<bool U = IsMap>
    inline typename std::enable_if<!U, khiter_t>::type Insert(const Key& key) { int r; return _put<const Key&>(key, &r); }
    template<bool U = IsMap>
    inline typename std::enable_if<!U, khiter_t>::type Insert(Key&& key) { int r; return _put<Key&&>(std::move(key), &r); }

    void Clear()
    {
      if(flags)
      {
        for(khint_t i = 0; i < n_buckets; ++i)
        {
          if(!__ac_iseither(flags, i))
          {
            keys[i].~Key();

            if constexpr(IsMap)
              vals[i].~Data();
          }
        }
        memset(flags, 2, n_buckets);
        size = n_occupied = 0;
      }
    }
    inline khiter_t Iterator(const Key& key) const { return _get(key); }
    inline const Key& GetKey(khiter_t i) { return keys[i]; }
    template<bool U = IsMap>
    inline typename std::enable_if<U, GET>::type GetValue(khiter_t i) const
    {
      if(!ExistsIter(i))
      {
        if constexpr(std::is_integral<GET>::value)
          return (GET)~0;
        else
          return (GET)0;
      }
      return internal::_HashBaseGET<Data, std::is_integral<Data>::value | std::is_pointer<Data>::value | std::is_member_pointer<Data>::value>::F(vals[i]);
    }
    template<bool U = IsMap>
    inline typename std::enable_if<U, GET>::type Get(const Key& key) const { return GetValue(Iterator(key)); }
    template<bool U = IsMap>
    inline typename std::enable_if<U, FakeData&>::type UnsafeValue(khiter_t i) const { return vals[i]; }
    template<bool U = IsMap>
    inline typename std::enable_if<U, FakeData&>::type& MutableValue(khiter_t i) { return vals[i]; }
    template<bool U = IsMap>
    inline typename std::enable_if<U, FakeData>::type* PointerValue(khiter_t i)
    {
      if(!ExistsIter(i))
        return nullptr;
      return &vals[i]; 
    }
    template<bool U = IsMap>
    inline typename std::enable_if<U, bool>::type SetValue(khiter_t iterator, const FakeData& newvalue) { return _setvalue<const Data&>(iterator, newvalue); }
    template<bool U = IsMap>
    inline typename std::enable_if<U, bool>::type SetValue(khiter_t iterator, FakeData&& newvalue) { return _setvalue<Data&&>(iterator, std::move(newvalue)); }
    template<bool U = IsMap>
    inline typename std::enable_if<U, bool>::type Set(const Key& key, const FakeData& newvalue) { return _setvalue<const Data&>(Iterator(key), newvalue); }
    template<bool U = IsMap>
    inline typename std::enable_if<U, bool>::type Set(const Key& key, FakeData&& newvalue) { return _setvalue<Data&&>(Iterator(key), std::move(newvalue)); }
    inline void SetCapacity(khint_t size) { if(n_buckets < size) _resize(size); }
    inline bool Remove(const Key& key)
    {
      khiter_t iterator = Iterator(key);
      if(n_buckets == iterator) // This isn't ExistsIter because _get will return n_buckets if key doesn't exist
        return false; 

      _delete(iterator);
      return true;
    }
    inline bool RemoveIter(khiter_t iterator)
    {
      if(!ExistsIter(iterator))
        return false;

      _delete(iterator);
      return true;
    }
    inline khint_t Length() const { return size; }
    inline khint_t Capacity() const { return n_buckets; }
    inline khiter_t Start() const { return 0; }
    inline khiter_t End() const { return n_buckets; }
    inline bool ExistsIter(khiter_t iterator) const { return (iterator < n_buckets) && !__ac_iseither(flags, iterator); }
    inline bool Exists(const Key& key) const { return ExistsIter(Iterator(key)); }
    template<bool U = IsMap>
    inline typename std::enable_if<U, GET>::type operator[](const Key& key) const { return Get(key); }
    inline bool operator()(const Key& key) const { return Exists(key); }
    template<bool U = IsMap>
    inline typename std::enable_if<U, bool>::type operator()(const Key& key, FakeData& v) const
    { 
      khiter_t i = Iterator(key); 

      if(!ExistsIter(i)) 
        return false; 

      v = vals[i];
      return true; 
    }

    HashBase& operator =(const HashBase& copy)
    {
      Clear();
      if(!copy.n_buckets)
      {
        if(flags) Alloc::deallocate((char*)flags);
        if(keys) Alloc::deallocate((char*)keys);
        if(vals) Alloc::deallocate((char*)vals);
        bssFill(*this, 0);
        return *this;
      }
      _resize(copy.n_buckets);
      assert(n_buckets == copy.n_buckets);
      memcpy(flags, copy.flags, n_buckets);

      for(khint_t i = 0; i < n_buckets; ++i)
      {
        if(!__ac_iseither(flags, i))
        {
          new(keys + i) Key((const Key&)copy.keys[i]);

          if constexpr(IsMap)
            new(vals + i) Data((const Data&)copy.vals[i]);
        }
      }

      assert(n_buckets == copy.n_buckets);
      size = copy.size;
      n_occupied = copy.n_occupied;
      upper_bound = copy.upper_bound;
      return *this;
    }
    HashBase& operator =(HashBase&& mov)
    {
      Clear();
      if(flags) Alloc::deallocate((char*)flags);
      if(keys) Alloc::deallocate((char*)keys);
      if(vals) Alloc::deallocate((char*)vals);
      memcpy(this, &mov, sizeof(HashBase));
      bssFill(mov);
      return *this;
    }

    class BSS_TEMPLATE_DLLEXPORT cHash_Iter : public std::iterator<std::bidirectional_iterator_tag, khiter_t>
    {
    public:
      inline explicit cHash_Iter(const HashBase& src) : _src(&src), cur(0) { _chknext(); }
      inline cHash_Iter(const HashBase& src, khiter_t start) : _src(&src), cur(start) { _chknext(); }
      inline khiter_t operator*() const { return cur; }
      inline cHash_Iter& operator++() { ++cur; _chknext(); return *this; } //prefix
      inline cHash_Iter operator++(int) { cHash_Iter r(*this); ++*this; return r; } //postfix
      inline cHash_Iter& operator--() { while((--cur) < _src->End() && !_src->ExistsIter(cur)); return *this; } //prefix
      inline cHash_Iter operator--(int) { cHash_Iter r(*this); --*this; return r; } //postfix
      inline bool operator==(const cHash_Iter& _Right) const { return (cur == _Right.cur); }
      inline bool operator!=(const cHash_Iter& _Right) const { return (cur != _Right.cur); }
      inline bool operator!() const { return !IsValid(); }
      inline bool IsValid() { return cur < _src->End(); }

      khiter_t cur; //khiter_t is unsigned (this is why operator--() works)

    protected:
      inline void _chknext() { while(cur < _src->End() && !_src->ExistsIter(cur)) ++cur; }

      const HashBase* _src;
    };

    inline cHash_Iter begin() const { return cHash_Iter(*this, Start()); }
    inline cHash_Iter end() const { return cHash_Iter(*this, End()); }

    template<typename Engine>
    void Serialize(Serializer<Engine>& e)
    {
      if constexpr(IsMap)
      {
        if(e.out)
        {
          for(auto i : *this)
            Serializer<Engine>::template ActionBind<Data>::Serialize(e, vals[i], ToString<Key>(keys[i]).c_str());
        }
      }
    }

  protected:
    template<typename U, typename V>
    inline khiter_t _insert(U && key, V && value)
    {
      int r;
      khiter_t i = _put<const Key&>(std::forward<U>(key), &r);
      if(!r) // If r is 0, this key was already present, so we need to assign, not initialize
        vals[i] = std::forward<V>(value);
      else
        new(vals + i) Data(std::forward<V>(value));
      return i;
    }
    template<typename U>
    inline bool _setvalue(khiter_t i, U && newvalue) { if(!ExistsIter(i)) return false; vals[i] = std::forward<U>(newvalue); return true; }
    char _resize(khint_t new_n_buckets)
    {
      khint8_t *new_flags = 0;
      khint_t j = 1;
      {
        kroundup32(new_n_buckets);
        if(new_n_buckets < 4) new_n_buckets = 32;
        if(size >= (khint_t)(new_n_buckets * __ac_HASH_UPPER + 0.5)) j = 0;	/* requested size is too small */
        else
        { /* hash table size to be changed (shrink or expand); rehash */
          new_flags = (khint8_t*)Alloc::allocate(new_n_buckets);
          if(!new_flags) return -1;
          memset(new_flags, 2, new_n_buckets);
          if(n_buckets < new_n_buckets)
          {	/* expand */
            Key *new_keys = _realloc<Key>(flags, keys, new_n_buckets, n_buckets);
            if(!new_keys) { Alloc::deallocate((char*)new_flags); return -1; }
            keys = new_keys;
            if constexpr(IsMap)
            {
              Data *new_vals = _realloc<Data>(flags, vals, new_n_buckets, n_buckets);
              if(!new_vals) { Alloc::deallocate((char*)new_flags); return -1; }
              vals = new_vals;
            }
          } /* otherwise shrink */
        }
      }
      if(j)
      { /* rehashing is needed */
        for(j = 0; j != n_buckets; ++j)
        {
          if(__ac_iseither(flags, j) == 0)
          {
            Key key(std::move(keys[j]));
            [[maybe_unused]] FakeData val;
            khint_t new_mask;
            new_mask = new_n_buckets - 1;
            if constexpr(IsMap) val = std::move(vals[j]);
            __ac_set_isdel_true(flags, j);
            while(1)
            { /* kick-out process; sort of like in Cuckoo hashing */
              khint_t k, i, step = 0;
              k = __hash_func(key);
              i = k & new_mask;
              while(!__ac_isempty(new_flags, i)) i = (i + (++step)) & new_mask;
              __ac_set_isempty_false(new_flags, i);
              if(i < n_buckets && __ac_iseither(flags, i) == 0)
              { /* kick out the existing element */
                std::swap(keys[i], key);
                if constexpr(IsMap) std::swap(vals[i], val);
                __ac_set_isdel_true(flags, i); /* mark it as deleted in the old hash table */
              }
              else // this code only runs if this bucket doesn't exist, so initialize
              {
                new(keys + i) Key(std::move(key));
                if constexpr(IsMap) new(vals + i) Data(std::move(val));
                break;
              }
            }
          }
        }
        if(n_buckets > new_n_buckets)
        { /* shrink the hash table */
          keys = _realloc<Key>(flags, keys, new_n_buckets, n_buckets);
          if constexpr(IsMap)
            vals = _realloc<Data>(flags, vals, new_n_buckets, n_buckets);
        }
        Alloc::deallocate((char*)flags); /* free the working space */
        flags = new_flags;
        n_buckets = new_n_buckets;
        n_occupied = size;
        upper_bound = (khint_t)(n_buckets * __ac_HASH_UPPER + 0.5);
      }
      return 0;
    }

    template<typename U>
    khint_t _put(U && key, int* ret)
    {
      khint_t x;
      if(n_occupied >= upper_bound)
      { /* update the hash table */
        if(n_buckets > (size << 1))
        {
          \
            if(_resize(n_buckets - 1) < 0)
            { /* clear "deleted" elements */
              *ret = -1; return n_buckets;
            }
        }
        else if(_resize(n_buckets + 1) < 0)
        { /* expand the hash table */
          *ret = -1; return n_buckets;
        }
      } /* TODO: to implement automatically shrinking; resize() already support shrinking */
      {
        khint_t k, i, site, last, mask = n_buckets - 1, step = 0;
        x = site = n_buckets; k = __hash_func(key); i = k & mask;
        if(__ac_isempty(flags, i)) x = i; /* for speed up */
        else
        {
          last = i;
          while(!__ac_isempty(flags, i) && (__ac_isdel(flags, i) || !__hash_equal(keys[i], key)))
          {
            if(__ac_isdel(flags, i)) site = i;
            i = (i + (++step)) & mask;
            if(i == last) { x = site; break; }
          }
          if(x == n_buckets)
          {
            if(__ac_isempty(flags, i) && site != n_buckets) x = site;
            else x = i;
          }
        }
      }
      if(__ac_isempty(flags, x))
      { /* not present at all */
        new(keys + x) Key(std::move(key));
        __ac_set_isboth_false(flags, x);
        ++size; ++n_occupied;
        *ret = 1;
      }
      else if(__ac_isdel(flags, x))
      { /* deleted */
        new(keys + x) Key(std::move(key));
        __ac_set_isboth_false(flags, x);
        ++size;
        *ret = 2;
      }
      else *ret = 0; /* Don't touch keys[x] if present and not deleted */
      return x;
    }
    khint_t _get(const Key& key) const
    {
      if(n_buckets)
      {
        khint_t k, i, last, mask, step = 0;
        mask = n_buckets - 1;
        k = __hash_func(key); i = k & mask;
        last = i;
        while(!__ac_isempty(flags, i) && (__ac_isdel(flags, i) || !__hash_equal(keys[i], key)))
        {
          i = (i + (++step)) & mask;
          if(i == last) return n_buckets;
        }
        return __ac_iseither(flags, i) ? n_buckets : i;
      }
      else return 0;
    }
    inline void _delete(khint_t x)
    {
      if(x != n_buckets && !__ac_iseither(flags, x))
      {
        keys[x].~Key();
        if constexpr(IsMap)
          vals[x].~Data();
        __ac_set_isdel_true(flags, x);
        --size;
      }
    }
    template<typename T>
    inline static T* _realloc(khint8_t* flags, T* src, khint_t new_n_buckets, khint_t n_buckets) noexcept
    {
      if constexpr(ArrayType == ARRAY_SIMPLE || ArrayType == ARRAY_CONSTRUCT)
        return (T*)Alloc::allocate(new_n_buckets * sizeof(T), (char*)src);
      else if(ArrayType == ARRAY_SAFE || ArrayType == ARRAY_MOVE)
      {
        T* n = (T*)Alloc::allocate(new_n_buckets * sizeof(T), 0);
        if(n != nullptr)
        {
          for(khint_t i = 0; i < n_buckets; ++i)
          {
            if(!__ac_iseither(flags, i))
              new(n + i) T(std::move(src[i]));
          }
        }

        if(src)
          Alloc::deallocate((char*)src);

        return n;
      }
    }

    khint_t n_buckets, size, n_occupied, upper_bound;
    khint8_t* flags;
    Key* keys;
    Data* vals;
  };

  template<typename T>
  BSS_FORCEINLINE bool KH_DEFAULT_EQUAL(T const& a, T const& b) { return a == b; }
  template<class T> 
  BSS_FORCEINLINE khint_t KH_INT_HASH(T key)
  { 
    if constexpr(sizeof(T) == sizeof(khint64_t))
    {
      khint64_t i = *reinterpret_cast<const khint64_t*>(&key);
      return kh_int64_hash_func(i);
    }
    else if constexpr(sizeof(T) == sizeof(khint_t))
    {
      khint_t i = *reinterpret_cast<const khint_t*>(&key);
      return kh_int_hash_func2(i);
    }
    else
      return static_cast<const khint32_t>(key);
  }
  template<class T, bool IgnoreCase>
  inline khint_t KH_STR_HASH(const T* s) = delete;

  template<>
  inline khint_t KH_STR_HASH<char, false>(const char * s)
  {
    khint_t h = *s; 
    if(h) 
      for(++s; *s; ++s) 
        h = (h << 5) - h + *s;
    return h; 
  }
  template<>
  inline khint_t KH_STR_HASH<char, true>(const char *s)
  { 
    khint_t h = ((*s) > 64 && (*s) < 91) ? (*s) + 32 : *s;	
    if(h) 
      for(++s; *s; ++s) 
        h = (h << 5) - h + (((*s) > 64 && (*s) < 91) ? (*s) + 32 : *s);
    return h;
  }
  template<>
  inline khint_t KH_STR_HASH<wchar_t, false>(const wchar_t * s)
  { 
    khint_t h = *s; 
    if(h) 
      for(++s; *s; ++s) 
        h = (h << 5) - h + *s; 
    return h; 
  }
  template<>
  inline khint_t KH_STR_HASH<wchar_t, true>(const wchar_t *s)
  { 
    khint_t h = towlower(*s);
    if(h) 
      for(++s; *s; ++s) 
        h = (h << 5) - h + towlower(*s);
    return h; 
  }

  namespace internal {
    template<typename T>
    BSS_FORCEINLINE khint_t KH_AUTO_HASH(const T& k) { return KH_INT_HASH<T>(k); }
    template<> BSS_FORCEINLINE khint_t KH_AUTO_HASH<std::string>(const std::string& k) { return KH_STR_HASH<char, false>(k.c_str()); }
    template<> BSS_FORCEINLINE khint_t KH_AUTO_HASH<Str>(const Str& k) { return KH_STR_HASH<char, false>(k.c_str()); }
    template<> BSS_FORCEINLINE khint_t KH_AUTO_HASH<const char*>(const char* const& k) { return KH_STR_HASH<char, false>(k); }
    template<> BSS_FORCEINLINE khint_t KH_AUTO_HASH<char*>(char* const& k) { return KH_STR_HASH<char, false>(k); }
    template<> BSS_FORCEINLINE khint_t KH_AUTO_HASH<const wchar_t*>(const wchar_t* const& k) { return KH_STR_HASH<wchar_t, false>(k); }
    template<> BSS_FORCEINLINE khint_t KH_AUTO_HASH<wchar_t*>(wchar_t* const& k) { return KH_STR_HASH<wchar_t, false>(k); }
    template<> BSS_FORCEINLINE khint_t KH_AUTO_HASH<double>(const double& k) { return KH_INT_HASH(k); }
    template<> BSS_FORCEINLINE khint_t KH_AUTO_HASH<float>(const float& k) { return KH_INT_HASH(k); }
    template<typename T> BSS_FORCEINLINE khint_t KH_AUTOINS_HASH(const T& k) { return KH_STR_HASH<char, true>(k); }
    template<> BSS_FORCEINLINE khint_t KH_AUTOINS_HASH<std::string>(const std::string& k) { return KH_STR_HASH<char, true>(k.c_str()); }
    template<> BSS_FORCEINLINE khint_t KH_AUTOINS_HASH<Str>(const Str& k) { return KH_STR_HASH<char, true>(k.c_str()); }
    template<> BSS_FORCEINLINE khint_t KH_AUTOINS_HASH<const wchar_t*>(const wchar_t* const& k) { return KH_STR_HASH<wchar_t, true>(k); }
    template<> BSS_FORCEINLINE khint_t KH_AUTOINS_HASH<wchar_t*>(wchar_t* const& k) { return KH_STR_HASH<wchar_t, true>(k); }

    template<typename T>
    BSS_FORCEINLINE bool KH_AUTO_EQUAL(T const& a, T const& b) { return a == b; }
    template<> BSS_FORCEINLINE bool KH_AUTO_EQUAL<std::string>(std::string const& a, std::string const& b) { return strcmp(a.c_str(), b.c_str()) == 0; }
    template<> BSS_FORCEINLINE bool KH_AUTO_EQUAL<Str>(Str const& a, Str const& b) { return strcmp(a, b) == 0; }
    template<> BSS_FORCEINLINE bool KH_AUTO_EQUAL<const char*>(const char* const& a, const char* const& b) { return strcmp(a, b) == 0; }
    template<> BSS_FORCEINLINE bool KH_AUTO_EQUAL<const wchar_t*>(const wchar_t* const& a, const wchar_t* const& b) { return wcscmp(a, b) == 0; }
    template<typename T> BSS_FORCEINLINE bool KH_AUTOINS_EQUAL(T const& a, T const& b) { return STRICMP(a, b) == 0; }
    template<> BSS_FORCEINLINE bool KH_AUTOINS_EQUAL<std::string>(std::string const& a, std::string const& b) { return STRICMP(a.c_str(), b.c_str()) == 0; }
    template<> BSS_FORCEINLINE bool KH_AUTOINS_EQUAL<Str>(Str const& a, Str const& b) { return STRICMP(a, b) == 0; }
    template<> BSS_FORCEINLINE bool KH_AUTOINS_EQUAL<const wchar_t*>(const wchar_t* const& a, const wchar_t* const& b) { return WCSICMP(a, b) == 0; }
    template<> BSS_FORCEINLINE bool KH_AUTOINS_EQUAL<wchar_t*>(wchar_t* const& a, wchar_t* const& b) { return WCSICMP(a, b) == 0; }

#ifdef BSS_PLATFORM_WIN32
    template<> BSS_FORCEINLINE khint_t KH_AUTO_HASH<StrW>(const StrW& k) { return KH_STR_HASH<wchar_t, false>(k); }
    template<> BSS_FORCEINLINE khint_t KH_AUTOINS_HASH<StrW>(const StrW& k) { return KH_STR_HASH<wchar_t, true>(k); }
    template<> BSS_FORCEINLINE bool KH_AUTO_EQUAL<StrW>(StrW const& a, StrW const& b) { return wcscmp(a, b) == 0; }
    template<> BSS_FORCEINLINE bool KH_AUTOINS_EQUAL<StrW>(StrW const& a, StrW const& b) { return WCSICMP(a, b) == 0; }
#endif

    template<typename T, bool ins> 
    struct _HashHelper {
      BSS_FORCEINLINE static khint_t hash(const T& k) { return KH_AUTO_HASH<T>(k); }
      BSS_FORCEINLINE static bool equal(const T& a, const T& b) { return KH_AUTO_EQUAL<T>(a, b); }
    };
    template<typename T> 
    struct _HashHelper<T, true> {
      BSS_FORCEINLINE static khint_t hash(const T& k) { return KH_AUTOINS_HASH<T>(k); }
      BSS_FORCEINLINE static bool equal(const T& a, const T& b) { return KH_AUTOINS_EQUAL<T>(a, b); }
    };
    template<typename U, typename V>
    struct _HashHelper<std::pair<U, V>, false> {
      BSS_FORCEINLINE static khint_t hash(const std::pair<U, V>& k) { return KH_INT_HASH<uint64_t>(uint64_t(KH_AUTO_HASH<U>(k.first)) | (uint64_t(KH_AUTO_HASH<V>(k.second)) << 32)); }
      BSS_FORCEINLINE static bool equal(const std::pair<U, V>& a, const std::pair<U, V>& b) { return KH_AUTO_EQUAL<U>(a.first, b.first) && KH_AUTO_EQUAL<V>(a.second, b.second); }
    };
  }

  // Generic hash definition
  template<typename K, typename T = void, bool ins = false, ARRAY_TYPE ArrayType = ARRAY_SIMPLE, typename Alloc = StaticAllocPolicy<char>>
  class BSS_COMPILER_DLLEXPORT Hash : public HashBase<K, T, &internal::_HashHelper<K, ins>::hash, &internal::_HashHelper<K, ins>::equal, ArrayType, Alloc>
  {
  public:
    typedef HashBase<K, T, &internal::_HashHelper<K, ins>::hash, &internal::_HashHelper<K, ins>::equal, ArrayType, Alloc> BASE;

    inline Hash(const Hash& copy) : BASE(copy) {}
    inline Hash(Hash&& mov) : BASE(std::move(mov)) {}
    inline Hash(khint_t size = 0) : BASE(size) {}

    inline Hash& operator =(const Hash& right) { BASE::operator=(right); return *this; }
    inline Hash& operator =(Hash&& mov) { BASE::operator=(std::move(mov)); return *this; }
    inline bool operator()(const K& key) const { return BASE::operator()(key); }
    template<bool U = !std::is_void<T>::value>
    inline typename std::enable_if<U, bool>::type operator()(const K& key, typename BASE::FakeData& v) const { return BASE::operator()(key, v); }
    template<bool U = !std::is_void<T>::value>
    inline typename std::enable_if<U, typename BASE::GET>::type operator[](const K& key) const { return BASE::operator[](key); }
  };
}

#endif
