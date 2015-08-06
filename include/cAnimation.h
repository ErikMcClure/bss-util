// Copyright �2015 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in "bss_util.h"

#ifndef __C_ANIMATION_H__BSS__
#define __C_ANIMATION_H__BSS__

#include "cArraySort.h"
#include "cRefCounter.h"
#include "delegate.h"
#include "cPriorityQueue.h"

namespace bss_util {
  // Abstract base animation class
  class BSS_COMPILER_DLLEXPORT cAniBase : public cRefCounter
  {
  public:
    cAniBase(cAniBase&& mov) : _typesize(mov._typesize), _length(mov._length), _calc(mov._calc), _loop(mov._loop) { Grab(); }
    cAniBase(size_t typesize) : _typesize(typesize), _length(0.0), _calc(0.0), _loop(-1.0) { Grab(); }
    virtual ~cAniBase() {}
    inline void SetLength(double length = 0.0) { _length = length; }
    inline double GetLength() const { _length == 0.0 ? _calc : _length; }
    inline void SetLoop(double loop = 0.0) { _loop = loop; }
    inline double GetLoop() const { return _loop; }
    inline size_t SizeOf() const { return _typesize; }
    virtual unsigned int GetSize() const = 0;
    virtual const void* GetArray() const = 0;
    virtual void* GetFunc() const = 0;

    cAniBase& operator=(cAniBase&& mov)
    {
      _length = mov._length;
      _calc = mov._calc;
      _loop = mov._loop;
      return *this;
    }

  protected:
    double _length;
    double _calc;
    const size_t _typesize;
    double _loop;
  };

  // Animation class that stores an animation for a specific type.
  template<typename T, ARRAY_TYPE ArrayType = CARRAY_SAFE, typename Alloc = StaticAllocPolicy<T>>
  class BSS_COMPILER_DLLEXPORT cAnimation : public cAniBase
  {
  public:
    typedef std::pair<double, T> PAIR;
    typedef T(BSS_FASTCALL *FUNC)(const PAIR*, unsigned int, unsigned int, double, const T&);

    explicit cAnimation(FUNC f = 0) : cAniBase(sizeof(T)), _f(f) { }
    cAnimation(cAnimation&& mov) : cAniBase(std::move(mov)), _pairs(std::move(mov._pairs)), _f(std::move(mov._f)) {}
    cAnimation(const PAIR* src, unsigned int len, FUNC f = 0) : cAniBase(sizeof(T)), _f(f) { Set(src, len); }
    virtual ~cAnimation() {}
    inline unsigned int Add(double time, const T& data) { unsigned int r = _pairs.Insert(std::pair(time, data)); _calc = _pairs.Back().first; return r; }
    inline void Set(const PAIR* src, unsigned int len) { _pairs.SetArray(src, len); _calc = _pairs.Back().first; }
    inline const PAIR& Get(unsigned int index) const { return _pairs[index]; }
    inline bool Remove(unsigned int index) { return _pairs.Remove(index); }
    virtual unsigned int GetSize() const { return _pairs.Length(); }
    virtual const void* GetArray() const { return (const PAIR*)_pairs; }
    void SetFunc(FUNC f) { _f = f; }
    virtual void* GetFunc() const { return _f; }

    cAnimation& operator=(cAnimation&& mov)
    {
      cAniBase::operator=(std::move(mov));
      _pairs = std::move(_pairs);
      _f = std::move(_f);
      return *this;
    }

  protected:
    cArraySort<PAIR, CompTFirst<PAIR>, unsigned int, ArrayType, Alloc> _pairs;
    FUNC _f;
  };

  // Animation state object. References an animation that inherits cAniBase
  struct BSS_COMPILER_DLLEXPORT cAniState
  {
    cAniState(const cAniState& copy) : _ani(copy._ani), _cur(copy._cur), _time(copy._time) { if(_ani) _ani->Grab(); }
    cAniState(cAniState&& mov) : _ani(mov._ani), _cur(mov._cur), _time(mov._time) { mov._ani = 0; }
    explicit cAniState(cAniBase* p) : _ani(p), _cur(0) { if(_ani) _ani->Grab(); }
    virtual ~cAniState() { if(_ani) _ani->Drop(); }
    virtual bool Interpolate(double delta)=0;
    virtual void Reset() { _cur = 0; _time = 0.0; }

    cAniState& operator=(const cAniState& copy)
    {
      if(_ani) _ani->Drop();
      _ani = copy._ani;
      _cur = copy._cur;
      _time = copy._time;
      if(_ani) _ani->Grab();
      return *this;
    }
    cAniState& operator=(cAniState&& mov)
    {
      if(_ani) _ani->Drop();
      _ani = mov._ani;
      _cur = mov._cur;
      _time = mov._time;
      mov._ani = 0;
      return *this;
    }

  protected:
    cAniBase* _ani;
    unsigned int _cur;
    double _time;
  };

  template<typename T>
  struct BSS_COMPILER_DLLEXPORT cAniStateDiscrete : public cAniState
  {
    cAniStateDiscrete(const cAniStateDiscrete& copy) : cAniState(copy), _set(copy._set) { }
    cAniStateDiscrete(cAniStateDiscrete&& mov) : cAniState(std::move(mov)), _set(std::move(mov._set)) { }
    cAniStateDiscrete(cAniBase* p, delegate<void, T> d) : cAniState(p), _set(d) { assert(_ani->SizeOf() == sizeof(T)); }
    template<ARRAY_TYPE ArrayType, typename Alloc>
    cAniStateDiscrete(cAnimation<T, ARRAY_TYPE, Alloc>* p, delegate<void, T> d) : cAniState(p), _set(d) {}
    cAniStateDiscrete() : _set(0,0) {}

    cAniStateDiscrete& operator=(const cAniStateDiscrete& copy) { cAniState::operator=(copy); _set = copy._set; return *this; }
    cAniStateDiscrete& operator=(cAniStateDiscrete&& mov) { cAniState::operator=(std::move(mov)); _set = std::move(mov._set); return *this; }

    virtual bool Interpolate(double delta)
    {
      _time += delta;
      auto svar = _ani->GetSize();
      auto v = (typename cAnimation<T>::PAIR*)_ani->GetArray();
      double loop = _ani->GetLoop();
      double length = _ani->GetLength();
      while(_cur < svar && v[_cur].first <= _time)
        _del(v[_cur++].second); // We call all the discrete values because many discrete values are interdependent on each other.
      if(_time >= length && loop >= 0.0) // We do the loop check down here because we need to finish calling all the discrete values on the end of the animation before looping
      {
        _cur = 0; // We can't call Reset() here because _time contains information we need.
        _time = fmod(_time - length, length - loop);// + loop; // instead of adding loop here we just pass loop into Interpolate, which adds it.
        return Interpolate(loop); // because we used fmod, it is impossible for this to result in another loop.
      }
      return _time < length;
    }

  protected:
    delegate<void, T> _set;
  };

  template<typename T>
  struct BSS_COMPILER_DLLEXPORT cAniStateSmooth : public cAniStateDiscrete<T>
  {
    cAniStateSmooth(const cAniStateSmooth& copy) : cAniStateDiscrete<T>(copy), _init(copy._init) { }
    cAniStateSmooth(cAniStateSmooth&& mov) : cAniStateDiscrete<T>(std::move(mov)), _init(std::move(mov._init)) { }
    cAniStateSmooth(cAniBase* p, delegate<void, T> d, T init = T()) : cAniStateDiscrete(p, d), _init(init) { assert(_ani->GetFunc() != 0); }
    template<ARRAY_TYPE ArrayType, typename Alloc>
    cAniStateSmooth(cAnimation<T, ARRAY_TYPE, Alloc>* p, delegate<void, T> d, T init = T()) : cAniStateDiscrete(p, d), _init(init) {}
    cAniStateSmooth() {}

    cAniStateSmooth& operator=(const cAniStateSmooth& copy) { cAniStateDiscrete<T>::operator=(copy); _init = copy._init; return *this; }
    cAniStateSmooth& operator=(cAniStateSmooth&& mov) { cAniStateDiscrete<T>::operator=(std::move(mov)); _init = std::move(mov._init); return *this; }

    virtual bool Interpolate(double delta)
    {
      _time += delta;
      auto svar = _ani->GetSize();
      auto v = (typename cAnimation<T>::PAIR*)_ani->GetArray();
      auto f = (typename cAnimation<T>::FUNC*)_ani->GetFunc();
      double loop = _ani->GetLoop();
      double length = _ani->GetLength();
      if(_time >= length && loop >= 0.0)
      {
        _cur = 0;
        _time = fmod(_time - length, length - loop) + loop;
      }

      while(_cur<svar && v[_cur].first <= _time) ++_cur;
      if(_cur >= svar)
      { //Resolve the animation, but only if there was more than 1 keyframe, otherwise we'll break it.
        if(svar>1) _setval(_func(v, svar - 1, 1.0));
      }
      else
      {
        double hold = !_cur?0.0:v[_cur - 1].first;
        _set(f(v, svar, _cur, (_time - hold) / (v[_cur].first - hold), _init));
      }
      return _time < length;
    }

    static inline T BSS_FASTCALL NoInterpolate(const typename cAnimation<T>::PAIR* v, unsigned int s, unsigned int cur, double t, const T& init) { unsigned int i = cur - (t != 1.0); return (i<0) ? init : a[i].value; }
    static inline T BSS_FASTCALL NoInterpolateRel(const typename cAnimation<T>::PAIR* v, unsigned int s, unsigned int cur, double t, const T& init) { assert(i > 0); return init + a[i - (t != 1.0)].value; }
    static inline T BSS_FASTCALL LerpInterpolate(const typename cAnimation<T>::PAIR* v, unsigned int s, unsigned int cur, double t, const T& init) { return lerp<VALUE>(!i ? init : a[i - 1].value, a[i].value, t); }
    static inline T BSS_FASTCALL LerpInterpolateRel(const typename cAnimation<T>::PAIR* v, unsigned int s, unsigned int cur, double t, const T& init) { assert(i > 0); return init + lerp<VALUE>(a[i - 1].value, a[i].value, t); }
    static inline T BSS_FASTCALL CubicInterpolateRel(const typename cAnimation<T>::PAIR* v, unsigned int s, unsigned int cur, double t, const T& init) { assert(i > 0); return init + CubicBSpline<VALUE>(t, a[i - 1 - (i != 1)].value, a[i - 1].value, a[i].value, a[i + ((i + 1) != a.Size())].value); }
    //static inline T BSS_FASTCALL QuadInterpolate(const PAIR* v, unsigned int s, unsigned int cur, double t, const T& init) {}
    //typedef T(BSS_FASTCALL *TIME_FNTYPE)(const TVT_ARRAY_T& a, IDTYPE i, double t); // VC++ 2010 can't handle this being in the template itself
    //template<TIME_FNTYPE FN, double(*TIME)(DATA&)>
    //static inline T BSS_FASTCALL TimeInterpolate(const TVT_ARRAY_T& a, IDTYPE i, double t) { return (*FN)(a, i, UniformQuadraticBSpline<double, double>(t, (*TIME)(a[i - 1 - (i>2)]), (*TIME)(a[i - 1]), (*TIME)(a[i]))); }

  protected:
    T _init; // This is referenced by an index of -1
  };


  /*
  template<typename T, double(*TODURATION)(const T&), ARRAY_TYPE ArrayType = CARRAY_SAFE, typename Alloc = StaticAllocPolicy<T>>
  class BSS_COMPILER_DLLEXPORT cAnimationInterval : public cAnimation
  {
    inline unsigned int Add(double time, const T& data) { unsigned int r = Add(time, data); return r; }
    inline void Set(const PAIR* src, unsigned int len) { Set(src,len); }
    inline bool Remove(unsigned int index) { return _pairs.Remove(index); }

  };
  
  template<typename T, typename AUX>
  struct BSS_COMPILER_DLLEXPORT cAniStateInterval : public cAniStateDiscrete<T>
  {
    cAniStateInterval(cAniBase* p, delegate<AUX, T> d, delegate<void, AUX> rm) : cAniState(p), _set(d) { assert(_ani->SizeOf() == sizeof(T)); }
    template<ARRAY_TYPE ArrayType, typename Alloc>
    cAniStateInterval(cAnimation<T, ARRAY_TYPE, Alloc>* p, delegate<void, T> d) : cAniState(p), _set(d) {}
  inline ~cAniStateInterval() {
  while(!_queue.Empty()) // Correctly remove everything currently on the queue
  _rmdel(_queue.Pop().second);
  }
  virtual bool Interpolate(double timepassed)
  {
  IDTYPE svar=_timevalues.Size();
  while(_curpair<svar && _timevalues[_curpair].time <= timepassed)
  _addtoqueue(_timevalues[_curpair++].value); // We call all the discrete values because many discrete values are interdependent on each other.

  while(!_queue.Empty() && _queue.Peek().first <= timepassed)
  _rmdel(_queue.Pop().second);
  return _curpair<svar && _queue.Empty();
  }
  protected:
    double _length; // Actual length taking into account duration of intervals
    RMDEL _rmdel; //delegate for removal
    cPriorityQueue<double, AUX, CompT<double>, unsigned int, CARRAY_SIMPLE, QUEUEALLOC> _queue;
  };*/
}

#endif

/*
// Discrete animation with an interval. After the interval has passed, the object is removed using a second delegate function.
template<typename Alloc, typename T, typename AUX, unsigned char TypeID, double(*TODURATION)(const T&), typename RMDEL = delegate<void, AUX>, typename DEL = delegate<AUX, T>, ARRAY_TYPE ARRAY = CARRAY_SIMPLE>
struct BSS_COMPILER_DLLEXPORT AniAttributeInterval : AniAttributeDiscrete<Alloc, T, TypeID, DEL, ARRAY>
{
typedef AttrDefInterval<DEL, RMDEL> ATTRDEF;
typedef AniAttributeBase<Alloc, T, TypeID, ARRAY> ROOT;
typedef AniAttributeDiscrete<Alloc, T, TypeID, DEL, ARRAY> BASE;
typedef typename ROOT::IDTYPE IDTYPE;
typedef std::pair<double, AUX> QUEUEPAIR; // VS2010 can't handle this being inside the rebind for some reason
typedef typename Alloc::template rebind<QUEUEPAIR>::other QUEUEALLOC;
typedef T DATA;
using ROOT::_timevalues;
using ROOT::_curpair;

inline AniAttributeInterval(const AniAttributeInterval& copy) : BASE(copy), _rmdel(copy._rmdel) {}
inline AniAttributeInterval() : BASE(), _rmdel(0, 0), _length(0) {}
inline ~AniAttributeInterval() {
while(!_queue.Empty()) // Correctly remove everything currently on the queue
_rmdel(_queue.Pop().second);
}
virtual bool Interpolate(double timepassed)
{
IDTYPE svar=_timevalues.Size();
while(_curpair<svar && _timevalues[_curpair].time <= timepassed)
_addtoqueue(_timevalues[_curpair++].value); // We call all the discrete values because many discrete values are interdependent on each other.

while(!_queue.Empty() && _queue.Peek().first <= timepassed)
_rmdel(_queue.Pop().second);
return _curpair<svar && _queue.Empty();
}
inline virtual double Length() { return _length; }
virtual double SetKeyFrames(const KeyFrame<TypeID>* frames, IDTYPE num) { BASE::SetKeyFrames(frames, num); _recalclength(); return _length; }
virtual IDTYPE AddKeyFrame(const KeyFrame<TypeID>& frame) //time is given in milliseconds
{
IDTYPE r = BASE::AddKeyFrame(frame);
double t = frame.time+TODURATION(frame.value);
if(t>_length) _length=t;
return r;
}
virtual bool RemoveKeyFrame(IDTYPE ID)
{
if(ID<_timevalues.Size() && _length==_timevalues[ID].time+TODURATION(_timevalues[ID].value)) _recalclength();
return BASE::RemoveKeyFrame(ID);
}
inline virtual void Start()
{
while(!_queue.Empty()) // Correctly remove everything currently on the queue
_rmdel(_queue.Pop().second);
if(!BASE::_attached()) return;
_curpair=1;
if(ROOT::_initzero())
_addtoqueue(_timevalues[0].value);
}
inline virtual AniAttribute* BSS_FASTCALL Clone() const { return new(Alloc::template rebind<AniAttributeInterval>::other::allocate(1)) AniAttributeInterval(*this); }
inline virtual void BSS_FASTCALL CopyAnimation(AniAttribute* ptr) { operator=(*static_cast<AniAttributeInterval*>(ptr)); }
virtual void BSS_FASTCALL Attach(AttrDef* def) { _rmdel = static_cast<ATTRDEF*>(def)->rmdel; BASE::Attach(def); }
inline AniAttributeInterval& operator=(const AniAttributeInterval& right) { BASE::operator=(right); _length=right._length; return *this; }

protected:
inline void _recalclength()
{
_length=0;
double t;
for(IDTYPE i = 0; i < _timevalues.Size(); ++i)
if((t = _timevalues[i].time+TODURATION(_timevalues[i].value))>_length)
_length=t;
}
inline void _addtoqueue(const T& v) { _queue.Push(TODURATION(v), BASE::_del(v)); }

RMDEL _rmdel; //delegate for removal
cPriorityQueue<double, AUX, CompT<double>, unsigned int, CARRAY_SIMPLE, QUEUEALLOC> _queue;
double _length;
};*/
