// Copyright ©2012 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in "bss_util.h"

#ifndef __CSTR_H__BSS__
#define __CSTR_H__BSS__

#include <string>
#include <stdarg.h>
#include <vector>
#include <assert.h>
#include "bss_deprecated.h"
#include "bss_util_c.h"
#ifdef BSS_COMPILER_GCC
#include <stdio.h>
#include <string.h>
#endif

//#define CSTRALLOC(T) std::basic_string<T, std::char_traits<T>, bss_util::RefAllocator<Alloc,bss_util::RefHackAllocPolicy<T>, bss_util::ObjectTraits<T>>>
#define CSTRALLOC(T) std::basic_string<T, std::char_traits<T>, Alloc>

/* This uses a trick stolen from numeric_limits, where you can explicitely initialize entire classes. Thus, we create a blank slate
and then explicitely initialize it for char and wchar_t, thus allowing our cStrT class template to be pretty and easy to use */
template<class _Ty>
class CSTR_CT
{
};

template<>
class CSTR_CT<char>
{
public:
  typedef char CHAR;
  typedef wchar_t OTHER_C;

  static inline BSS_FORCEINLINE const CHAR* SCHR(const CHAR* str, int val) { return strchr(str,val); }
  static inline BSS_FORCEINLINE size_t SLEN(const CHAR* str) { return strlen(str); }
  static inline BSS_FORCEINLINE CHAR* STOK(CHAR* str,const CHAR* delim, CHAR** context) { return STRTOK(str,delim,context); }
  //static inline errno_t WTOMB(size_t* outsize, CHAR* dest, size_t destsize, const OTHER_C* src, size_t maxcount) { return wcstombs_s(outsize,dest,destsize,src,maxcount); }
  static inline BSS_FORCEINLINE int VPF(CHAR *dest, size_t size, const CHAR *format, va_list args) { return VSNPRINTF(dest,size,format,args); }
  static inline BSS_FORCEINLINE int VPCF(const CHAR* str, va_list args) { return VSCPRINTF(str,args); }
  static inline BSS_FORCEINLINE size_t CONV(const OTHER_C* src, CHAR* dest, size_t len) { return UTF16toUTF8(src,dest,len); }

  static inline BSS_FORCEINLINE size_t O_SLEN(const OTHER_C* str) { return wcslen(str); }
  static inline BSS_FORCEINLINE const CHAR* STREMPTY() { return ""; }
};

template<>
class CSTR_CT<wchar_t>
{
public:
  typedef wchar_t CHAR;
  typedef char OTHER_C;

  static inline BSS_FORCEINLINE const CHAR* SCHR(const CHAR* str, wchar_t val) { return wcschr(str,val); }
  static inline BSS_FORCEINLINE size_t SLEN(const CHAR* str) { return wcslen(str); }
  static inline BSS_FORCEINLINE CHAR* STOK(CHAR* str,const CHAR* delim, CHAR** context) { return WCSTOK(str,delim,context); }
  //static inline errno_t WTOMB(size_t* outsize, CHAR* dest, size_t destsize, const OTHER_C* src, size_t maxcount) { return mbstowcs_s(outsize,dest,destsize,src,maxcount); }
#ifdef BSS_COMPILER_GCC
  //template<class S> static inline BSS_FORCEINLINE int VPF(S* str,const CHAR *format, va_list args)
  //{
  //  str->resize(SLEN(format));
  //  while(VSNWPRINTF(str->UnsafeString(),str->capacity(),format,args)==str->capacity()) //double size until it fits.
  //    str->resize(str->capacity()*2);
  //  return str->capacity();
  //}
#else
  static inline BSS_FORCEINLINE int VPF(CHAR *dest, size_t size, const CHAR *format, va_list args) { return VSNWPRINTF(dest,size,format,args); }
  static inline BSS_FORCEINLINE int VPCF(const CHAR* str, va_list args) { return VSCWPRINTF(str,args); }
#endif
  static inline BSS_FORCEINLINE size_t CONV(const OTHER_C* src, CHAR* dest, size_t len) { return UTF8toUTF16(src,dest,len); }

  static inline BSS_FORCEINLINE size_t O_SLEN(const OTHER_C* str) { return strlen(str); }
  static inline BSS_FORCEINLINE const CHAR* STREMPTY() { return L""; }
};

#pragma warning(push)
#pragma warning(disable: 4275)
#pragma warning(disable: 4251)
//template<typename Alloc = bss_util::AllocatorPolicyRef<char, bss_util::StringPoolAllocPolicy<char>, bss_util::ObjectTraits<char>>>
//template<typename Alloc = bss_util::Allocator<CHAR,bss_util::StringPoolPolicy<CHAR>>>
template<typename T=char, typename Alloc=std::allocator<T>>
class BSS_COMPILER_DLLEXPORT cStrT : public CSTRALLOC(T)//If dllimport is used on this linker errors crop up. I have no idea why. Also, if the constructors aren't inlined, it causes heap corruption errors because the basic_string constructors are inlined
{ //Note that you can take off BSS_COMPILER_DLLEXPORT but you'll pay for it with even more unnecessary 4251 warnings.
  typedef typename CSTR_CT<T>::CHAR CHAR;
  typedef typename CSTR_CT<T>::OTHER_C OTHER_C;
public:
  explicit inline cStrT(size_t length = 1) : CSTRALLOC(CHAR)() { CSTRALLOC(T)::reserve(length); } //an implicit constructor here would be bad
  inline cStrT(const CSTRALLOC(CHAR)& copy) : CSTRALLOC(CHAR)(copy) {}
  inline cStrT(CSTRALLOC(CHAR)&& mov) : CSTRALLOC(CHAR)(std::move(mov)) {}
  inline cStrT(const cStrT& copy) : CSTRALLOC(CHAR)(copy) {}
  inline cStrT(cStrT&& mov) : CSTRALLOC(CHAR)(std::move(mov)) {}
  template<class U> inline cStrT(const cStrT<T,U>& copy) : CSTRALLOC(CHAR)(copy) {}
  template<class U> inline cStrT(const cStrT<OTHER_C,U>& copy) : CSTRALLOC(CHAR)() { _convstr(copy.c_str()); }
  inline cStrT(const CHAR* string) : CSTRALLOC(CHAR)(!string?CSTR_CT<T>::STREMPTY():string) { }
  inline cStrT(const OTHER_C* text) : CSTRALLOC(CHAR)() { if(text!=0) _convstr(text); }
  inline cStrT(unsigned short index, const CHAR* text, const CHAR delim) : CSTRALLOC(CHAR)() //Creates a new string from the specified chunk
  {
    for(unsigned short i = 0; i < index; ++i)
    {
      if(text) text = CSTR_CT<T>::SCHR(text,(int)delim);  
      if(text) text++;
    }
    if(!text) return;
    const CHAR* end = CSTR_CT<T>::SCHR(text,(int)delim);
    if(!end) end = &text[CSTR_CT<T>::SLEN(text)];
    
    size_t _length = end-text;
    CSTRALLOC(T)::reserve(++_length);
    insert(0, text,_length-1);
  }

  inline operator const CHAR*() const { return _internal_ptr(); }
  
  inline const cStrT operator +(const cStrT& right) const { return cStrT(*this)+=right; }
  inline const cStrT operator +(cStrT&& right) const { right.insert(0, *this); return (std::move(right)); }
  template<class U> inline const cStrT operator +(const cStrT<T,U>& right) const { return cStrT(*this)+=right; }
  template<class U> inline const cStrT operator +(cStrT<T,U>&& right) const { right.insert(0, *this); return (std::move(right)); }
  inline const cStrT operator +(const CHAR* right) const { return cStrT(*this)+=right; }
  inline const cStrT operator +(const CHAR right) const { return cStrT(*this)+=right; }

  inline cStrT& operator =(const cStrT& right) { CSTRALLOC(CHAR)::operator =(right); return *this; }
  template<class U> inline cStrT& operator =(const cStrT<T,U>& right) { CSTRALLOC(CHAR)::operator =(right); return *this; }
  template<class U> inline cStrT& operator =(const cStrT<OTHER_C,U>& right) { CSTRALLOC(CHAR)::clear(); _convstr(right.c_str()); return *this; }
  inline cStrT& operator =(const CHAR* right) { if(right != 0) CSTRALLOC(CHAR)::operator =(right); else CSTRALLOC(CHAR)::clear(); return *this; }
  inline cStrT& operator =(const OTHER_C* right) { CSTRALLOC(CHAR)::clear(); if(right != 0) _convstr(right); return *this; }
  //inline cStrT& operator =(const CHAR right) { CSTRALLOC(CHAR)::operator =(right); return *this; } //Removed because char is also a number, and therefore confuses the compiler whenever something could feasibly be a number of any kind. If you need it, cast to basic_string<>
  inline cStrT& operator =(cStrT&& right) { CSTRALLOC(CHAR)::operator =(std::move(right)); return *this; }

  inline cStrT& operator +=(const cStrT& right) { CSTRALLOC(CHAR)::operator +=(right); return *this; }
  template<class U> inline cStrT& operator +=(const cStrT<T,U>& right) { CSTRALLOC(CHAR)::operator +=(right); return *this; }
  inline cStrT& operator +=(const CHAR* right) { if(right != 0 && right != _internal_ptr()) CSTRALLOC(CHAR)::operator +=(right); return *this; }
  inline cStrT& operator +=(const CHAR right) { CSTRALLOC(CHAR)::operator +=(right); return *this; }
  
  inline CHAR* UnsafeString() { return _internal_ptr(); } //This is potentially dangerous if the string is modified
  //inline const CHAR* String() const { return _internal_ptr(); }
  inline CHAR& GetChar(size_t index) { return CSTRALLOC(CHAR)::operator[](index); }
  inline void RecalcSize() { CSTRALLOC(CHAR)::resize(CSTR_CT<T>::SLEN(_internal_ptr())); }
  inline cStrT Trim() const { cStrT r(*this); r=_ltrim(_rtrim(r._internal_ptr(),r.size())); return r; }
  inline cStrT& ReplaceChar(CHAR search, CHAR replace)
  { 
    CHAR* pmod=_internal_ptr();
    size_t sz = CSTRALLOC(CHAR)::size();
    for(size_t i = 0; i < sz; ++i)
      if(pmod[i] == search)
        pmod[i] = replace;
    return *this;
  }

  static inline void Explode(std::vector<cStrT> &dest, const CHAR delim, const CHAR* text)
  {
    cStrT copy(text);
    CHAR delimhold[2] = { delim, 0 };
    CHAR* hold;
    CHAR* res = CSTR_CT<T>::STOK(copy.UnsafeString(), delimhold, &hold);

    while(res)
    {
      dest.push_back(res);
      res = CSTR_CT<T>::STOK(NULL,delimhold,&hold);
    }
  }
  template<typename I, typename F> // F = I(const T* s)
  static inline size_t BSS_FASTCALL ParseTokens(const T* str, const T* delim, std::vector<I>& vec, F parser)
  {
    cStrT<T> buf(str);
    return ParseTokens<I,F>(buf.UnsafeString(),delim,vec,parser);
  }
  template<typename I, typename F> // F = I(const T* s)
  static inline size_t BSS_FASTCALL ParseTokens(T* str, const T* delim, std::vector<I>& vec, F parser)
  {
    T* ct;
    T* cur=CSTR_CT<T>::STOK(str,delim,&ct);
    while(cur!=0)
    {
      vec.push_back(parser(cur));
      cur=CSTR_CT<T>::STOK(0,delim,&ct);
    }
    return vec.size();
  }

  static inline std::vector<cStrT> Explode(const CHAR delim, const CHAR* text) { std::vector<cStrT> r; Explode(r,delim,text); return r; }
  static inline cStrT StripChar(const CHAR* text, const CHAR c)
  { 
    cStrT r;
    r.resize(CSTR_CT<T>::SLEN(text)+1); // resize doesn't always account for null terminator
    size_t i;
    for(i=0;*text!=0;++text)
      if(*text!=c)
        r.UnsafeString()[i++]=*text;
    r.UnsafeString()[i]=0;
    r.RecalcSize();
    return r;
  }

  BSS_FORCEINLINE static cStrT<T,Alloc> cStrTF(const T* string, va_list vl)
  {
    if(!string)
      return cStrT<T,Alloc>();
    if(CSTR_CT<T>::SCHR(string, '%')==0) //Do we even need to check our va_list?
      return cStrT<T,Alloc>(string);
    cStrT<T,Alloc> r;
//#ifdef BSS_COMPILER_GCC
//    CSTR_CT<T>::VPF(&r, string, vl);
//#else
    size_t _length = (size_t)CSTR_CT<T>::VPCF(string,vl);
    r.resize(_length+1);
    CSTR_CT<T>::VPF(r.UnsafeString(), r.capacity(), string, vl);
//#endif
    r.resize(CSTR_CT<T>::SLEN(r.c_str()));
    return r;
  }

private:
#ifdef BSS_COMPILER_MSC
  inline BSS_FORCEINLINE CHAR* _internal_ptr() { return _Myptr(); }
  inline BSS_FORCEINLINE const CHAR* _internal_ptr() const { return _Myptr(); }
#elif defined(BSS_COMPILER_GCC)
  inline BSS_FORCEINLINE CHAR* _internal_ptr() { return const_cast<CHAR*>(CSTRALLOC(CHAR)::c_str()); }
  inline BSS_FORCEINLINE const CHAR* _internal_ptr() const { return CSTRALLOC(CHAR)::c_str(); }
#else
#error "cStr is not supported for this compiler"
#endif
  inline BSS_FORCEINLINE void BSS_FASTCALL _convstr(const OTHER_C* src)
  {
    size_t r = CSTR_CT<T>::CONV(src,0,0);
    if(r==(size_t)-1) return; // If invalid, bail
    CSTRALLOC(T)::resize(r); // resize() only adds one for null terminator if it feels like it.
    r = CSTR_CT<T>::CONV(src,_internal_ptr(),CSTRALLOC(T)::capacity());
    if(r==(size_t)-1) return; // If somehow still invalid, bail again
    CSTRALLOC(T)::resize(r-1); // resize to actual number of characters instead of simply the maximum (disregard null terminator)
    //_internal_ptr()[r]=0; // Otherwise ensure we have a null terminator.
  }

  inline static T* BSS_FASTCALL _ltrim(T* str) { for(;*str>0 && *str<33;++str); return str; }
  inline static T* BSS_FASTCALL _rtrim(T* str, size_t size) { T* inter=str+size; for(;inter>str && *inter<33;--inter); *(++inter)=0; return str; }
  //The following line of code is in such a twisted state because it must overload the operator[] inherent in the basic_string class and render it totally unusable so as to force the compiler to disregard it as a possibility, otherwise it gets confused with the CHAR* type conversion
  void operator[](std::allocator<CHAR>& f) { }//return CSTRALLOC(CHAR)::operator [](_Off); }

};
#pragma warning(pop)

#ifdef BSS_PLATFORM_WIN32
typedef cStrT<wchar_t,std::allocator<wchar_t>> cStrW;
inline cStrW cStrWF(const wchar_t* string, ...) { va_list vl; va_start(vl,string); return cStrW::cStrTF(string, vl); va_end(vl); }
typedef wchar_t bsschar;
#else
typedef char bsschar;
#endif
typedef cStrT<char,std::allocator<char>> cStr;
inline cStr cStrF(const char* string, ...) { va_list vl; va_start(vl,string); return cStr::cStrTF(string, vl); va_end(vl); }

#ifdef _UNICODE
typedef cStrW TStr;
#else
typedef cStr TStr;
#endif

// This allows cStr to inherit all the string operations
template<class _Elem, class _Alloc> // return NTCS + string
inline cStrT<_Elem,_Alloc> operator+(const _Elem *_Left,const cStrT<_Elem,_Alloc>& _Right)
	{	cStrT<_Elem,_Alloc> _Ans(std::char_traits<_Elem>::length(_Left) + _Right.size()); _Ans += _Left; return (_Ans += _Right); }
template<class _Elem, class _Alloc> // return character + string
inline cStrT<_Elem,_Alloc> operator+(const _Elem _Left,const cStrT<_Elem,_Alloc>& _Right)
	{	cStrT<_Elem,_Alloc> _Ans(1+_Right.size()); _Ans += _Left; return (_Ans += _Right); }
template<class _Elem, class _Alloc> // return string + string
inline cStrT<_Elem,_Alloc> operator+(cStrT<_Elem,_Alloc>&& _Left,const cStrT<_Elem,_Alloc>& _Right)
	{	_Left.append(_Right); return (std::move(_Left)); } //These operations are moved to the left because they return a basic_string, not cStr
template<class _Elem, class _Alloc> // return NTCS + string
inline cStrT<_Elem,_Alloc> operator+(const _Elem *_Left,cStrT<_Elem,_Alloc>&& _Right)
	{	_Right.insert(0, _Left); return (std::move(_Right)); }
template<class _Elem, class _Alloc> // return character + string
inline cStrT<_Elem,_Alloc> operator+(const _Elem _Left,cStrT<_Elem,_Alloc>&& _Right)
	{	_Right.insert(0, 1, _Left); return (std::move(_Right)); }
template<class _Elem, class _Alloc> // return string + NTCS
inline cStrT<_Elem,_Alloc> operator+(cStrT<_Elem,_Alloc>&& _Left,const _Elem *_Right)
	{	_Left.append(_Right); return (std::move(_Left));	}
template<class _Elem, class _Alloc> // return string + character
inline cStrT<_Elem,_Alloc> operator+(cStrT<_Elem,_Alloc>&& _Left,const _Elem _Right)
	{	_Left.append(1, _Right); return (std::move(_Left)); }
template<class _Elem, class _Alloc> // return string + string
inline cStrT<_Elem,_Alloc> operator+(cStrT<_Elem,_Alloc>&& _Left,cStrT<_Elem,_Alloc>&& _Right)
	{	
	  if (_Right.size() <= _Left.capacity() - _Left.size() || _Right.capacity() - _Right.size() < _Left.size()) {
      _Left.append(_Right);
		  return (std::move(_Left));
    }
    _Right.insert(0, _Left);
		return (std::move(_Right));
	}

#endif
