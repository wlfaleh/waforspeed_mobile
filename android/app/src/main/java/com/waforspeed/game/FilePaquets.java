// WaForSpeed — paquets de plusieurs cartes (WFS-307) : état de chaque carte et file d'attente des téléchargements (une carte à la fois,
// RG-02), règle pure sans Android (testée par JUnit, FilePaquetsTest). Chaque carte a sa phase (Installation.*, plus ATTENTE quand elle
// attend son tour), ses octets reçus et son paquet proposé (ListePaquets.Entree). Le téléchargement lui-même est celui de WFS-306
// (Installation.executer : .part, Range, SHA-256, reprise après coupure, arrêt qui rend PRETE).
package com.waforspeed.game;

import java.io.File;
import java.util.ArrayDeque;
import java.util.Collection;
import java.util.Deque;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicLong;

public final class FilePaquets {
    public static final int ATTENTE = 10;   // dans la file, après la carte en cours (src/ui/ecrans/paquets.h : SourcePaquets::ATTENTE)

    public interface Journal { void ecrire(String texte); }

    private static final class Carte {
        volatile int phase = Installation.RIEN;
        final AtomicLong recus = new AtomicLong(0);
        final AtomicBoolean arret = new AtomicBoolean(false);
        volatile ListePaquets.Entree entree = null;
        boolean occupee() { return phase == Installation.TELECHARGE || phase == Installation.COUPURE || phase == ATTENTE; }
    }

    private final Map<String, Carte> cartes = new ConcurrentHashMap<>();
    private final Deque<String> file = new ArrayDeque<>();
    private final long attenteMs;
    private final Journal journal;
    private String dossier = null;
    private Thread travailleur = null;

    public FilePaquets(long attenteMs, Journal journal) { this.attenteMs = attenteMs; this.journal = journal; }

    private Carte carte(String c) { return cartes.computeIfAbsent(c, k -> new Carte()); }

    // lecture de la liste en cours pour ces cartes (celles qui téléchargent ne changent pas)
    public synchronized void recherche(Collection<String> noms) { for (String c : noms) { final Carte k = carte(c); if (!k.occupee()) k.phase = Installation.LISTE; } }
    // pas de liste (hors ligne, 404, illisible) : rien à proposer pour ces cartes (secours)
    public synchronized void listeAbsente(Collection<String> noms) {
        for (String c : noms) { final Carte k = carte(c); if (!k.occupee()) { k.phase = Installation.ABSENTE; k.entree = null; } }
    }
    // liste lue : paquet de chaque carte pour ce téléphone (empreintes : carte → empreinte attendue en hexadécimal)
    public synchronized void liste(String json, String[] formats, Map<String, String> empreintes, int versionApk) {
        for (Map.Entry<String, String> m : empreintes.entrySet()) {
            final Carte k = carte(m.getKey());
            if (k.occupee()) continue;
            k.entree = ListePaquets.choisir(json, m.getKey(), formats, m.getValue(), versionApk);
            k.phase = k.entree != null ? Installation.PRETE : Installation.ABSENTE;
        }
    }

    // RG-02 : file d'attente ; vrai si la carte y entre (paquet proposé, pas déjà en cours ou en attente)
    public synchronized boolean installer(String dossierPaquets, String c) {
        final Carte k = carte(c);
        if (k.entree == null || k.occupee()) return false;
        dossier = dossierPaquets;
        k.arret.set(false);
        k.phase = ATTENTE;
        file.addLast(c);
        if (travailleur == null) {
            travailleur = new Thread(this::travailler, "wfs-paquets-file");
            travailleur.setDaemon(true);
            travailleur.start();
        }
        return true;
    }
    // « Plus tard » / annuler : retirée de la file si elle attend ; arrêt demandé si c'est la carte en cours (.part gardé)
    public synchronized void arreter(String c) {
        final Carte k = carte(c);
        if (file.remove(c)) { k.phase = Installation.PRETE; return; }
        k.arret.set(true);
    }
    // { phase, octets reçus, taille annoncée }
    public long[] etat(String c) { final Carte k = carte(c); final ListePaquets.Entree e = k.entree; return new long[] {k.phase, k.recus.get(), e != null ? e.taille : 0}; }
    // { nom affiché, format } du paquet proposé, null sans proposition
    public String[] proposition(String c) { final ListePaquets.Entree e = carte(c).entree; return e == null ? null : new String[] {e.nom, e.format}; }
    // après une suppression par le jeu : la carte redevient proposable (son paquet reste dans la liste lue)
    public synchronized void supprimee(String c) { final Carte k = carte(c); if (!k.occupee()) { k.recus.set(0); k.phase = k.entree != null ? Installation.PRETE : Installation.ABSENTE; } }

    // tests seulement (FilePaquetsTest) : adresse locale http://127.0.0.1 à la place de l'adresse https de la liste
    synchronized void adresseDeTest(String c, String adresse) {
        final ListePaquets.Entree e = carte(c).entree;
        if (e != null) carte(c).entree = new ListePaquets.Entree(e.carte, e.nom, e.format, e.empreinte, e.sha256, adresse, e.taille, e.versionCarte);
    }

    private void travailler() {
        while (true) {
            final String c; final Carte k; final String d;
            synchronized (this) {
                c = file.pollFirst();
                if (c == null) { travailleur = null; return; }
                k = carte(c); d = dossier;
            }
            final ListePaquets.Entree e = k.entree;
            if (e == null) { k.phase = Installation.ABSENTE; continue; }
            Installation.executer(e.adresse, new File(d, e.carte + "_" + e.format + ".wfcarte"), e.taille, e.sha256, k.recus, k.arret, attenteMs,
                new Installation.Suivi() {
                    @Override public void phase(int p) { k.phase = p; }
                    @Override public void journal(String t) { if (journal != null) journal.ecrire(t); }
                });
        }
    }
}
