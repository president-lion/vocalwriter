//
//  dt_TTS.mm
//  DECtalk Text To Speech Lua Interface
//
//  Copyright (c) 2013 Schnee Consulting. All rights reserved.
//

#import "dt_TTS.h"

#include "ttsapi.h"

#include "CoronaRuntime.h"

#import <UIKit/UIKit.h>

static long* phTTS;

// ----------------------------------------------------------------------------

class dtTTS
{
	public:
		typedef dtTTS Self;

	public:
		static const char kName[];
		static const char kEvent[];

	protected:
		dtTTS();

	public:
		bool Initialize( CoronaLuaRef listener );

	public:
		CoronaLuaRef GetListener() const { return fListener; }

	public:
		static int Open( lua_State *L );

	protected:
		static int Finalizer( lua_State *L );

	public:
		static Self *ToLibrary( lua_State *L );

	public:
		static int DT_TextToSpeechSetConfFileInfo( lua_State *L );
		static int DT_TextToSpeechSpeak( lua_State *L );
		static int DT_TextToSpeechStartup( lua_State *L );
		static int DT_TextToSpeechReset( lua_State *L );
		static int DT_TextToSpeechShutdown( lua_State *L);
		static int DT_TextToSpeechGetStatusEx( lua_State *L);
		static int init( lua_State *L );
		static int show( lua_State *L );

	private:
		CoronaLuaRef fListener;
};

// ----------------------------------------------------------------------------

// This corresponds to the name of the library, e.g. [Lua] require "dtTTS"
const char dtTTS::kName[] = "dtTTS";

// This corresponds to the event name, e.g. [Lua] event.name
const char dtTTS::kEvent[] = "dtTTSevent";

dtTTS::dtTTS()
:	fListener( NULL )
{
}

bool
dtTTS::Initialize( CoronaLuaRef listener )
{
	// Can only initialize listener once
	bool result = ( NULL == fListener );

	if ( result )
	{
		fListener = listener;
	}

	return result;
}

int
dtTTS::Open( lua_State *L )
{
	// Register __gc callback
	const char kMetatableName[] = __FILE__; // Globally unique string to prevent collision
	CoronaLuaInitializeGCMetatable( L, kMetatableName, Finalizer );

	// Functions in library
	const luaL_Reg kVTable[] =
	{
          { "DT_TextToSpeechSetConfFileInfo", DT_TextToSpeechSetConfFileInfo },
          { "DT_TextToSpeechStartup", DT_TextToSpeechStartup },
          { "DT_TextToSpeechSpeak", DT_TextToSpeechSpeak },
          { "DT_TextToSpeechReset", DT_TextToSpeechReset },
          { "DT_TextToSpeechShutdown", DT_TextToSpeechShutdown },
          { "DT_TextToSpeechGetStatusEx", DT_TextToSpeechGetStatusEx },
        { NULL, NULL }
	};

        phTTS = NULL;        

	// Set library as upvalue for each library function
	Self *library = new Self;
	CoronaLuaPushUserdata( L, library, kMetatableName );

	luaL_openlib( L, kName, kVTable, 1 ); // leave "library" on top of stack

	return 1;
}

int
dtTTS::Finalizer( lua_State *L )
{
	Self *library = (Self *)CoronaLuaToUserdata( L, 1 );

	CoronaLuaDeleteRef( L, library->GetListener() );

	delete library;

	return 0;
}

dtTTS *
dtTTS::ToLibrary( lua_State *L )
{
	// library is pushed as part of the closure
	Self *library = (Self *)CoronaLuaToUserdata( L, lua_upvalueindex( 1 ) );
	return library;
}

// [Lua] library.init( listener )
int
dtTTS::init( lua_State *L )
{
	int listenerIndex = 1;

	if ( CoronaLuaIsListener( L, listenerIndex, kEvent ) )
	{
		Self *library = ToLibrary( L );

		CoronaLuaRef listener = CoronaLuaNewRef( L, listenerIndex );
		library->Initialize( listener );
	}

	return 0;
}

int dtTTS::DT_TextToSpeechSetConfFileInfo( lua_State *L )
{

        MMRESULT result = 0;
        char *dict_path = (char *)lua_tostring( L, 1);
        char license[] = "YV808w008IQ0";
        char def_lang[] = "HB";
        char langs[] = "hb,HEBREW";
    
        result = TextToSpeechSetConfFileInfo(def_lang,langs,license,dict_path);

        lua_pushnumber( L, result );           
        
	return 1;
}

int dtTTS::DT_TextToSpeechSpeak( lua_State *L )
{

        MMRESULT result = 0;
        char *toSpeak = (char *)lua_tostring( L, 1);
        
    result = TextToSpeechSpeakA(phTTS, toSpeak, TTS_FORCE);

        lua_pushnumber( L, result );           
    
	return 1;
}

int dtTTS::DT_TextToSpeechStartup( lua_State *L )
{
        MMRESULT result = 0;
	
        result = TextToSpeechStartup((LPTTS_HANDLE_T *)(&phTTS), -1, 0, NULL, NULL );

        lua_pushnumber( L, result );           
               
        return 1;
}

int dtTTS::DT_TextToSpeechReset( lua_State *L )
{
        MMRESULT result = 0;

        result = TextToSpeechReset( phTTS, TRUE );

        lua_pushnumber( L, result );           
               
        return 1;
}

int dtTTS::DT_TextToSpeechShutdown( lua_State *L)
{
         MMRESULT result = 0;

        result = TextToSpeechShutdown( phTTS );

    phTTS = NULL;
        lua_pushnumber( L, result );
               
        return 1;       
}

int dtTTS::DT_TextToSpeechGetStatusEx( lua_State *L)
{
        MMRESULT result = 0;
        int iPlayStatus = 0;
        int iCurIndex = 0;
        unsigned int uiInstanceParam = 0;

    result = TextToSpeechGetStatusEx( phTTS, &iPlayStatus, &iCurIndex, &uiInstanceParam);

        lua_pushnumber( L, result );
        lua_pushnumber( L, iPlayStatus );           
        lua_pushnumber( L, iCurIndex );           
               
        return 3;       
}

// [Lua] library.show( word )
int
dtTTS::show( lua_State *L )
{
	NSString *message = @"Error: Could not display UIReferenceLibraryViewController. This feature requires iOS 5 or later.";
	
	if ( [UIReferenceLibraryViewController class] )
	{
		id<CoronaRuntime> runtime = (id<CoronaRuntime>)CoronaLuaGetContext( L );

		const char kDefaultWord[] = "corona";
		const char *word = lua_tostring( L, 1 );
		if ( ! word )
		{
			word = kDefaultWord;
		}

		UIReferenceLibraryViewController *controller = [[[UIReferenceLibraryViewController alloc] initWithTerm:[NSString stringWithUTF8String:word]] autorelease];

		// Present the controller modally.
		[runtime.appViewController presentModalViewController:controller animated:YES];

		message = @"Success. Displaying UIReferenceLibraryViewController for 'corona'.";
	}

	Self *library = ToLibrary( L );

	// Create event and add message to it
	CoronaLuaNewEvent( L, kEvent );
	lua_pushstring( L, [message UTF8String] );
	lua_setfield( L, -2, "message" );

	// Dispatch event to library's listener
	CoronaLuaDispatchEvent( L, library->GetListener(), 0 );

	return 0;
}

// ----------------------------------------------------------------------------

CORONA_EXPORT int luaopen_dtTTS( lua_State *L )
{
	return dtTTS::Open( L );
}
