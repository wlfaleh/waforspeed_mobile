// WaForSpeed — paquets de cartes côté Android (WFS-306, plusieurs cartes : WFS-307) : liste cartes.json et téléchargement sur un fil de fond (jamais sur le fil
// du jeu ni sur celui de l'interface), lus par le jeu via JNI (src/platform/paquets_android.cpp) à chaque image de l'écran de chargement.
// États (phase) : RIEN → LISTE (lecture de cartes.json) → PRETE (paquet proposé) ou ABSENTE (pas de liste, rien pour ce téléphone :
// secours silencieux) ; INSTALLER → TELECHARGE → FINI, ou COUPURE (« Connexion perdue, reprise automatique » : nouvel essai toutes
// les 3 s tant que l'arrêt n'est pas demandé), SHA_FAUX (après un 2e essai), PLACE, ERREUR.
// Aucune permission ajoutée : INTERNET et ACCESS_NETWORK_STATE sont déjà au manifeste (RG-08).
package com.waforspeed.game;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.util.Log;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.nio.charset.StandardCharsets;

public final class Paquets {
    private Paquets() {}

    // WFS-307 : état de chaque carte et file d'attente (FilePaquets, règle pure testée) ; l'interface « une carte » de WFS-306 (chercher,
    // etat, proposition, installer, arreter) porte sur la carte de la dernière recherche (carte du chargement de l'application)
    private static final FilePaquets FILE = new FilePaquets(3000, new FilePaquets.Journal() {
        @Override public void ecrire(String t) { Log.i("WaForSpeed", "PAQUETS " + t); }
    });
    private static volatile String carteCourante = "";
    private static volatile String dossierPaquets = "";

    // RG-01 de WFS-306 : lit cartes.json (copie gardée sur le téléphone si le réseau manque), paquet de « carte » pour ce téléphone
    public static void chercher(String adresse, String copie, String carte, String formats, String empreinte, int versionApk) {
        carteCourante = carte;
        chercherTout(adresse, copie, formats, carte + ":" + empreinte, versionApk);
    }
    // WFS-307 : même lecture pour plusieurs cartes à la fois (« ville:c30f…,plage:… » : carte et empreinte attendue)
    public static void chercherTout(final String adresse, final String copie, final String formats, final String empreintes, final int versionApk) {
        final java.util.Map<String, String> emp = new java.util.LinkedHashMap<>();
        for (String p : empreintes.split(",")) { final int k = p.indexOf(':'); if (k > 0) emp.put(p.substring(0, k), p.substring(k + 1)); }
        FILE.recherche(emp.keySet());
        lancer(new Runnable() {
            @Override
            public void run() {
                String json = Telechargeur.lireTexte(adresse, 1 << 20);
                if (json != null) ecrire(new File(copie), json);
                else { json = lire(new File(copie)); Log.i("WaForSpeed", "PAQUETS liste injoignable : " + (json != null ? "copie du téléphone" : "aucune copie")); }
                if (json == null) FILE.listeAbsente(emp.keySet());
                else FILE.liste(json, formats.split(","), emp, versionApk);
                Log.i("WaForSpeed", "PAQUETS liste lue pour " + emp.keySet());
            }
        }, "wfs-paquets-liste");
    }

    // RG-03 de WFS-306 / RG-02 de WFS-307 : téléchargement (file d'attente, une carte à la fois)
    public static void installer(String dossier) { installerCarte(dossier, carteCourante); }
    public static boolean installerCarte(String dossier, String carte) { dossierPaquets = dossier; return FILE.installer(dossier, carte); }
    public static void arreter() { FILE.arreter(carteCourante); }
    public static void arreterCarte(String carte) { FILE.arreter(carte); }
    public static void supprimee(String carte) { FILE.supprimee(carte); }
    // RG-03 de WFS-307 : débit réduit pendant la course quand la cadence du jeu passe sous 30 images par seconde
    public static void ralentir(boolean oui) { Telechargeur.pauseMs = oui ? 20 : 0; }

    // { phase, octets reçus, taille annoncée }
    public static long[] etat() { return FILE.etat(carteCourante); }
    public static long[] etatCarte(String carte) { return FILE.etat(carte); }
    // { nom affiché, format de texture } du paquet proposé ; null sans proposition
    public static String[] proposition() { return FILE.proposition(carteCourante); }
    public static String[] propositionCarte(String carte) { return FILE.proposition(carte); }

    // RG-02 : 0 sans réseau, 1 Wi-Fi (ou réseau non facturé : Ethernet), 2 données mobiles
    public static int reseau(Context ctx) {
        try {
            final ConnectivityManager cm = (ConnectivityManager) ctx.getSystemService(Context.CONNECTIVITY_SERVICE);
            final Network n = cm == null ? null : cm.getActiveNetwork();
            final NetworkCapabilities c = n == null ? null : cm.getNetworkCapabilities(n);
            if (c == null || !c.hasCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET)) return 0;
            if (c.hasTransport(NetworkCapabilities.TRANSPORT_WIFI) || c.hasTransport(NetworkCapabilities.TRANSPORT_ETHERNET)) return 1;
            return c.hasCapability(NetworkCapabilities.NET_CAPABILITY_NOT_METERED) ? 1 : 2;
        } catch (RuntimeException e) { return 0; }
    }

    // RG-07 : place libre du dossier des paquets (créé au besoin), en octets
    public static long placeLibre(String dossier) { final File d = new File(dossier); d.mkdirs(); return d.getUsableSpace(); }

    private static void lancer(Runnable r, String nom) {
        final Thread t = new Thread(r, nom);
        t.setDaemon(true);

        t.start();
    }
    private static void ecrire(File f, String texte) {
        final File tmp = new File(f.getPath() + ".tmp");
        try (FileOutputStream o = new FileOutputStream(tmp)) { o.write(texte.getBytes(StandardCharsets.UTF_8)); } catch (Exception e) { return; }
        tmp.renameTo(f);
    }
    private static String lire(File f) {
        if (!f.isFile() || f.length() > (1 << 20)) return null;
        try (FileInputStream in = new FileInputStream(f)) {
            final byte[] b = new byte[(int) f.length()];
            int n = 0, k;
            while (n < b.length && (k = in.read(b, n, b.length - n)) > 0) n += k;
            return new String(b, 0, n, StandardCharsets.UTF_8);
        } catch (Exception e) { return null; }
    }
}
