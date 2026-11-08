// WFS-306 (RG-03, RG-05, RG-07, RG-09) : téléchargement avec un petit serveur local (com.sun.net.httpserver, dans le JDK) sur 127.0.0.1 :
// fichier complet, reprise par Range après une coupure, serveur sans Range (repart de zéro), SHA-256 faux (deux essais puis refus,
// .part effacé, ancien fichier gardé), redirection, erreur 404, place insuffisante, arrêt demandé.
package com.waforspeed.game;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import com.sun.net.httpserver.HttpExchange;
import com.sun.net.httpserver.HttpServer;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.net.InetSocketAddress;
import java.nio.file.Files;
import java.security.MessageDigest;
import java.util.Random;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

public class TelechargeurTest {
    private HttpServer serveur;
    private String base;
    private File dossier;
    private final byte[] donnees = new byte[300_000];
    private String sha;
    private final AtomicInteger coupureA = new AtomicInteger(-1);   // le serveur coupe après N octets (une fois)
    private final AtomicBoolean rangeAccepte = new AtomicBoolean(true);
    private final AtomicInteger requetes = new AtomicInteger(0);

    private static String hex(byte[] b) throws Exception {
        final StringBuilder s = new StringBuilder();
        for (byte x : MessageDigest.getInstance("SHA-256").digest(b)) s.append(String.format("%02x", x & 0xff));
        return s.toString();
    }

    @Before public void demarrer() throws Exception {
        new Random(42).nextBytes(donnees);
        sha = hex(donnees);
        dossier = Files.createTempDirectory("paquets").toFile();
        serveur = HttpServer.create(new InetSocketAddress("127.0.0.1", 0), 0);
        serveur.createContext("/ville.wfcarte", this::servir);
        serveur.createContext("/redirige", ex -> { ex.getResponseHeaders().add("Location", "/ville.wfcarte"); ex.sendResponseHeaders(302, -1); ex.close(); });
        serveur.start();
        base = "http://127.0.0.1:" + serveur.getAddress().getPort();
    }
    @After public void arreter() { serveur.stop(0); for (File f : dossier.listFiles()) f.delete(); dossier.delete(); }

    private void servir(HttpExchange ex) throws IOException {
        requetes.incrementAndGet();
        int debut = 0;
        final String r = ex.getRequestHeaders().getFirst("Range");
        if (r != null && rangeAccepte.get()) debut = Integer.parseInt(r.replace("bytes=", "").replace("-", ""));
        final int n = donnees.length - debut;
        ex.sendResponseHeaders(debut > 0 ? 206 : 200, n);
        try (OutputStream o = ex.getResponseBody()) {
            final int coupe = coupureA.getAndSet(-1);
            if (coupe >= 0) { o.write(donnees, debut, coupe); o.flush(); ex.close(); return; }
            o.write(donnees, debut, n);
        } catch (IOException e) { /* client parti */ }
    }

    private File fini() { return new File(dossier, "ville_astc.wfcarte"); }
    private Telechargeur.Resultat telecharger(String chemin, String s) {
        return Telechargeur.telecharger(base + chemin, fini(), donnees.length, s, new AtomicLong(), new AtomicBoolean(false));
    }

    @Test public void fichierCompletVerifieEtRenomme() throws Exception {
        assertEquals(Telechargeur.Resultat.FINI, telecharger("/ville.wfcarte", sha));
        assertArrayEquals(donnees, Files.readAllBytes(fini().toPath()));
        assertFalse(Telechargeur.part(fini()).exists());
    }
    @Test public void repriseParRangeApresCoupure() throws Exception {
        coupureA.set(120_000);
        final Telechargeur.Resultat r1 = telecharger("/ville.wfcarte", sha);
        assertEquals(Telechargeur.Resultat.COUPURE, r1);
        final long dejaLa = Telechargeur.part(fini()).length();
        assertTrue(dejaLa > 0 && dejaLa < donnees.length);
        assertFalse(fini().exists());   // le C++ ne voit jamais un fichier non fini
        assertEquals(Telechargeur.Resultat.FINI, telecharger("/ville.wfcarte", sha));
        assertArrayEquals(donnees, Files.readAllBytes(fini().toPath()));
    }
    @Test public void serveurSansRangeRepartDeZero() throws Exception {
        rangeAccepte.set(false);
        try (FileOutputStream o = new FileOutputStream(Telechargeur.part(fini()))) { o.write(donnees, 0, 1000); }
        assertEquals(Telechargeur.Resultat.FINI, telecharger("/ville.wfcarte", sha));
        assertArrayEquals(donnees, Files.readAllBytes(fini().toPath()));
    }
    @Test public void shaFauxDeuxEssaisPuisRefusAncienGarde() throws Exception {
        Files.write(fini().toPath(), new byte[] {1, 2, 3});   // ancien paquet
        final String faux = sha.replace(sha.charAt(0), sha.charAt(0) == 'a' ? 'b' : 'a');
        assertEquals(Telechargeur.Resultat.SHA_FAUX, telecharger("/ville.wfcarte", faux));
        assertEquals(2, requetes.get());
        assertFalse(Telechargeur.part(fini()).exists());
        assertArrayEquals(new byte[] {1, 2, 3}, Files.readAllBytes(fini().toPath()));   // RG-05 : ancien gardé
    }
    @Test public void redirectionSuivie() throws Exception {
        assertEquals(Telechargeur.Resultat.FINI, telecharger("/redirige", sha));
    }
    @Test public void erreurHttp() {
        assertEquals(Telechargeur.Resultat.ERREUR_HTTP, telecharger("/absent", sha));
        assertEquals(null, Telechargeur.lireTexte(base + "/absent", 1000));
    }
    @Test public void placeInsuffisante() {
        final long enorme = Long.MAX_VALUE / 4;
        assertEquals(Telechargeur.Resultat.PLACE, Telechargeur.telecharger(base + "/ville.wfcarte", fini(), enorme, sha, new AtomicLong(), new AtomicBoolean(false)));
        assertEquals(0, requetes.get());   // rien n'est demandé au serveur
        assertEquals(1100L, Telechargeur.placeNecessaire(1000, 0));
        assertEquals(600L, Telechargeur.placeNecessaire(1000, 500));
    }
    @Test public void arretDemande() {
        assertEquals(Telechargeur.Resultat.ARRET, Telechargeur.telecharger(base + "/ville.wfcarte", fini(), donnees.length, sha, new AtomicLong(), new AtomicBoolean(true)));
        assertFalse(fini().exists());
    }
}
