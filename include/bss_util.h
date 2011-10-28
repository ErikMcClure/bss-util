/* Black Sphere Studios Utility Library
   Copyright �2011 Black Sphere Studios

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/


#ifndef __BSS_UTIL_H__
#define __BSS_UTIL_H__

#include "bss_util_c.h"
#include <assert.h>
#include <math.h>
#include <memory.h>

namespace bss_util { 
  static const VersionType BSSUTIL_VERSION = { 0,3,82 };

  BSS_COMPILER_DLLEXPORT extern void BSS_FASTCALL SetWorkDirToCur(); //Sets the working directory to the actual goddamn location of the EXE instead of the freaking start menu, or possibly the desktop. The possibilities are endless! Fuck you, windows.
  BSS_COMPILER_DLLEXPORT extern unsigned long long BSS_FASTCALL bssFileSize(const char* path);
  BSS_COMPILER_DLLEXPORT extern unsigned long long BSS_FASTCALL bssFileSize(const wchar_t* path);
  BSS_COMPILER_DLLEXPORT extern long BSS_FASTCALL GetTimeZoneMinutes(); //Returns the current time zone difference from UTC in minutes

  //Useful numbers
  const double PI = 3.141592653589793238462643383279;
  const double PI_HALF = PI*0.5;
  const double PI_DOUBLE = PI*2.0;  
  const double E_CONST = 2.718281828459045235360287471352;
  const double SQRT_TWO = 1.414213562373095048801688724209;

  /* Given the size of a type, lets you return the signed or unsigned equivelent */
  template<int bytes> struct TSignPick {};
  template<> struct TSignPick<1> { typedef char SIGNED; typedef unsigned char UNSIGNED; };
  template<> struct TSignPick<2> { typedef __int16 SIGNED; typedef unsigned __int16 UNSIGNED; };
  template<> struct TSignPick<4> { typedef __int32 SIGNED; typedef unsigned __int32 UNSIGNED; };
  template<> struct TSignPick<8> { typedef __int64 SIGNED; typedef unsigned __int64 UNSIGNED; };
#ifdef BSS_64BIT
  template<> struct TSignPick<16> { typedef __int128 SIGNED; typedef unsigned __int128 UNSIGNED; };
#endif 

  /* Get max size of an arbitrary number of bits, either signed or unsigned (assuming one's or two's complement implementation) */
  template<unsigned char BITS>
  struct ABitLimit
  {
    typedef typename TSignPick<((BITS>>3) << (0+((BITS%3)>0)))>::SIGNED SIGNED; //rounds the type up if necessary.
    typedef typename TSignPick<((BITS>>3) << (0+((BITS%3)>0)))>::UNSIGNED UNSIGNED;

    static const SIGNED SIGNED_MIN=(((SIGNED)1)<<(BITS-1));
    static const SIGNED SIGNED_MAX=(SIGNED_MIN-1);
    static const UNSIGNED UNSIGNED_MIN=0;
    static const UNSIGNED UNSIGNED_MAX=(SIGNED_MIN<<1)+1; //these are all done carefully to ensure no overflow is ever utilized and it respects an arbitrary bit limit
  };
  template<typename T>
  struct TBitLimit : public ABitLimit<sizeof(T)<<3> {};

  /* template inferred version of T_GETBIT and T_GETBITRANGE */
  template<class T>
  inline T BSS_FASTCALL GetBitMask(int bit) { return T_GETBIT(T,bit); }
  template<class T>
  inline T BSS_FASTCALL GetBitMask(int low, int high) { return T_GETBITRANGE(T,low,high); }

  /* Replaces one character with another in a string */
  inline char* BSS_FASTCALL strreplace(char* string, const char find, const char replace)
	{
		if(!string) return 0;
		unsigned int curpos = (unsigned int)-1; //this will wrap around to 0 when we increment

		while(string[++curpos] != '\0') //replace until the null-terminator
			if(string[curpos] == find)
				string[curpos] = replace;

		return string;
	}
  inline wchar_t* BSS_FASTCALL wcsreplace(wchar_t* string, const wchar_t find, const wchar_t replace)
	{
		if(!string) return 0;
		unsigned int curpos = (unsigned int)-1; //this will wrap around to 0 when we increment

		while(string[++curpos] != '\0') //replace until the null-terminator
			if(string[curpos] == find)
				string[curpos] = replace;

		return string;
	}

  template<typename T>
  inline unsigned int BSS_FASTCALL strccount(T* string, T c)
  {
    unsigned int ret=0;
    while(*string) { if(*string==c) ++ret; ++string; }
    return ret;
  }
  
  template<typename T>
  inline unsigned int BSS_FASTCALL strccount(T* string, unsigned int length, T c)
  {
    unsigned int ret=0;
    for(unsigned int i = 0; i < length; ++i)
      if(string[i]==c) ++ret;
    return ret;
  }

  /* template swap function, h should be optimized out by the compiler */
  template<typename T>
  inline void BSS_FASTCALL rswap(T& p, T& q)
  {
    T h(std::move(p));
    p=std::move(q);
    q=std::move(h);
  }
  
  template<typename T>
  inline T* BSS_FASTCALL strltrim(T* str)
  {
    for(;*str>0 && *str<33;++str);
    return str;
  }
  
  template<typename T>
  inline T* BSS_FASTCALL strrtrim(T* str)
  {
    T* inter=str+strlen(str);

    for(;inter>str && *inter<33;--inter);
    *(++inter)='\0';
    return str;
  }

  template<typename T>
  inline T* BSS_FASTCALL strtrim(T* str)
  {
    return strrtrim(strltrim(str));
  }

  /* Shuffler using Fisher-Yates/Knuth Shuffle algorithm based on Durstenfeld's implementation. This is an in-place algorithm and works with any numeric type T. */
  template<typename T, typename ST, ST (*RandFunc)(ST min, ST max)>
  inline void BSS_FASTCALL shuffle(T* p, ST size)
  {
    for(ST i=size-1; i!=(ST)-1; --i)
      rswap<T>(p[i],p[RandFunc(0,i)]);
  }
  template<typename T, typename ST, ST size, ST (*RandFunc)(ST min, ST max)>
  inline void BSS_FASTCALL shuffle(T (&p)[size])
  {
    for(ST i=size-1; i!=(ST)-1; --i)
      rswap<T>(p[i],p[RandFunc(0,i)]);
  }

#ifdef _INC_STDLIB //These shortcuts are only available if you have access to rand() in the first place
  /* inline function wrapper to the #define RANDINTGEN */
  inline int bss_randfunc(int min, int max)
  {
    return !(max-min)?min:RANDINTGEN(min,max);
  }

  /* Shuffler using default random number generator.*/
  template<typename T>
  inline void BSS_FASTCALL shuffle(T* p, int size)
  {
    shuffle<T,int,&bss_randfunc>(p,size);
  }
  template<typename T, int size>
  inline void BSS_FASTCALL shuffle(T (&p)[size])
  {
    shuffle<T,int,size,&bss_randfunc>(p);
  }
#endif

  /* This is a bit-shift method of calculating the next number in the fibonacci sequence by approximating the golden ratio with 0.6171875 (1/2 + 1/8 - 1/128) */
  template<typename T>
  inline T BSS_FASTCALL fbnext(T in)
  {
    return in + 1 + (in>>1) + (in>>3) - (in>>7);
  }

  /* Gets the sign of any number (0 is assumed to be positive) */
  template<typename T>
  inline char BSS_FASTCALL tsign(T n)
  {
    return (n >= 0) - (n < 0);
  }

  /* Gets the sign of any number, where a value of 0 returns 0 */
  template<typename T>
  inline char BSS_FASTCALL tsignzero(T n)
  {
    return (n > 0) - (n < 0);
  }
  
  /* Directly calls the fistp function for float to int rounding. Because the FPU tends to be a rounding mode that doesn't clamp to 0, this
     will usually round the float to the nearest integer instead of simply chopping off the decimal. This usually takes 6 cycles, compared
     to the 80 or so cycles caused by a normal float to int cast, and works in any precision. */
  template<typename T> //T must be either float or double or you need to stop programming when you're high
  inline __int32 fFastRound(T f)
  {
	  __int32 retval;
	  __asm fld f
	  __asm fistp retval
	  return retval;
  }

  /* Returns true if FPU is in single precision mode and false otherwise (false for both double and extended precision) */
  inline bool FPUsingle()
  { 
    unsigned int i;
    __asm fnstcw i;
    return ((i&(0x0300))==0); //0x0300 is the mask for the precision bits, 0 indicates single precision
  }

  /* Extremely fast rounding function that again will usually round to the nearest integer, but only works in double precision mode */
  inline __int32 fFastDoubleRound(double val)
  {
    const double _double2fixmagic = 4503599627370496.0*1.5; //2^52 for 52 bits of mantissa
    assert(!FPUsingle());
	  val		= val + _double2fixmagic;
	  return ((__int32*)&val)[0]; 
  }

  /* Single precision version of the above function. While precision problems are mostly masked in the above function by limiting it to
    __int32, in this function they are far more profound due to there only being 24 bits of mantissa to work with. Use with caution. */
  inline __int32 fFastSingleRound(double val)
  {
    const double _single2fixmagic = 16777216.0*1.5; //2^24 for 24 bits of mantissa
    assert(FPUsingle());
	  val		= val + _single2fixmagic;
	  return (__int32)(((((unsigned __int64*)&val)[0])&0xFFFFF0000000)>>28);
  }

	/* This is a super fast floating point comparison function with a significantly higher tolerance and no
		 regard towards the size of the floats. */
	inline bool BSS_FASTCALL fcompare(float fleft, float fright)
	{
		__int32 left = *(__int32*)(&fleft); //This maps our float to an int so we can do bitshifting operations on it
		__int32 right = *(__int32*)(&fright); //see above
		unsigned char dif = abs((0x7F800000&left)-(0x7F800000&right))>>23; // This grabs the 8 exponent bits and subtracts them.
		if(dif>1) // An exponent difference of 2 or greater means the numbers are different.
			return false;
		return !dif?((0x007FFF80&left)==(0x007FFF80&right)):!(abs((0x007FFF80&left)-(0x007FFF80&right))-0x007FFF80); //If there is no difference in exponent we tear off the last 7 bits and compare the value, otherwise we tear off the last 7 bits, subtract, and then subtract the highest possible significand to compensate for the extra exponent.
	}

  /* This determines if a float is sufficiently close to 0 */
  inline bool BSS_FASTCALL fsmall(float f, int maxtolerance=-20)
  {
    __int32 i = *(__int32*)(&f);
    int check = ((0x7F800000&i)>>23)-127;
    return ((!check) && (!(i&0x007FFF80)))||(check<maxtolerance);
  }

  /* This is a super fast length calculation for 2D coordinates; See http://www.azillionmonkeys.com/qed/sqroot.html for details (Algorithm by Paul Hsieh) */
  inline float BSS_FASTCALL flength(float x, float y)
  {
    x = abs(x);
    y = abs(y);
    //(1 + 1/(4-2*sqrt(2)))/2 = 0.92677669529663688
    float hold=0.7071067811865475f*(x+y), mval=(x > y)?x:y;
    return 0.92677669529663688f * (hold > mval)?hold:mval;
  }

  /* The classic fast square root approximation, which is often mistakenly attributed to John Carmack. The algorithm is in fact over 15 years old and no one knows where it came from. */
  inline float BSS_FASTCALL fFastSqrt(float number)
  {
    const float f = 1.5F;
    __int32 i;
    float x, y;

    x = number * 0.5F;
    y  = number;
    i  = * ( __int32 * ) &y;
    i  = 0x5f3759df - ( i >> 1 );
    y  = * ( float * ) &i;
    y  = y * ( f - ( x * y * y ) );
    y  = y * ( f - ( x * y * y ) ); //extra iteration for added accuracy
    return number * y;
  }

  /* Adaptation of the class fast square root approximation for double precision, based on http://www.azillionmonkeys.com/qed/sqroot.html */
  inline double BSS_FASTCALL dFastSqrt(double number)
  {
    const double f = 1.5;
    unsigned __int32* i;
    double x, y;

    x = number*0.5;
	  y = number;
    i = ((unsigned __int32 *)&y) + 1;
	  *i = (0xbfcdd90a - *i)>>1; /* estimate of 1/sqrt(number) */

    y = y * ( f - ( x * y * y ) );
    y = y * ( f - ( x * y * y ) );
    y = y * ( f - ( x * y * y ) );
    y = y * ( f - ( x * y * y ) ); //twice the iterations for twice the precision
    return number * y;
  }

  /* bit-twiddling based method of calculating an integral square root from Wilco Dijkstra - http://www.finesse.demon.co.uk/steven/sqrt.html */
  template<typename T, unsigned int bits>
  inline T BSS_FASTCALL IntFastSqrt(T n)
  {
    T root = 0, t;

    for(unsigned int i = bits; i>0;)
    {
      --i;
      t = root + (1 << (i)); 
      if (n >= t << (i))   
      {   n -= t << (i);   
          root |= 2 << (i); 
      }
    }
    return root >> 1;
  }
  template<typename T>
  inline T BSS_FASTCALL IntFastSqrt(T n)
  {
    return IntFastSqrt<T,sizeof(T)<<2>(n); //done to ensure loop gets unwound
  }
  
  template<typename T> //assumes integer type if not one of the floating point types
  inline T BSS_FASTCALL FastSqrt(T n) { return IntFastSqrt(n); } //Picks correct method for calculating any square root quickly
  template<> inline float BSS_FASTCALL FastSqrt(float n) { return fFastSqrt(n); }
  template<> inline double BSS_FASTCALL FastSqrt(double n) { return dFastSqrt(n); }
  template<> inline long double BSS_FASTCALL FastSqrt(long double n) { return dFastSqrt((double)n); }

  /* Distance calculation (squared) */
  template<typename T>
  inline T BSS_FASTCALL distsqr(T X, T Y, T x, T y)
  {
    T tx=X-x,ty=Y-y; return (tx*tx)+(ty*ty); //It doesn't matter if you use temporary values for floats, but it does if you use ints (for unknown reasons)
  }

  /* Distance calculation */
  template<typename T>
  inline T BSS_FASTCALL dist(T X, T Y, T x, T y)
  {
    return FastSqrt<T>(distsqr<T>(X,Y,x,y));
  }

  inline const void* bytesearch(const void* search, size_t length, const void* find, size_t flength)
  {
    if(!search || !length || !find || !flength || length < flength) return 0;

    unsigned char* s=(unsigned char*)search;
//    size_t d = length-flength; //Because flength > 0, d < length and so i is always valid
    for(size_t i = 0; i <= length; ++i)
    {
      search=s+i;
      if(!memcmp(search,find,flength))
        return search;
    }
    return 0;
  }

  inline void* bytesearch(void* search, size_t length, void* find, size_t flength)
  {
    return const_cast<void*>(bytesearch((const void*)search,length,(const void*)find,flength));
  }
  //Unlike FastSqrt, these are useless unless you are on a CPU without SSE instructions, or have a terrible std implementation.
  ///* Fast sin function with 0.078% error when extra precision is left in. See http://www.devmaster.net/forums/showthread.php?t=5784 */
  //inline float BSS_FASTCALL FastSin(float x)
  //{
  //  x-= (int)(x*(1/(float)PI_DOUBLE))*(float)PI_DOUBLE;
  //  const float B = 4/(float)PI;
  //  const float C = -4/(float)PI_DOUBLE;

  //  float y = B * x + C * x * abs(x);

  //  //const float Q = 0.775f;
  //  const float P = 0.225f;

  //  return (P * (y * abs(y) - y) + y);   // Q * y + P * y * abs(y) (extra precision)
  //}

  //inline float BSS_FASTCALL FastCos(float x)
  //{
  //  return FastSin(x+(float)PI_HALF);
  //}

  /* Bit-twiddling hack for base 2 log by Sean Eron Anderson */
  inline unsigned int BSS_FASTCALL log2(unsigned char v)
  {
    const unsigned int b[] = {0x2, 0xC, 0xF0};
    const unsigned int S[] = {1, 2, 4};

    register unsigned int r = 0; // result of log2(v) will go here
    if (v & b[2]) { v >>= S[2]; r |= S[2]; } 
    if (v & b[1]) { v >>= S[1]; r |= S[1]; } 
    if (v & b[0]) { v >>= S[0]; r |= S[0]; } 

    return r;
  }
  inline unsigned int BSS_FASTCALL log2(unsigned short v)
  {
    const unsigned int b[] = {0x2, 0xC, 0xF0, 0xFF00};
    const unsigned int S[] = {1, 2, 4, 8};

    register unsigned int r = 0; // result of log2(v) will go here
    if (v & b[3]) { v >>= S[3]; r |= S[3]; } 
    if (v & b[2]) { v >>= S[2]; r |= S[2]; } 
    if (v & b[1]) { v >>= S[1]; r |= S[1]; } 
    if (v & b[0]) { v >>= S[0]; r |= S[0]; } 

    return r;
  }

  inline unsigned int BSS_FASTCALL log2(unsigned int v)
  {
    const unsigned int b[] = {0x2, 0xC, 0xF0, 0xFF00, 0xFFFF0000};
    const unsigned int S[] = {1, 2, 4, 8, 16};

    register unsigned int r = 0; // result of log2(v) will go here
    if (v & b[4]) { v >>= S[4]; r |= S[4]; } 
    if (v & b[3]) { v >>= S[3]; r |= S[3]; } 
    if (v & b[2]) { v >>= S[2]; r |= S[2]; } 
    if (v & b[1]) { v >>= S[1]; r |= S[1]; } 
    if (v & b[0]) { v >>= S[0]; r |= S[0]; } 

    return r;
  }
  inline unsigned int BSS_FASTCALL log2(unsigned __int64 v)
  {
    const unsigned __int64 b[] = {0x2, 0xC, 0xF0, 0xFF00, 0xFFFF0000, 0xFFFFFFFF00000000 };
    const unsigned int S[] = {1, 2, 4, 8, 16, 32};

    register unsigned int r = 0; // result of log2(v) will go here
    if (v & b[5]) { v >>= S[5]; r |= S[5]; } 
    if (v & b[4]) { v >>= S[4]; r |= S[4]; } 
    if (v & b[3]) { v >>= S[3]; r |= S[3]; } 
    if (v & b[2]) { v >>= S[2]; r |= S[2]; } 
    if (v & b[1]) { v >>= S[1]; r |= S[1]; } 
    if (v & b[0]) { v >>= S[0]; r |= S[0]; } 

    return r;
  }
  inline unsigned int BSS_FASTCALL log2_p2(unsigned int v) //Works only if v is a power of 2
  {
    assert(v && !(v & (v - 1))); //debug version checks to ensure its a power of two
    const unsigned int b[] = {0xAAAAAAAA, 0xCCCCCCCC, 0xF0F0F0F0, 0xFF00FF00, 0xFFFF0000};
    register unsigned int r = (v & b[0]) != 0;
    r |= ((v & b[4]) != 0) << 4;
    r |= ((v & b[3]) != 0) << 3;
    r |= ((v & b[2]) != 0) << 2;
    r |= ((v & b[1]) != 0) << 1;

    return r;
  }

  /* Basic lerp function with no bounds checking */
  template<class T>
  inline T lerp(T a, T b, double bounds)
  {
	  return a+((T)((b-a)*bounds));
  }
  
  /* Simple 128bit integer for x86 instruction set. Replaced with __int128 if possible */
//#ifdef BSS32BIT
//  struct int128
//  {
//    inline int128& BSS_FASTCALL operator+=(const int128& right) { _add(right.ints[0],right.ints[1],right.ints[2],right.ints[3]); return *this; }
//
//    __int32 ints[4];
//
//  private:
//    inline void BSS_FASTCALL _add(__int32 a,__int32 b,__int32 c,__int32 d) //Done because I suck with assembly :D
//    {
//      __asm {
//#ifdef BSS_NO_FASTCALL //if we are using fastcall we don't need these instructions
//        mov ECX, a
//        mov EDX, b
//#endif
//        mov EAX, c
//        mov EBX, d
//        add ints[0], ECX
//        adc ints[1], EDX
//        adc ints[2], EAX
//        adc ints[3], EBX
//      }
//    }
//  };
//  struct uint128
//  {
//    inline uint128& BSS_FASTCALL operator+=(const uint128& right) { _add(right.ints[0],right.ints[1],right.ints[2],right.ints[3]); return *this; }
//
//    unsigned __int32 ints[4];
//
//  private:
//    inline void BSS_FASTCALL _add(unsigned __int32 a,unsigned __int32 b,unsigned __int32 c,unsigned __int32 d) //Done because I suck with assembly :D
//    {
//      __asm {
//#ifdef BSS_NO_FASTCALL //if we are using fastcall we don't need these instructions
//        mov ECX, a
//        mov EDX, b
//#endif
//        mov EAX, c
//        mov EBX, d
//        add ints[0], ECX
//        adc ints[1], EDX
//        adc ints[2], EAX
//        adc ints[3], EBX
//      }
//    }
//  };
//#else
//  typedef __int128 int128;
//  typedef unsigned __int128 uint128;
//#endif

  /* Utilizes Clone() to create a clone based reference */
  template<class T>
  class BSS_COMPILER_DLLEXPORT cCloneRef
  {
  public:
    inline cCloneRef() : _ref(0) {}
    inline cCloneRef(const cCloneRef& copy) : _ref(copy._ref->Clone()) {}
    inline cCloneRef(const T& ref) : _ref(ref.Clone()) {}
    inline ~cCloneRef() { if(_ref) delete _ref; }
    inline T* GetRef() { return _ref; }

    inline cCloneRef& operator =(const cCloneRef& right) { if(_ref) delete _ref; _ref=right._ref->Clone(); return *this; }
    inline cCloneRef& operator =(const T& right) { if(_ref) delete _ref; _ref=right.Clone(); return *this; }
    operator T*() { return _ref; }
    T* operator ->() const { return _ref; }
    T& operator *() const { return *_ref; }

  protected:
    T* _ref;
  };
} 

#endif