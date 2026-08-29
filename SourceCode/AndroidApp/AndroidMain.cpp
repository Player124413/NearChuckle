//////////////////////////////////////////////////////////////////////
//
//  Android bootstrap for NearChuckle.
//
//  Runs inside SDL_main (on the SDL thread) before the engine starts:
//   - changes the working directory to the game folder
//     (Android external app storage: /sdcard/Android/data/<pkg>/files)
//   - generates an optimized system.cfg on first launch
//   - tunes SDL hints for touch input
//
//////////////////////////////////////////////////////////////////////

#if defined(__ANDROID__)

#include <SDL3/SDL.h>
#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern "C" void AndroidTouchVibrate(int ms);
void DiagInit(); // AndroidApp/DiagLog.cpp

namespace
{
	bool FileExists(const char *szPath)
	{
		FILE *f = fopen(szPath, "rb");
		if (f)
		{
			fclose(f);
			return true;
		}
		return false;
	}
}

void AndroidBootstrap()
{
	// diagnostics log first: everything after this point is captured
	DiagInit();

	// touch should not synthesize mouse events for the game; the engine
	// has its own touch routing (CSDLTouch). Menus get the synthesized
	// mouse through the same events when the overlay is disabled.
	SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "1");
	SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
	SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");

	const char *szStorage = SDL_GetAndroidExternalStoragePath();
	if (szStorage && szStorage[0])
	{
		if (chdir(szStorage) != 0)
			SDL_Log("AndroidBootstrap: chdir to %s failed", szStorage);
		else
			SDL_Log("AndroidBootstrap: game dir = %s", szStorage);
	}
	else
	{
		SDL_Log("AndroidBootstrap: no external storage path, using internal dir");
		char buf[512];
		const char *szInt = SDL_GetAndroidInternalStoragePath();
		if (szInt && szInt[0] && chdir(szInt) == 0)
		{
			(void)buf;
		}
	}

	// First launch: generate a mobile-optimized system.cfg.
	if (!FileExists("system.cfg"))
	{
		// pick a sane rendering resolution based on the display
		int w = 1280, h = 720;
		SDL_DisplayID disp = SDL_GetPrimaryDisplay();
		const SDL_DisplayMode *mode = SDL_GetDesktopDisplayMode(disp);
		if (!mode)
			mode = SDL_GetCurrentDisplayMode(disp);
		if (mode && mode->w > 0 && mode->h > 0)
		{
			w = mode->w;
			h = mode->h;
		}
		// resolution scale (default 0.75) - huge perf win, still sharp
		float fScale = 0.75f;
		{
			const char *szEnv = SDL_getenv("FARCRY_RES_SCALE");
			if (szEnv && szEnv[0])
				fScale = (float)atof(szEnv);
		}
		if (fScale < 0.4f) fScale = 0.4f;
		if (fScale > 1.0f) fScale = 1.0f;
		w = (int)(w * fScale) & ~7;
		h = (int)(h * fScale) & ~7;
		if (w < 640) w = 640;
		if (h < 360) h = 360;

		FILE *f = fopen("system.cfg", "w");
		if (f)
		{
			fprintf(f, "-- NearChuckle Android: first-boot configuration\n");
			fprintf(f, "-- (delete this file to regenerate optimized defaults)\n");
			fprintf(f, "r_Driver = \"OpenGL\"\n");
			fprintf(f, "r_Width = %d\n", w);
			fprintf(f, "r_Height = %d\n", h);
			fprintf(f, "r_ColorBits = 32\n");
			fprintf(f, "r_DepthBits = 24\n");
			fprintf(f, "r_StencilBits = 8\n");
			fprintf(f, "r_Fullscreen = 0\n");
			fprintf(f, "r_VSync = 1\n");
			// mobile performance defaults
			fprintf(f, "e_shadows = 0\n");
			fprintf(f, "r_Quality_BumpMaps = 0\n");
			fprintf(f, "r_Quality_Textures = 0\n");
			fprintf(f, "r_EnhanceImage = 0\n");
			fprintf(f, "r_FSAA = 0\n");
			fprintf(f, "r_Texture_Anisotropic = 0\n");
			fprintf(f, "e_terrain_texture_lod = 2\n");
			fprintf(f, "e_terrain_normal_lod = 2\n");
			fprintf(f, "sys_skiponlowspec = 1\n");
			fprintf(f, "s_SpeakerConfig = 0\n");
			fprintf(f, "-- touch defaults\n");
			fprintf(f, "touch_enabled = 1\n");
			fclose(f);
			SDL_Log("AndroidBootstrap: wrote system.cfg (%dx%d)", w, h);
		}
	}
}

// haptic feedback from the touch overlay (CryGame)
extern "C" void AndroidTouchVibrate(int ms)
{
	JNIEnv *env = (JNIEnv *)SDL_GetAndroidJNIEnv();
	jobject activity = (jobject)SDL_GetAndroidActivity();
	if (!env || !activity)
		return;

	jclass cls = env->GetObjectClass(activity);
	if (cls)
	{
		jmethodID mid = env->GetMethodID(cls, "vibrate", "(I)V");
		if (mid)
			env->CallVoidMethod(activity, mid, (jint)ms);
		env->DeleteLocalRef(cls);
	}
	if (env->ExceptionCheck())
		env->ExceptionClear();
	env->DeleteLocalRef(activity);
}

#else // !__ANDROID__

// keep the object file non-empty on other platforms (it is never built there)
void AndroidTouchVibrate(int ms)
{
	(void)ms;
}

#endif // __ANDROID__
