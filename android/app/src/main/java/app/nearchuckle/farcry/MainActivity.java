package app.nearchuckle.farcry;

import android.content.Context;
import android.os.Build;
import android.os.VibrationEffect;
import android.os.Vibrator;
import android.os.VibratorManager;
import android.util.Log;

import org.libsdl.app.SDLActivity;

/**
 * NearChuckle launcher.
 *
 * The engine modules (libCrySystem.so, libCryGame.so, ...) are loaded by
 * libCrySystem itself via dlopen(RTLD_GLOBAL), exactly like on Linux, so
 * Java only needs to load SDL3 and the entry point library (libmain.so).
 */
public class MainActivity extends SDLActivity {

    private static final String TAG = "NearChuckle";

    @Override
    protected String[] getLibraries() {
        return new String[] {
            "SDL3",
            "main"
        };
    }

    @Override
    protected String getMainSharedObject() {
        return "libmain.so";
    }

    @Override
    protected String[] getArguments() {
        // no command line needed; the engine reads system.cfg
        return new String[] { "NearChuckle" };
    }

    /** Haptic feedback for the touch controls (called via JNI). */
    public static void vibrateStatic(Context context, int ms) {
        try {
            Vibrator vib = null;
            if (Build.VERSION.SDK_INT >= 31) {
                VibratorManager vm = (VibratorManager)
                    context.getSystemService(Context.VIBRATOR_MANAGER_SERVICE);
                if (vm != null) vib = vm.getDefaultVibrator();
            } else {
                vib = (Vibrator) context.getSystemService(Context.VIBRATOR_SERVICE);
            }
            if (vib == null || !vib.hasVibrator()) return;
            if (Build.VERSION.SDK_INT >= 26) {
                vib.vibrate(VibrationEffect.createOneShot(
                    Math.max(1, Math.min(ms, 200)), VibrationEffect.DEFAULT_AMPLITUDE));
            } else {
                //noinspection deprecation
                vib.vibrate(ms);
            }
        } catch (Throwable t) {
            Log.w(TAG, "vibrate failed: " + t);
        }
    }

    /** Instance wrapper called through SDL_GetAndroidActivity(). */
    public void vibrate(int ms) {
        vibrateStatic(this, ms);
    }
}
