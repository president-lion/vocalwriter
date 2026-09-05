//
//  dtTTS.h
//  DECtalk Text To Speech Lua Interface
//
//  Copyright (c) 2013 Schnee Consulting. All rights reserved.
//

#ifndef _dtTTS_H__
#define _dtTTS_H__

#include "CoronaLua.h"
#include "CoronaMacros.h"


#define BOOL_DEFINED

#if !defined(OBJC_HIDE_64) && TARGET_OS_IPHONE && __LP64__
typedef bool BOOL;
#else
typedef signed char BOOL;
// BOOL is explicitly signed so @encode(BOOL) == "c" rather than "C"
// even if -funsigned-char is used.
#endif

// This corresponds to the name of the library, e.g. [Lua] require "plugin.library"
// where the '.' is replaced with '_'
CORONA_EXPORT int luaopen_dtTTS( lua_State *L );

#endif // _dtTTS_H__
