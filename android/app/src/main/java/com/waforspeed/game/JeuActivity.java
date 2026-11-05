// WaForSpeed — activité Android du jeu (WFS-29).
// Le jeu reste entièrement natif (NativeActivity + android_main) ; cette petite sous-classe ne fait que ce qui doit se
// faire sur le thread de l'interface Android :
//   - mode immersif collant : barres d'état et de navigation masquées, qui reviennent un instant au balayage du bord,
//     réappliqué au démarrage, à la reprise et à chaque retour du focus (onWindowFocusChanged) ;
//   - écran toujours allumé pendant le jeu ;
//   - image sous l'encoche (aussi dans les thèmes values-v28 et values-v30) ; bord à bord d'Android 15 (WFS-129) ;
//   - manettes (WFS-22) : une manette connue qui se déconnecte est signalée au jeu (pause automatique en course) ;
//   - touche Retour au menu principal (WFS-116) : arrière-plan au lieu de fermer le jeu ;
//   - duel entre amis (WFS-112) : lien d'invitation reçu (waforspeed://duel/<code>, au lancement ou jeu déjà ouvert),
//     lu par le jeu (prendreLienDuel) ; « Inviter » ouvre la feuille de partage d'Android (partagerTexte) ;
//   - performances soutenues (WFS-123, RG-05) : Window.setSustainedPerformanceMode(true) à la reprise si le téléphone
//     le permet (PowerManager.isSustainedPerformanceModeSupported), coupé à la mise en pause ;
//   - rapport de plantage (WFS-126) : plantage Java écrit dans plantage.txt (RG-03) ; au lancement suivant, boîte
//     « Le jeu s'est fermé la dernière fois » avec « Partager le rapport » / « Ignorer » (RG-04) ; déclencheur de test
//     `--ez wfs_plantage true`, APK de test seulement (RG-05). Le plantage natif est écrit par plantage.cpp.
//   - nouvelle version (WFS-132) : au lancement, Release `apercu` lue dans un thread d'arrière-plan (au plus toutes les
//     6 h) ; boîte « Nouvelle version disponible » / « Télécharger » si la construction distante est plus récente
//     (règle dans MiseAJour.java) ; déclencheur de test `--ez wfs_maj_test true` ; boutons de volume sur le son du jeu.
package com.waforspeed.game;

import android.app.AlertDialog;
import android.app.NativeActivity;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageInfo;
import android.net.Uri;
import android.hardware.input.InputManager;
import android.media.AudioManager;
import android.os.Build;
import android.os.Bundle;
import android.os.PowerManager;
import android.os.SystemClock;
import android.util.Log;
import android.view.DisplayCutout;
import android.view.InputDevice;
import android.view.View;
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.view.WindowManager;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.io.PrintWriter;
import java.io.ByteArrayOutputStream;
import java.io.StringWriter;
import java.nio.charset.StandardCharsets;
import java.util.HashSet;

public class JeuActivity extends NativeActivity implements InputManager.InputDeviceListener {

    // la bibliothèque du jeu (déjà chargée par NativeActivity) doit l'être aussi par Java pour la fonction JNI ci-dessous
    static { System.loadLibrary("waforspeed"); }

    // marges de sécurité (WFS-51) : encoche / coins, en pixels de la fenêtre ; lues par la boucle de jeu (android_main)
    private static native void nativeMarges(int gauche, int haut, int droite, int bas);
    // manettes (WFS-22) : une manette connue est partie (le jeu se met en pause s'il était piloté à la manette)
    private static native void nativeManetteRetiree();
    // WFS-126 : version de l'appli et déclencheur de test, donnés avant le démarrage du thread du jeu (plantage_android.cpp)
    private static native void nativeInfosPlantage(String version, int code, boolean declencheur);
    private final HashSet<Integer> manettes = new HashSet<>();
    private String derniersEncarts = "";   // WFS-129 : dernières marges écrites au journal
    private InputManager entrees;

    // WFS-112 : dernier lien d'invitation reçu, pris une fois par le jeu (reseau_android.cpp, fil du jeu)
    private static volatile String lienDuel = null;
    public static String prendreLienDuel() { final String l = lienDuel; lienDuel = null; return l; }
    private static void lireLien(Intent i) {
        if (i == null) return;
        final Uri u = i.getData();
        if (u != null) lienDuel = u.toString();
    }

    @Override
    protected void onNewIntent(Intent i) {   // jeu déjà ouvert (singleTask) : le lien arrive ici
        super.onNewIntent(i);
        setIntent(i);
        lireLien(i);
    }

    // WFS-112 : « Inviter » (appelé par le jeu) : feuille de partage (WhatsApp, SMS…) avec le code et le lien
    public void partagerTexte(final String texte) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                final Intent envoi = new Intent(Intent.ACTION_SEND);
                envoi.setType("text/plain");
                envoi.putExtra(Intent.EXTRA_TEXT, texte);
                try { startActivity(Intent.createChooser(envoi, "Inviter un ami")); } catch (ActivityNotFoundException e) { /* rien pour partager */ }
            }
        });
    }

    // ---------- rapport de plantage (WFS-126) ----------
    private String versionNom = "?";
    private int versionCode = 0;

    @SuppressWarnings("deprecation")
    private void lireVersion() {
        try {
            final PackageInfo pi = getPackageManager().getPackageInfo(getPackageName(), 0);
            versionNom = pi.versionName != null ? pi.versionName : "?";
            versionCode = Build.VERSION.SDK_INT >= Build.VERSION_CODES.P ? (int) pi.getLongVersionCode() : pi.versionCode;
        } catch (Exception e) { /* version inconnue : « ? » */ }
    }

    // RG-05 : APK de test seulement = pas installé par le Play Store (APK de la CI posé à la main ou par adb)
    @SuppressWarnings("deprecation")
    private boolean apkDeTest() {
        String installeur = null;
        try {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) installeur = getPackageManager().getInstallSourceInfo(getPackageName()).getInstallingPackageName();
            else installeur = getPackageManager().getInstallerPackageName(getPackageName());
        } catch (Exception e) { /* inconnu : traité comme un APK de test */ }
        return !"com.android.vending".equals(installeur);
    }

    // RG-03 : plantage Java → pile Java dans plantage.txt, puis le gestionnaire d'origine (fermeture habituelle d'Android)
    private void installerPlantageJava() {
        final Thread.UncaughtExceptionHandler origine = Thread.getDefaultUncaughtExceptionHandler();
        final File fichier = new File(getFilesDir(), "plantage.txt");
        final String entete = "WaForSpeed — rapport de plantage (Java)\nversion : " + versionNom + " (" + versionCode + ")\nappareil : "
                + Build.MANUFACTURER + " " + Build.MODEL + ", Android " + Build.VERSION.RELEASE + " (API " + Build.VERSION.SDK_INT + ")\n";
        Thread.setDefaultUncaughtExceptionHandler(new Thread.UncaughtExceptionHandler() {
            @Override
            public void uncaughtException(Thread t, Throwable e) {
                try (FileOutputStream f = new FileOutputStream(fichier)) {
                    final StringWriter pile = new StringWriter();
                    e.printStackTrace(new PrintWriter(pile));
                    f.write((entete + "thread : " + t.getName() + "\npile Java :\n" + pile + "--- fin du rapport ---\n").getBytes(StandardCharsets.UTF_8));
                } catch (Throwable ignore) { /* rien de plus à faire pendant un plantage */ }
                if (origine != null) origine.uncaughtException(t, e);
            }
        });
    }

    // RG-04 : rapport du lancement précédent lu puis renommé en plantage-precedent.txt (un seul gardé) ; null s'il n'y en a pas
    private String prendreRapportPrecedent() {
        final File f = new File(getFilesDir(), "plantage.txt");
        if (!f.exists()) return null;
        String texte;
        try (FileInputStream in = new FileInputStream(f)) {
            final byte[] b = new byte[(int) Math.min(f.length(), 512 * 1024)];
            int n = 0, k;
            while (n < b.length && (k = in.read(b, n, b.length - n)) > 0) n += k;
            texte = new String(b, 0, n, StandardCharsets.UTF_8);
        } catch (Exception e) { texte = "(rapport illisible : " + e + ")"; }
        final File precedent = new File(getFilesDir(), "plantage-precedent.txt");
        if (precedent.exists() && !precedent.delete()) Log.w("WaForSpeed", "PLANTAGE ancien rapport non effacé");
        final boolean ok = f.renameTo(precedent);
        Log.i("WaForSpeed", "PLANTAGE rapport précédent trouvé (" + texte.length() + " caractères) → plantage-precedent.txt" + (ok ? "" : " (renommage impossible)"));
        if (!ok && !f.delete()) Log.w("WaForSpeed", "PLANTAGE rapport non effacé");
        return texte;
    }

    // RG-04 : boîte sombre avant le jeu (le jeu se charge derrière et va au menu) ; aucun envoi sans geste du joueur
    private void proposerRapport(final String rapport) {
        final AlertDialog boite = new AlertDialog.Builder(this, android.R.style.Theme_Material_Dialog_Alert)
            .setTitle(R.string.plantage_titre)
            .setMessage(R.string.plantage_message)
            .setCancelable(true)
            .setPositiveButton(R.string.plantage_partager, new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface d, int which) {
                    final Intent envoi = new Intent(Intent.ACTION_SEND);
                    envoi.setType("text/plain");
                    envoi.putExtra(Intent.EXTRA_SUBJECT, getString(R.string.plantage_sujet, versionNom));
                    envoi.putExtra(Intent.EXTRA_TEXT, rapport);
                    try { startActivity(Intent.createChooser(envoi, getString(R.string.plantage_choisir))); }
                    catch (ActivityNotFoundException e) { Log.w("WaForSpeed", "PLANTAGE aucune appli pour partager"); }
                    Log.i("WaForSpeed", "PLANTAGE rapport : « Partager »");
                }
            })
            .setNegativeButton(R.string.plantage_ignorer, new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface d, int which) { Log.i("WaForSpeed", "PLANTAGE rapport : « Ignorer »"); }
            })
            .create();
        boite.setOnShowListener(new DialogInterface.OnShowListener() {
            @Override
            public void onShow(DialogInterface d) {   // boutons aux couleurs du jeu (orange, cyan)
                boite.getButton(AlertDialog.BUTTON_POSITIVE).setTextColor(getColor(R.color.orange));
                boite.getButton(AlertDialog.BUTTON_NEGATIVE).setTextColor(getColor(R.color.cyan));
            }
        });
        boite.show();
    }

    // ---------- nouvelle version de l'APK (WFS-132) ----------
    private static final String MAJ_SOURCE = "https://api.github.com/repos/wlfaleh/waforspeed_mobile/releases/tags/apercu";
    private static final long MAJ_ECART_MS = 6L * 3600 * 1000;   // RG-01 : au plus une fois toutes les 6 h
    private static final long MAJ_FENETRE_MS = 8000;              // RG-06 : réponse montrée seulement dans les 8 s
    private static final int MAJ_DELAI_MS = 5000;                 // RG-01 : connexion et lecture

    // RG-01 / RG-06 : jamais sur le thread du jeu ni de l'interface ; le démarrage n'attend jamais la réponse
    private void verifierMiseAJour(boolean plantageAffiche, boolean test) {
        final SharedPreferences prefs = getSharedPreferences("wfs_maj", MODE_PRIVATE);
        if (test) {   // RG-08 : boîte avec N = versionCode + 1, sans réseau
            Log.i("WaForSpeed", "MISEAJOUR déclencheur de test");
            proposerMiseAJour(new MiseAJour.Proposition(versionCode + 1, "0000000",
                "https://github.com/wlfaleh/waforspeed_mobile/releases/download/apercu/WaForSpeed.apk"), prefs);
            return;
        }
        if (plantageAffiche) { Log.i("WaForSpeed", "MISEAJOUR reportée (boîte de plantage affichée)"); return; }
        if (versionCode == 1) { Log.i("WaForSpeed", "MISEAJOUR APK hors CI (versionCode 1) : aucune vérification"); return; }
        final long maintenant = System.currentTimeMillis();
        if (maintenant - prefs.getLong("derniere", 0) < MAJ_ECART_MS) { Log.i("WaForSpeed", "MISEAJOUR déjà vérifiée il y a moins de 6 h"); return; }
        final long debut = SystemClock.elapsedRealtime();
        final int code = versionCode;
        final int ignoree = prefs.getInt("ignoree", 0);
        final Thread t = new Thread(new Runnable() {
            @Override
            public void run() {
                String raison;
                try {
                    final HttpURLConnection c = (HttpURLConnection) new URL(MAJ_SOURCE).openConnection();
                    c.setConnectTimeout(MAJ_DELAI_MS);
                    c.setReadTimeout(MAJ_DELAI_MS);
                    c.setRequestProperty("Accept", "application/vnd.github+json");
                    c.setRequestProperty("User-Agent", "WaForSpeed-Android");
                    try {
                        final int http = c.getResponseCode();
                        if (http != 200) { Log.i("WaForSpeed", "MISEAJOUR erreur HTTP " + http); return; }
                        final ByteArrayOutputStream tampon = new ByteArrayOutputStream();
                        try (InputStream in = c.getInputStream()) {
                            final byte[] b = new byte[16384];
                            int k;
                            while ((k = in.read(b)) > 0 && tampon.size() < 4 * 1024 * 1024) tampon.write(b, 0, k);
                        }
                        final MiseAJour.Proposition p = MiseAJour.analyser(new String(tampon.toByteArray(), StandardCharsets.UTF_8), code, ignoree);
                        if (SystemClock.elapsedRealtime() - debut > MAJ_FENETRE_MS) { Log.i("WaForSpeed", "MISEAJOUR réponse trop tardive : au prochain lancement"); return; }
                        prefs.edit().putLong("derniere", maintenant).apply();
                        if (p == null) { Log.i("WaForSpeed", "MISEAJOUR rien à proposer (installée " + code + ", ignorée " + ignoree + ")"); return; }
                        Log.i("WaForSpeed", "MISEAJOUR construction " + p.numero + " (" + p.commit + ") proposée, installée " + code);
                        runOnUiThread(new Runnable() {
                            @Override
                            public void run() { if (!isFinishing() && !isDestroyed()) proposerMiseAJour(p, prefs); }
                        });
                    } finally { c.disconnect(); }
                } catch (Exception e) {
                    Log.i("WaForSpeed", "MISEAJOUR sans réseau ou réponse illisible : " + e.getClass().getSimpleName());
                }
            }
        }, "wfs-miseajour");
        t.setDaemon(true);
        t.start();
    }

    // RG-04 / RG-05 : boîte sombre comme celle du plantage ; « Plus tard », Retour ou toucher dehors : N mémorisée
    private void proposerMiseAJour(final MiseAJour.Proposition p, final SharedPreferences prefs) {
        final AlertDialog boite = new AlertDialog.Builder(this, android.R.style.Theme_Material_Dialog_Alert)
            .setTitle("Nouvelle version disponible")
            .setMessage("Construction " + p.numero + " (commit " + p.commit + "). Tu as la construction " + versionCode + ". Ta sauvegarde est gardée.")
            .setCancelable(true)
            .setPositiveButton("Télécharger", new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface d, int which) {
                    Log.i("WaForSpeed", "MISEAJOUR « Télécharger » " + p.numero);
                    try { startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(p.adresseApk))); }
                    catch (ActivityNotFoundException e) { Log.w("WaForSpeed", "MISEAJOUR aucun navigateur"); }
                }
            })
            .setNegativeButton("Plus tard", new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface d, int which) { plusTard(p, prefs); }
            })
            .setOnCancelListener(new DialogInterface.OnCancelListener() {
                @Override
                public void onCancel(DialogInterface d) { plusTard(p, prefs); }
            })
            .create();
        boite.setOnShowListener(new DialogInterface.OnShowListener() {
            @Override
            public void onShow(DialogInterface d) {   // boutons aux couleurs du jeu (orange, cyan)
                boite.getButton(AlertDialog.BUTTON_POSITIVE).setTextColor(getColor(R.color.orange));
                boite.getButton(AlertDialog.BUTTON_NEGATIVE).setTextColor(getColor(R.color.cyan));
            }
        });
        boite.show();
    }

    private static void plusTard(MiseAJour.Proposition p, SharedPreferences prefs) {
        prefs.edit().putInt("ignoree", p.numero).apply();
        Log.i("WaForSpeed", "MISEAJOUR « Plus tard » : construction " + p.numero + " ignorée");
    }

    @Override
    protected void onCreate(Bundle etat) {
        // WFS-126 : avant super.onCreate (qui démarre le thread du jeu) : version, déclencheur de test, plantage Java,
        // rapport du lancement précédent pris et renommé
        lireVersion();
        final Intent depart = getIntent();
        final boolean demande = depart != null && depart.getBooleanExtra("wfs_plantage", false);
        final boolean declencheur = demande && apkDeTest();
        if (demande && !declencheur) Log.i("WaForSpeed", "PLANTAGE déclencheur ignoré (appli installée par le store)");
        nativeInfosPlantage(versionNom, versionCode, declencheur);
        installerPlantageJava();
        final String rapport = prendreRapportPrecedent();
        super.onCreate(etat);
        if (rapport != null) proposerRapport(rapport);
        setVolumeControlStream(AudioManager.STREAM_MUSIC);   // WFS-132 (RG-07) : boutons de volume sur le son du jeu
        final boolean testMaj = depart != null && depart.getBooleanExtra("wfs_maj_test", false) && apkDeTest();
        verifierMiseAJour(rapport != null, testMaj);   // WFS-132 (RG-01 à RG-06)
        lireLien(getIntent());   // WFS-112 : jeu lancé par le lien d'invitation
        Window w = getWindow();
        w.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);   // écran toujours allumé
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            // WFS-129 (RG-02) : Android 15 (SDK 35) impose le bord à bord ; ALWAYS (Android 11+) : image sous l'encoche sur
            // tous les bords, comme SHORT_EDGES en paysage ; les écrans restent dans les marges sûres envoyées au jeu
            WindowManager.LayoutParams lp = w.getAttributes();
            lp.layoutInDisplayCutoutMode = Build.VERSION.SDK_INT >= Build.VERSION_CODES.R
                ? WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS
                : WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
            w.setAttributes(lp);
        }
        pleinEcran();
        // à chaque changement d'insets (rotation, barres) : marges de l'encoche transmises au jeu (WFS-51)
        w.getDecorView().setOnApplyWindowInsetsListener(new View.OnApplyWindowInsetsListener() {
            @Override
            public WindowInsets onApplyWindowInsets(View v, WindowInsets insets) {
                int g = 0, h = 0, d = 0, b = 0;
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
                    DisplayCutout c = insets.getDisplayCutout();
                    if (c != null) { g = c.getSafeInsetLeft(); h = c.getSafeInsetTop(); d = c.getSafeInsetRight(); b = c.getSafeInsetBottom(); }
                }
                nativeMarges(g, h, d, b);
                // WFS-129 (RG-02) : marges relevées dans le journal (logcat), une fois par changement
                final String encarts = g + "/" + d + "/" + h + "/" + b;
                if (!encarts.equals(derniersEncarts)) {
                    derniersEncarts = encarts;
                    Log.i("WaForSpeed", "ENCARTS g/d/h/b " + encarts + " px (fenêtre " + v.getWidth() + "x" + v.getHeight() + ", API " + Build.VERSION.SDK_INT + ")");
                }
                return v.onApplyWindowInsets(insets);
            }
        });
    }

    // WFS-116 (RG-04) : la touche / le geste Retour n'arrive ici qu'au menu principal (le jeu garde l'appui partout
    // ailleurs : pause, reprise, bouton « Retour » de l'écran, retour_systeme.cpp) : le jeu passe en arrière-plan sans se
    // fermer, comme le bouton Accueil (la sauvegarde est déjà écrite à la mise en arrière-plan)
    @Override
    @SuppressWarnings("deprecation")
    public void onBackPressed() {
        moveTaskToBack(true);
    }

    @Override
    protected void onResume() {
        super.onResume();
        pleinEcran();
        performancesSoutenues(true);   // WFS-123 (RG-05)
        entrees = (InputManager) getSystemService(Context.INPUT_SERVICE);
        if (entrees != null) {
            entrees.registerInputDeviceListener(this, null);
            manettes.clear();
            for (int id : entrees.getInputDeviceIds()) onInputDeviceAdded(id);
        }
    }

    @Override
    protected void onPause() {
        if (entrees != null) entrees.unregisterInputDeviceListener(this);
        performancesSoutenues(false);   // WFS-123 (RG-05)
        super.onPause();
    }

    // WFS-123 (RG-05) : fréquences du processeur et du GPU tenues sur la durée (moins hautes, mais sans chute à la chauffe)
    private void performancesSoutenues(boolean actif) {
        final PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
        if (pm == null || !pm.isSustainedPerformanceModeSupported()) {
            if (actif) Log.i("WaForSpeed", "CHAUFFE performances soutenues non prises en charge par ce téléphone");
            return;
        }
        getWindow().setSustainedPerformanceMode(actif);
        Log.i("WaForSpeed", "CHAUFFE performances soutenues " + (actif ? "activées" : "coupées"));
    }

    // ---------- manettes (WFS-22) : connexion / déconnexion à chaud ----------
    private static boolean estManette(InputDevice d) {
        if (d == null || d.isVirtual()) return false;
        final int s = d.getSources();
        return (s & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD || (s & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK;
    }

    @Override
    public void onInputDeviceAdded(int id) { if (estManette(InputDevice.getDevice(id))) manettes.add(id); }

    @Override
    public void onInputDeviceRemoved(int id) { if (manettes.remove(id)) nativeManetteRetiree(); }

    @Override
    public void onInputDeviceChanged(int id) { onInputDeviceAdded(id); }

    @Override
    public void onWindowFocusChanged(boolean focus) {
        super.onWindowFocusChanged(focus);
        if (focus) pleinEcran();   // après une notification, un appel, le retour d'une autre appli…
    }

    // mode immersif collant
    private void pleinEcran() {
        Window w = getWindow();
        w.getDecorView().requestApplyInsets();   // relit les marges (WFS-51)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            w.setDecorFitsSystemWindows(false);
            WindowInsetsController c = w.getInsetsController();
            if (c != null) {
                c.hide(WindowInsets.Type.statusBars() | WindowInsets.Type.navigationBars());
                c.setSystemBarsBehavior(WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
            }
        } else {
            w.getDecorView().setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                | View.SYSTEM_UI_FLAG_FULLSCREEN
                | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN);
        }
    }
}
