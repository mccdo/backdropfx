// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#ifndef __BACKDROPFX_EXPORT__
#define __BACKDROPFX_EXPORT__ 1


#if defined ( _MSC_VER ) || defined ( __CYGWIN__ ) || defined ( __MINGW32__ ) || defined ( __BCPLUSPLUS__ ) || defined ( __MWERKS__ )
 #if defined ( BACKDROPFX_LIBRARY )
  #define BACKDROPFX_EXPORT __declspec( dllexport )
 #else
  #define BACKDROPFX_EXPORT __declspec( dllimport )
 #endif
#else
 #define BACKDROPFX_EXPORT
#endif


// __BACKDROPFX_EXPORT__
#endif
