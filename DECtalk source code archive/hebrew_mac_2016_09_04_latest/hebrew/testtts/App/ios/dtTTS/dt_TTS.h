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

// This corresponds to the name of the library, e.g. [Lua] require "plugin.library"
// where the '.' is replaced with '_'
CORONA_EXPORT int luaopen_dtTTS( lua_State *L );

#endif // _dtTTS_H__
