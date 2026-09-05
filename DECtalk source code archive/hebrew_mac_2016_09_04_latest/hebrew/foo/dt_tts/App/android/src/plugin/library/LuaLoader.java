//
//  LuaLoader.java
//  TemplateApp
//
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

// This corresponds to the name of the Lua library,
// e.g. [Lua] require "plugin.library"
package plugin.library;

import android.app.Activity;
import android.util.Log;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import com.naef.jnlua.LuaState;
import com.naef.jnlua.JavaFunction;
import com.naef.jnlua.NamedJavaFunction;
import com.ansca.corona.CoronaActivity;
import com.ansca.corona.CoronaEnvironment;
import com.ansca.corona.CoronaLua;
import com.ansca.corona.CoronaRuntime;
import com.ansca.corona.CoronaRuntimeListener;

public class LuaLoader implements JavaFunction, CoronaRuntimeListener {

	private CoronaActivity fParentActivity;

	private int fListener;

	// This corresponds to the event name, e.g. [Lua] event.name
	private static final String EVENT_NAME = "pluginlibraryevent";

	/**
	 * Creates a new object for displaying banner ads on the CoronaActivity
	 */
	public LuaLoader() {
		CoronaActivity activity = CoronaEnvironment.getCoronaActivity();

		// Validate.
		if (activity == null) {
			throw new IllegalArgumentException("Activity cannot be null.");
		}
		
		// Initialize member variables.
		fParentActivity = activity;
		fListener = CoronaLua.REFNIL;
	}

	/**
	 * Warning! This method is not called on the main UI thread.
	 */
	@Override
	public int invoke(LuaState L) {
		// LuaState L = runtime.getLuaState();

		// Add functions to library
		NamedJavaFunction[] luaFunctions = new NamedJavaFunction[] {
			new InitWrapper(),
			new ShowWrapper(),
		};

		String libName = L.toString( 1 );
		L.register(libName, luaFunctions);

		return 1;
	}

	// CoronaRuntimeListener
	@Override
	public void onLoaded(CoronaRuntime runtime) {
	}

	// CoronaRuntimeListener
	@Override
	public void onStarted(CoronaRuntime runtime) {
	}

	// CoronaRuntimeListener
	@Override
	public void onSuspended(CoronaRuntime runtime) {
	}

	// CoronaRuntimeListener
	@Override
	public void onResumed(CoronaRuntime runtime) {
	}

	// CoronaRuntimeListener
	@Override
	public void onExiting(CoronaRuntime runtime) {
		LuaState L = runtime.getLuaState();

		CoronaLua.deleteRef( L, fListener );
		fListener = CoronaLua.REFNIL;
	}

	// library.init( listener )
	public int init(LuaState L) {
		int listenerIndex = 1;

		if ( CoronaLua.isListener( L, listenerIndex, EVENT_NAME ) ) {
			fListener = CoronaLua.newRef( L, listenerIndex );
		}

		return 0;
	}

	// library.show( word )
	public int show(LuaState L) {
		String word = L.checkString( 1 );
		if ( null == word ) {
			word = "corona";
		}

		// Create web view on UI thread
		android.os.Handler handler = fParentActivity.getHandler();
		if (handler != null) {
			String baseUrl = "http://dictionary.reference.com/browse/";
			final String url = baseUrl + word;

			// Post the runnable object onto the UI thread's message queue.
			handler.post( new Runnable() {
				public void run() {
					// Do not continue if the activity has just exited out.
					if (null == fParentActivity) {
						return;
					}

					// Create and set up the web view.
					WebView view = new WebView(fParentActivity);

					// Prevent redirect which causes an external browser to be launched
					// because some sites detect phone/tablet and redirect.
					view.setWebViewClient(new WebViewClient() {
						@Override
						public boolean shouldOverrideUrlLoading(WebView view, String url) {
							return false;
						}
					});

					fParentActivity.getOverlayView().addView(view);
					view.loadUrl( url );
				}
			} );
		}

		return 0;
	}

	/**
	 * Posts the given Runnable object to the main UI thread's message queue in a thread safe manner.
	 * @param runnable The Runnable object to be posted and executed on the main UI thread.
	 * @return Returns true if the Runnable object was successfully posted. Returns false if not.
	 */
	private boolean postOnUiThread(Runnable runnable) {
		// Validate.
		if (runnable == null) {
			return false;
		}
		
		// Fetch the activity's Handler needed to post the runnable object.
		android.os.Handler handler = fParentActivity.getHandler();
		if (handler == null) {
			return false;
		}
		
		// Post the runnable object onto the UI thread's message queue.
		return handler.post(runnable);
	}

	private class InitWrapper implements NamedJavaFunction {
		@Override
		public String getName() {
			return "init";
		}
		
		/**
		 * Warning! This method is not called on the main UI thread.
		 */
		@Override
		public int invoke(LuaState L) {
			return init(L);
		}
	}

	private class ShowWrapper implements NamedJavaFunction {
		@Override
		public String getName() {
			return "show";
		}
		
		/**
		 * Warning! This method is not called on the main UI thread.
		 */
		@Override
		public int invoke(LuaState L) {
			return show(L);
		}
	}
}