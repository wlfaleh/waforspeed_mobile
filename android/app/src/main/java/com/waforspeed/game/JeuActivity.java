// WaForSpeed — activité Android du jeu (WFS-29).
// Le jeu reste entièrement natif (NativeActivity + android_main) ; cette petite sous-classe ne fait que ce qui doit se
// faire sur le thread de l'interface Android :
//   - mode immersif collant : barres d'état et de navigation masquées, qui reviennent un instant au balayage du bord,
//     réappliqué au démarrage, à la reprise et à chaque retour du focus (onWindowFocusChanged) ;
//   - écran toujours allumé pendant le jeu ;
//   - image sous l'encoche (aussi dans le thème values-v28).
package com.waforspeed.game;

import android.app.NativeActivity;
import android.os.Build;
import android.os.Bundle;
import android.view.View;
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.view.WindowManager;

public class JeuActivity extends NativeActivity {

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
    }

    @Override
    protected void onResume() {
        super.onResume();
        pleinEcran();
    }

    @Override
    public void onWindowFocusChanged(boolean focus) {
        super.onWindowFocusChanged(focus);
        if (focus) pleinEcran();   // après une notification, un appel, le retour d'une autre appli…
    }

    // mode immersif collant
    private void pleinEcran() {
        Window w = getWindow();
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
