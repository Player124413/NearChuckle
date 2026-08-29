package app.nearchuckle.farcry;

import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.VibrationEffect;
import android.os.Vibrator;
import android.os.VibratorManager;
import android.provider.MediaStore;
import android.content.ContentValues;
import android.util.Log;

import org.libsdl.app.SDLActivity;

import java.io.File;
import java.io.FileWriter;
import java.io.OutputStream;

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
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

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
        // SDL prepends its own argv[0]; anything returned here becomes argv[1..]
        // and the engine executes argv as console commands (that is where the
        // bogus "Unknown command: NearChuckle" log line came from).
        return new String[0];
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

    /** Opens the Android share sheet with the log text (no PC needed). */
    public void offerLogShare(final String text) {
        try {
            runOnUiThread(new Runnable() {
                public void run() {
                    try {
                        Intent send = new Intent();
                        send.setAction(Intent.ACTION_SEND);
                        send.putExtra(Intent.EXTRA_TEXT, text);
                        send.setType("text/plain");
                        startActivity(Intent.createChooser(send, "NearChuckle log"));
                    } catch (Throwable t) {
                        Log.w(TAG, "share failed: " + t);
                    }
                }
            });
        } catch (Throwable t) {
            Log.w(TAG, "share post failed: " + t);
        }
    }

    /** Saves the log into the public Downloads folder. Returns location or null. */
    public String saveLog(String name, String content) {
        try {
            if (Build.VERSION.SDK_INT >= 29) {
                ContentValues cv = new ContentValues();
                cv.put(MediaStore.MediaColumns.DISPLAY_NAME, name);
                cv.put(MediaStore.MediaColumns.MIME_TYPE, "text/plain");
                cv.put(MediaStore.MediaColumns.RELATIVE_PATH, Environment.DIRECTORY_DOWNLOADS);
                Uri uri = getContentResolver().insert(
                    MediaStore.Downloads.EXTERNAL_CONTENT_URI, cv);
                if (uri == null) return null;
                OutputStream os = getContentResolver().openOutputStream(uri);
                if (os == null) return null;
                try {
                    os.write(content.getBytes("UTF-8"));
                } finally {
                    os.close();
                }
                return uri.toString();
            } else {
                File dir = Environment.getExternalStoragePublicDirectory(
                    Environment.DIRECTORY_DOWNLOADS);
                File f = new File(dir, name);
                FileWriter w = new FileWriter(f);
                w.write(content);
                w.close();
                return f.getAbsolutePath();
            }
        } catch (Throwable t) {
            Log.w(TAG, "saveLog failed: " + t);
            return null;
        }
    }
}
