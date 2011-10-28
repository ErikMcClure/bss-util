// Copyright �2011 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in "bss_util.h"

#ifndef __C_SINGLETON_H__BSS__ //These are used in case this header file is used by two different projects dependent on each other, resulting in duplicates which cannot be differentiated by #pragma once
#define __C_SINGLETON_H__BSS__

#include "bss_compiler.h"

namespace bss_util {
  template<class T>
  class BSS_COMPILER_DLLEXPORT cSingleton //exported to make VC++ shut up
  {
  public:
    cSingleton(T* ptr) { _ptr = ptr; _instance = _ptr; }
    ~cSingleton() { if(_instance == _ptr) _instance = 0; }
    
    static T* Instance() { return _instance; }
    static T& InstRef() { return *_instance; }

  protected:
    static T* _instance;

  private:
    T* _ptr;
  };

  template<class T>
  T* cSingleton<T>::_instance=0;
}

#endif