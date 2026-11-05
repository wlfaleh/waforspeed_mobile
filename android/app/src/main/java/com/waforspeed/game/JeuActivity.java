// WaForSpeed — activité Android du jeu (WFS-29).
// Le jeu reste entièrement natif (NativeActivity + android_main) ; cette petite sous-classe ne fait que ce qui doit se
// faire sur le thread de l'interface Android :
//   - mode immersif collant : barres d'état et de navigation masquées, qui reviennent un instant au balayage du bord,
//     réappliqué au démarrage, à la reprise et à chaque retour du focus (onWindowFocusChanged) ;
//   - écran toujours allumé pendant le jeu ;
//   - image sous l'encoche (aussi dans le thème values-v28) ;
//   - manettes (WFS-22) : une manette connue qui se déconnecte est signalée au jeu (pause automatique en course) ;
//   - touche Retour au menu principal (WFS-116) : arrière-plan au lieu de fermer le jeu ;
//   - duel entre amis (WFS-112) : lien d'invitation reçu (waforspeed://duel/<code>, au lancement ou jeu déjà ouvert),
//     lu par le jeu (prendreLienDuel) ; « Inviter » ouvre la feuille de partage d'Android (partagerTexte) ;
//   - performances soutenues (WFS-123, RG-05) : Window.setSustainedPerformanceMode(true) à la reprise si le téléphone
//     le permet (PowerManager.isSustainedPerformanceModeSupported), coupé à la mise en pause.
package com.waforspeed.game;

import android.app.NativeActivity;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.hardware.input.InputManager;
import android.os.Build;
import android.os.Bundle;
import android.os.PowerManager;
import android.util.Log;
import android.view.DisplayCutout;
import android.view.InputDevice;
import android.view.View;
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.view.WindowManager;
import java.util.HashSet;

public class JeuActivity extends NativeActivity implements InputManager.InputDeviceListener {

    // la bibliothèque du jeu (déjà chargée par NativeActivity) doit l'être aussi par Java pour la fonction JNI ci-dessous
    static { System.loadLibrary("waforspeed"); }

    // marges de sécurité (WFS-51) : encoche / coins, en pixels de la fenêtre ; lues par la boucle de jeu (android_main)
    private static native void nativeMarges(int gauche, int haut, int droite, int bas);
    // manettes (WFS-22) : une manette connue est partie (le jeu se met en pause s'il était piloté à la manette)
    private static native void nativeManetteRetiree();
    private final HashSet<Integer> manettes = new HashSet<>();
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

    @Override
    protected void onCreate(Bundle etat) {
        super.onCreate(etat);
        lireLien(getIntent());   // WFS-112 : jeu lancé par le lien d'invitation
        Window w = getWindow();
        w.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);   // écran toujours allumé
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            WindowManager.LayoutParams lp = w.getAttributes();
            lp.layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
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
