// WaForSpeed — activité Android du jeu (WFS-29).
// Le jeu reste entièrement natif (NativeActivity + android_main) ; cette petite sous-classe ne fait que ce qui doit se
// faire sur le thread de l'interface Android :
//   - mode immersif collant : barres d'état et de navigation masquées, qui reviennent un instant au balayage du bord,
//     réappliqué au démarrage, à la reprise et à chaque retour du focus (onWindowFocusChanged) ;
//   - écran toujours allumé pendant le jeu ;
//   - image sous l'encoche (aussi dans le thème values-v28) ;
//   - manettes (WFS-22) : une manette connue qui se déconnecte est signalée au jeu (pause automatique en course).
package com.waforspeed.game;

import android.app.NativeActivity;
import android.content.Context;
import android.hardware.input.InputManager;
import android.os.Build;
import android.os.Bundle;
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

    @Override
    protected void onCreate(Bundle etat) {
        super.onCreate(etat);
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

    @Override
    protected void onResume() {
        super.onResume();
        pleinEcran();
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
        super.onPause();
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
