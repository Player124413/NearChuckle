package app.nearchuckle.farcry;

import android.app.Activity;
import android.content.Intent;
import android.database.Cursor;
import android.graphics.Color;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.provider.DocumentsContract;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

/**
 * NearChuckle launcher.
 *
 * Lets the user pick a game-data ZIP or a game folder with the system file
 * picker; the archive/folder is extracted/copied into the app's own external
 * files dir (Android/data/app.nearchuckle.farcry/files) - the exact place
 * the engine expects - and a big PLAY button starts the game.
 *
 * No PC, no file manager, no storage permissions needed: reading a
 * user-picked document is granted by the picker itself, and writing into
 * the app's own external dir is always allowed.
 */
public class LauncherActivity extends Activity {

    private static final int REQ_PICK_ZIP = 41;
    private static final int REQ_PICK_DIR = 42;

    private TextView mStatus;
    private TextView mProgress;
    private Button mPlay;
    private Button mPickZip;
    private Button mPickDir;
    private volatile boolean mImporting = false;

    // ------------------------------------------------------------------ UI
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(buildUi());
        refreshStatus();
    }

    private View buildUi() {
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setGravity(Gravity.CENTER_HORIZONTAL);
        root.setPadding(dp(16), dp(12), dp(16), dp(12));
        root.setBackgroundColor(Color.rgb(14, 16, 24));

        TextView title = new TextView(this);
        title.setText("NearChuckle");
        title.setTextColor(Color.rgb(120, 200, 255));
        title.setTextSize(26);
        title.setGravity(Gravity.CENTER);
        root.addView(title, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));

        mStatus = new TextView(this);
        mStatus.setTextSize(15);
        mStatus.setGravity(Gravity.CENTER);
        mStatus.setPadding(0, dp(8), 0, dp(4));
        root.addView(mStatus, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));

        mPlay = new Button(this);
        mPlay.setText("▶  PLAY");
        mPlay.setTextSize(22);
        mPlay.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) { startGame(); }
        });
        root.addView(mPlay, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, dp(64)));

        mProgress = new TextView(this);
        mProgress.setTextColor(Color.rgb(180, 190, 205));
        mProgress.setTextSize(13);
        mProgress.setGravity(Gravity.CENTER);
        mProgress.setPadding(0, dp(6), 0, dp(2));
        root.addView(mProgress, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));

        LinearLayout row = new LinearLayout(this);
        row.setOrientation(LinearLayout.HORIZONTAL);
        row.setGravity(Gravity.CENTER);

        mPickZip = new Button(this);
        mPickZip.setText("Выбрать ZIP игры");
        mPickZip.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) { pickZip(); }
        });
        row.addView(mPickZip, new LinearLayout.LayoutParams(0,
                ViewGroup.LayoutParams.WRAP_CONTENT, 1f));

        mPickDir = new Button(this);
        mPickDir.setText("Выбрать папку игры");
        mPickDir.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) { pickFolder(); }
        });
        row.addView(mPickDir, new LinearLayout.LayoutParams(0,
                ViewGroup.LayoutParams.WRAP_CONTENT, 1f));

        root.addView(row, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));

        TextView help = new TextView(this);
        help.setText("В архиве/папке нужен Far Cry с папкой FCData.\n"
                + "Данные установятся автоматически, вложенная папка\n"
                + "(например \"Far Cry/\") находится сама.");
        help.setTextColor(Color.rgb(140, 150, 165));
        help.setTextSize(12);
        help.setGravity(Gravity.CENTER);
        help.setPadding(0, dp(10), 0, dp(4));
        root.addView(help, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));

        Button logBtn = new Button(this);
        logBtn.setText("Поделиться логом");
        logBtn.setTextSize(13);
        logBtn.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) { shareLog(); }
        });
        root.addView(logBtn, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));

        ScrollView sc = new ScrollView(this);
        sc.addView(root);
        return sc;
    }

    private int dp(int v) {
        return Math.round(v * getResources().getDisplayMetrics().density);
    }

    // ------------------------------------------------------------- status
    private File filesDir() {
        File f = getExternalFilesDir(null);
        return f != null ? f : getFilesDir();
    }

    private boolean gameDataValid(File base) {
        File fc = new File(base, "FCData");
        if (!fc.isDirectory())
            return false;
        File[] fs = fc.listFiles();
        if (fs != null)
            for (File f : fs)
                if (f.isFile() && f.getName().toLowerCase().endsWith(".pak"))
                    return true;
        // one level deeper (FCData/paks/...)
        File[] dirs = fc.listFiles();
        if (dirs != null)
            for (File d : dirs)
                if (d.isDirectory())
                    for (File f : d.listFiles())
                        if (f.isFile() && f.getName().toLowerCase().endsWith(".pak"))
                            return true;
        return false;
    }

    private void refreshStatus() {
        boolean ok = gameDataValid(filesDir());
        mPlay.setEnabled(ok);
        if (ok) {
            mStatus.setText("Данные игры на месте ✓");
            mStatus.setTextColor(Color.rgb(120, 230, 140));
        } else {
            mStatus.setText("Данные игры не найдены — выбери ZIP или папку");
            mStatus.setTextColor(Color.rgb(255, 190, 90));
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        refreshStatus();
    }

    private void startGame() {
        try {
            startActivity(new Intent(this, MainActivity.class));
        } catch (Throwable t) {
            toast("Не удалось запустить: " + t);
        }
    }

    // ------------------------------------------------------------ pickers
    private void pickZip() {
        if (mImporting) return;
        Intent i = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        i.addCategory(Intent.CATEGORY_OPENABLE);
        i.setType("*/*");
        try {
            startActivityForResult(
                    Intent.createChooser(i, "Выбери ZIP-архив игры"), REQ_PICK_ZIP);
        } catch (Throwable t) {
            toast("Файл выбран не был: " + t);
        }
    }

    private void pickFolder() {
        if (mImporting) return;
        Intent i = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
        try {
            startActivityForResult(i, REQ_PICK_DIR);
        } catch (Throwable t) {
            toast("Папка выбрана не была: " + t);
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (resultCode != RESULT_OK || data == null || data.getData() == null)
            return;
        if (requestCode == REQ_PICK_ZIP) {
            startImport(data.getData(), true);
        } else if (requestCode == REQ_PICK_DIR) {
            startImport(data.getData(), false);
        }
    }

    // ------------------------------------------------------------- import
    private void setBusy(boolean busy) {
        mImporting = busy;
        mPlay.setEnabled(!busy && gameDataValid(filesDir()));
        mPickZip.setEnabled(!busy);
        mPickDir.setEnabled(!busy);
    }

    private void startImport(final Uri uri, final boolean isZip) {
        if (mImporting) return;
        setBusy(true);
        progress(isZip ? "Читаю архив…" : "Копирую папку…");
        new Thread(new Runnable() {
            public void run() {
                String err = null;
                try {
                    doImport(uri, isZip);
                } catch (Throwable t) {
                    err = t.toString();
                }
                final String error = err;
                runOnUiThread(new Runnable() {
                    public void run() {
                        setBusy(false);
                        refreshStatus();
                        if (error == null && gameDataValid(filesDir())) {
                            progress("Установка завершена ✓");
                            toast("Данные игры установлены! Жми PLAY");
                        } else if (error != null) {
                            progress("Ошибка: " + error);
                            toast("Ошибка установки — см. текст на экране");
                        } else {
                            progress("В архиве не найдена папка FCData — это не те данные игры?");
                        }
                    }
                });
            }
        }).start();
    }

    private void doImport(Uri uri, boolean isZip) throws IOException {
        File files = filesDir();
        File tmp = new File(files, ".import_tmp");
        deleteR(tmp);
        if (!tmp.mkdirs() && !tmp.isDirectory())
            throw new IOException("не удалось создать временную папку");
        try {
            if (isZip)
                extractZip(uri, tmp);
            else
                copyTree(uri, tmp);
            progress("Расставляю файлы…");
            if (!promote(tmp, files))
                throw new IOException("FCData не найдена внутри");
        } finally {
            deleteR(tmp);
        }
    }

    // -------------------------------------------------------------- zip
    private void extractZip(Uri uri, File tmp) throws IOException {
        InputStream in = getContentResolver().openInputStream(uri);
        if (in == null)
            throw new IOException("архив недоступен");
        BufferedInputStream bin = new BufferedInputStream(in, 1 << 16);
        ZipInputStream zin = new ZipInputStream(bin);
        try {
            String canonTmp = tmp.getCanonicalPath();
            int n = 0;
            ZipEntry e;
            byte[] buf = new byte[1 << 16];
            while ((e = zin.getNextEntry()) != null) {
                String name = e.getName().replace('\\', '/');
                while (name.startsWith("/"))
                    name = name.substring(1);
                if (name.length() == 0)
                    continue;
                if (name.contains("../"))
                    continue; // zip-slip guard
                File out = new File(tmp, name);
                if (!out.getCanonicalPath().startsWith(canonTmp))
                    continue;
                if (e.isDirectory()) {
                    out.mkdirs();
                    continue;
                }
                File parent = out.getParentFile();
                if (parent != null && !parent.isDirectory())
                    parent.mkdirs();
                OutputStream os = new BufferedOutputStream(new FileOutputStream(out), 1 << 16);
                try {
                    int r;
                    while ((r = zin.read(buf)) > 0)
                        os.write(buf, 0, r);
                } finally {
                    os.close();
                }
                zin.closeEntry();
                n++;
                if ((n & 31) == 0)
                    progress("Распаковано файлов: " + n);
            }
        } finally {
            zin.close();
        }
    }

    // ------------------------------------------------------------ folder
    private void copyTree(Uri treeUri, File tmp) throws IOException {
        String rootDocId = DocumentsContract.getTreeDocumentId(treeUri);
        copyDocs(treeUri, rootDocId, tmp, new int[]{0});
    }

    private void copyDocs(Uri treeUri, String parentDocId, File dstDir, int[] counter)
            throws IOException {
        if (!dstDir.isDirectory() && !dstDir.mkdirs())
            throw new IOException("нельзя создать " + dstDir);
        Uri children = DocumentsContract.buildChildDocumentsUriUsingTree(treeUri, parentDocId);
        ArrayList<String[]> kids = new ArrayList<String[]>();
        Cursor c = getContentResolver().query(children, new String[]{
                DocumentsContract.Document.COLUMN_DOCUMENT_ID,
                DocumentsContract.Document.COLUMN_DISPLAY_NAME,
                DocumentsContract.Document.COLUMN_MIME_TYPE}, null, null, null);
        if (c != null) {
            try {
                while (c.moveToNext())
                    kids.add(new String[]{c.getString(0), c.getString(1), c.getString(2)});
            } finally {
                c.close();
            }
        }
        byte[] buf = new byte[1 << 16];
        for (String[] k : kids) {
            String docId = k[0], name = k[1], mime = k[2];
            if (name == null) name = docId;
            name = name.replace('\\', '/');
            if (name.contains("../"))
                continue;
            File out = new File(dstDir, name);
            boolean isDir = DocumentsContract.Document.MIME_TYPE_DIR.equals(mime);
            if (isDir) {
                copyDocs(treeUri, docId, out, counter);
            } else {
                Uri docUri = DocumentsContract.buildDocumentUriUsingTree(treeUri, docId);
                InputStream is = getContentResolver().openInputStream(docUri);
                if (is == null)
                    continue;
                OutputStream os = new BufferedOutputStream(new FileOutputStream(out), 1 << 16);
                try {
                    int r;
                    while ((r = is.read(buf)) > 0)
                        os.write(buf, 0, r);
                } finally {
                    is.close();
                    os.close();
                }
                counter[0]++;
                if ((counter[0] & 31) == 0)
                    progress("Скопировано файлов: " + counter[0]);
            }
        }
    }

    // ------------------------------------------------------------ helpers
    /** Finds the folder that actually contains FCData (depth <= 2). */
    private File findGameRoot(File dir, int depth) {
        if (new File(dir, "FCData").isDirectory())
            return dir;
        if (depth >= 2)
            return null;
        File[] kids = dir.listFiles();
        if (kids == null)
            return null;
        for (File k : kids)
            if (k.isDirectory()) {
                File r = findGameRoot(k, depth + 1);
                if (r != null)
                    return r;
            }
        return null;
    }

    /** Moves the game root's children into filesDir (fresh import wins). */
    private boolean promote(File tmp, File filesDir) throws IOException {
        File root = findGameRoot(tmp, 0);
        if (root == null)
            return false;
        File[] kids = root.listFiles();
        if (kids == null || kids.length == 0)
            return false;
        for (File k : kids) {
            File dst = new File(filesDir, k.getName());
            if (dst.exists())
                deleteR(dst);
            if (!k.renameTo(dst)) {
                copyR(k, dst);
                deleteR(k);
            }
        }
        return true;
    }

    private void deleteR(File f) {
        if (f == null || !f.exists())
            return;
        File[] kids = f.listFiles();
        if (kids != null)
            for (File k : kids)
                deleteR(k);
        //noinspection ResultOfMethodCallIgnored
        f.delete();
    }

    private void copyR(File src, File dst) throws IOException {
        if (src.isDirectory()) {
            if (!dst.isDirectory() && !dst.mkdirs())
                throw new IOException("нельзя создать " + dst);
            File[] kids = src.listFiles();
            if (kids != null)
                for (File k : kids)
                    copyR(k, new File(dst, k.getName()));
        } else {
            File parent = dst.getParentFile();
            if (parent != null && !parent.isDirectory())
                parent.mkdirs();
            FileInputStream in = new FileInputStream(src);
            FileOutputStream out = new FileOutputStream(dst);
            byte[] buf = new byte[1 << 16];
            try {
                int r;
                while ((r = in.read(buf)) > 0)
                    out.write(buf, 0, r);
            } finally {
                in.close();
                out.close();
            }
        }
    }

    private void progress(final String s) {
        runOnUiThread(new Runnable() {
            public void run() { mProgress.setText(s); }
        });
    }

    private void toast(final String s) {
        runOnUiThread(new Runnable() {
            public void run() { Toast.makeText(LauncherActivity.this, s, Toast.LENGTH_LONG).show(); }
        });
    }

    // ---------------------------------------------------------- log share
    private void shareLog() {
        StringBuilder sb = new StringBuilder();
        sb.append("======= NearChuckle log =======\n");
        appendTail(new File(filesDir(), "diag.txt"), sb);
        sb.append("\n======= engine log.txt =======\n");
        appendTail(new File(filesDir(), "log.txt"), sb);
        if (sb.indexOf("NearChuckle run started") < 0 && sb.indexOf("Loading") < 0)
            sb.append("(лог пуст - запусти игру сначала)\n");
        try {
            Intent send = new Intent();
            send.setAction(Intent.ACTION_SEND);
            send.putExtra(Intent.EXTRA_TEXT, sb.toString());
            send.setType("text/plain");
            startActivity(Intent.createChooser(send, "NearChuckle log"));
        } catch (Throwable t) {
            toast("Не удалось открыть меню отправки: " + t);
        }
    }

    private void appendTail(File f, StringBuilder sb) {
        try {
            long len = f.length();
            long skip = len > 120 * 1024 ? len - 120 * 1024 : 0;
            FileInputStream in = new FileInputStream(f);
            try {
                in.skip(skip);
                byte[] buf = new byte[8192];
                int r;
                while ((r = in.read(buf)) > 0)
                    sb.append(new String(buf, 0, r, "UTF-8"));
            } finally {
                in.close();
            }
        } catch (Throwable ignored) {
        }
    }
}
