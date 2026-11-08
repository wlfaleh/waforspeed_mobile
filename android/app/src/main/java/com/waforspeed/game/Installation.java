// WaForSpeed — boucle d'installation d'un paquet de carte (WFS-306 RG-03, RG-05) : règle pure, sans Android (testée par JUnit,
// InstallationTest). Télécharge (Telechargeur), reprend automatiquement après une coupure (attente puis nouvel essai), et rend toujours
// la phase PRETE quand l'arrêt est demandé (« Plus tard »), y compris pendant l'attente après une coupure : le .part est gardé et une
// nouvelle installation reste possible dans la même session.
package com.waforspeed.game;

import java.io.File;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicLong;

public final class Installation {
    private Installation() {}

    // phases lues par le jeu (Paquets.etat(), src/ui/ecrans/paquets.h : SourcePaquets::Phase)
    public static final int RIEN = 0, LISTE = 1, PRETE = 2, ABSENTE = 3, TELECHARGE = 4, FINI = 5, COUPURE = 6, SHA_FAUX = 7, PLACE = 8, ERREUR = 9;

    public interface Suivi {
        void phase(int p);
        void journal(String texte);
    }

    public static void executer(String adresse, File fini, long taille, String sha256, AtomicLong recus, AtomicBoolean arret, long attenteMs, Suivi s) {
        s.phase(TELECHARGE);
        while (true) {
            if (arret.get()) { s.phase(PRETE); return; }
            final Telechargeur.Resultat r = Telechargeur.telecharger(adresse, fini, taille, sha256, recus, arret);
            s.journal(fini.getName() + " : " + r + " (" + recus.get() + " / " + taille + ")");
            switch (r) {
                case FINI: s.phase(FINI); return;
                case SHA_FAUX: s.phase(SHA_FAUX); return;
                case PLACE: s.phase(PLACE); return;
                case ERREUR_HTTP: s.phase(ERREUR); return;
                case ARRET: s.phase(PRETE); return;
                default:   // COUPURE : attente du réseau, puis reprise là où on s'est arrêté
                    s.phase(COUPURE);
                    try { Thread.sleep(attenteMs); } catch (InterruptedException x) { s.phase(PRETE); return; }
                    if (arret.get()) { s.phase(PRETE); return; }
                    s.phase(TELECHARGE);
            }
        }
    }
}
