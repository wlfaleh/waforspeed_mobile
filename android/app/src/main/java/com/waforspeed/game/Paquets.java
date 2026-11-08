// WaForSpeed — paquets de cartes côté Android (WFS-306) : liste cartes.json et téléchargement sur un fil de fond (jamais sur le fil
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
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicLong;

public final class Paquets {
    private Paquets() {}

    public static final int RIEN = 0, LISTE = 1, PRETE = 2, ABSENTE = 3, TELECHARGE = 4, FINI = 5, COUPURE = 6, SHA_FAUX = 7, PLACE = 8, ERREUR = 9;
    private static volatile int phase = RIEN;
    private static volatile ListePaquets.Entree entree = null;
    private static final AtomicLong recus = new AtomicLong(0);
    private static final AtomicBoolean arret = new AtomicBoolean(false);

    // RG-01 : lit cartes.json (copie gardée sur le téléphone si le réseau manque) et choisit le paquet de la carte pour ce téléphone
    public static synchronized void chercher(final String adresse, final String copie, final String carte, final String formats,
                                             final String empreinte, final int versionApk) {
        if (phase == TELECHARGE || phase == COUPURE) return;   // téléchargement déjà en cours (retour dans le jeu)
        phase = LISTE; entree = null;
        lancer(new Runnable() {
            @Override
            public void run() {
                String json = Telechargeur.lireTexte(adresse, 1 << 20);
                if (json != null) ecrire(new File(copie), json);
                else { json = lire(new File(copie)); Log.i("WaForSpeed", "PAQUETS liste injoignable : " + (json != null ? "copie du téléphone" : "aucune copie")); }
                final ListePaquets.Entree e = json == null ? null : ListePaquets.choisir(json, carte, formats.split(","), empreinte, versionApk);
                entree = e;
                phase = e != null ? PRETE : ABSENTE;
                Log.i("WaForSpeed", e != null ? "PAQUETS proposé : " + e.nom + " (" + e.format + ", " + e.taille + " octets)" : "PAQUETS rien à proposer (secours : carte du jeu)");
            }
        }, "wfs-paquets-liste");
    }

    // RG-03 / RG-05 : téléchargement du paquet proposé dans dossier/<carte>_<format>.wfcarte ; reprise automatique après une coupure
    public static synchronized void installer(final String dossier) {
        final ListePaquets.Entree e = entree;
        if (e == null || phase == TELECHARGE || phase == COUPURE) return;
        arret.set(false);
        phase = TELECHARGE;
        lancer(new Runnable() {
            @Override
            public void run() {
                final File fini = new File(dossier, e.carte + "_" + e.format + ".wfcarte");
                while (!arret.get()) {
                    final Telechargeur.Resultat r = Telechargeur.telecharger(e.adresse, fini, e.taille, e.sha256, recus, arret);
                    Log.i("WaForSpeed", "PAQUETS " + e.nom + " : " + r + " (" + recus.get() + " / " + e.taille + ")");
                    switch (r) {
                        case FINI: phase = FINI; return;
                        case SHA_FAUX: phase = SHA_FAUX; return;
                        case PLACE: phase = PLACE; return;
                        case ERREUR_HTTP: phase = ERREUR; return;
                        case ARRET: phase = PRETE; return;
                        default:   // COUPURE : on attend le réseau et on reprend là où on s'est arrêté
                            phase = COUPURE;
                            try { Thread.sleep(3000); } catch (InterruptedException x) { return; }
                            if (!arret.get()) phase = TELECHARGE;
                    }
                }
            }
        }, "wfs-paquets-telechargement");
    }

    public static void arreter() { arret.set(true); }

    // { phase, octets reçus, taille annoncée }
    public static long[] etat() { final ListePaquets.Entree e = entree; return new long[] {phase, recus.get(), e != null ? e.taille : 0}; }
    // { nom affiché, format de texture } du paquet proposé ; null sans proposition
    public static String[] proposition() { final ListePaquets.Entree e = entree; return e == null ? null : new String[] {e.nom, e.format}; }

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
